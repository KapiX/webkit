/*
 * Copyright (C) 2006, 2007, 2008, 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "DataTransfer.h"

#include "CachedImage.h"
#include "CachedImageClient.h"
#include "DataTransferItem.h"
#include "DataTransferItemList.h"
#include "DeprecatedGlobalSettings.h"
#include "DocumentFragment.h"
#include "DragData.h"
#include "Editor.h"
#include "FileList.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "HTMLImageElement.h"
#include "HTMLParserIdioms.h"
#include "Image.h"
#include "Pasteboard.h"
#include "Settings.h"
#include "StaticPasteboard.h"
#include "URLParser.h"
#include "WebContentReader.h"
#include "WebCorePasteboardFileReader.h"
#include "markup.h"

namespace WebCore {

#if ENABLE(DRAG_SUPPORT)

class DragImageLoader final : private CachedImageClient {
    WTF_MAKE_NONCOPYABLE(DragImageLoader); WTF_MAKE_FAST_ALLOCATED;
public:
    explicit DragImageLoader(DataTransfer*);
    void startLoading(CachedResourceHandle<CachedImage>&);
    void stopLoading(CachedResourceHandle<CachedImage>&);
    void moveToDataTransfer(DataTransfer&);

private:
    void imageChanged(CachedImage*, const IntRect*) override;
    DataTransfer* m_dataTransfer;
};

#endif

DataTransfer::DataTransfer(StoreMode mode, std::unique_ptr<Pasteboard> pasteboard, Type type)
    : m_storeMode(mode)
    , m_pasteboard(WTFMove(pasteboard))
#if ENABLE(DRAG_SUPPORT)
    , m_type(type)
    , m_dropEffect(ASCIILiteral("uninitialized"))
    , m_effectAllowed(ASCIILiteral("uninitialized"))
    , m_shouldUpdateDragImage(false)
#endif
{
#if !ENABLE(DRAG_SUPPORT)
    ASSERT_UNUSED(type, type != Type::DragAndDropData && type != Type::DragAndDropFiles);
#endif
}

Ref<DataTransfer> DataTransfer::createForCopyAndPaste(Document& document, StoreMode storeMode, std::unique_ptr<Pasteboard>&& pasteboard)
{
    auto dataTransfer = adoptRef(*new DataTransfer(storeMode, WTFMove(pasteboard)));
    dataTransfer->m_originIdentifier = document.originIdentifierForPasteboard();
    return dataTransfer;
}

DataTransfer::~DataTransfer()
{
#if ENABLE(DRAG_SUPPORT)
    if (m_dragImageLoader && m_dragImage)
        m_dragImageLoader->stopLoading(m_dragImage);
#endif
}

bool DataTransfer::canReadTypes() const
{
    return m_storeMode == StoreMode::Readonly || m_storeMode == StoreMode::Protected || m_storeMode == StoreMode::ReadWrite;
}

bool DataTransfer::canReadData() const
{
    return m_storeMode == StoreMode::Readonly || m_storeMode == StoreMode::ReadWrite;
}

bool DataTransfer::canWriteData() const
{
    return m_storeMode == StoreMode::ReadWrite;
}

static String normalizeType(const String& type)
{
    if (type.isNull())
        return type;

    String lowercaseType = stripLeadingAndTrailingHTMLSpaces(type).convertToASCIILowercase();
    if (lowercaseType == "text" || lowercaseType.startsWith("text/plain;"))
        return "text/plain";
    if (lowercaseType == "url" || lowercaseType.startsWith("text/uri-list;"))
        return "text/uri-list";
    if (lowercaseType.startsWith("text/html;"))
        return "text/html";

    return lowercaseType;
}

void DataTransfer::clearData(const String& type)
{
    if (!canWriteData())
        return;

    String normalizedType = normalizeType(type);
    if (normalizedType.isNull())
        m_pasteboard->clear();
    else
        m_pasteboard->clear(normalizedType);
    if (m_itemList)
        m_itemList->didClearStringData(normalizedType);
}

String DataTransfer::getDataForItem(Document& document, const String& type) const
{
    if (!canReadData())
        return { };

    auto lowercaseType = stripLeadingAndTrailingHTMLSpaces(type).convertToASCIILowercase();
    if (shouldSuppressGetAndSetDataToAvoidExposingFilePaths()) {
        if (lowercaseType == "text/uri-list") {
            auto urlString = m_pasteboard->readString(lowercaseType);
            if (Pasteboard::canExposeURLToDOMWhenPasteboardContainsFiles(urlString))
                return urlString;
        }
        return { };
    }

    if (!DeprecatedGlobalSettings::customPasteboardDataEnabled())
        return m_pasteboard->readString(lowercaseType);

    // StaticPasteboard is only used to stage data written by websites before being committed to the system pasteboard.
    bool isSameOrigin = is<StaticPasteboard>(*m_pasteboard) || (!m_originIdentifier.isNull() && m_originIdentifier == m_pasteboard->readOrigin());
    if (isSameOrigin) {
        String value = m_pasteboard->readStringInCustomData(lowercaseType);
        if (!value.isNull())
            return value;
    }
    if (!Pasteboard::isSafeTypeForDOMToReadAndWrite(lowercaseType))
        return { };

    if (!is<StaticPasteboard>(*m_pasteboard) && type == "text/html") {
        if (!document.frame())
            return { };
        WebContentMarkupReader reader { *document.frame() };
        m_pasteboard->read(reader);
        return reader.markup;
    }

    return m_pasteboard->readString(type);
}

String DataTransfer::getData(Document& document, const String& type) const
{
    return getDataForItem(document, normalizeType(type));
}

bool DataTransfer::shouldSuppressGetAndSetDataToAvoidExposingFilePaths() const
{
    if (!forFileDrag() && !DeprecatedGlobalSettings::customPasteboardDataEnabled())
        return false;
    return m_pasteboard->containsFiles();
}

void DataTransfer::setData(const String& type, const String& data)
{
    if (!canWriteData())
        return;

    if (shouldSuppressGetAndSetDataToAvoidExposingFilePaths())
        return;

    auto normalizedType = normalizeType(type);
    setDataFromItemList(normalizedType, data);
    if (m_itemList)
        m_itemList->didSetStringData(normalizedType);
}

void DataTransfer::setDataFromItemList(const String& type, const String& data)
{
    ASSERT(canWriteData());
    RELEASE_ASSERT(is<StaticPasteboard>(*m_pasteboard));

    if (!DeprecatedGlobalSettings::customPasteboardDataEnabled()) {
        m_pasteboard->writeString(type, data);
        return;
    }

    String sanitizedData;
    if (type == "text/html")
        sanitizedData = sanitizeMarkup(data);
    else if (type == "text/uri-list") {
        auto url = URLParser(data).result();
        if (url.isValid())
            sanitizedData = url.string();
    } else if (type == "text/plain")
        sanitizedData = data; // Nothing to sanitize.

    if (sanitizedData != data)
        downcast<StaticPasteboard>(*m_pasteboard).writeStringInCustomData(type, data);

    if (Pasteboard::isSafeTypeForDOMToReadAndWrite(type) && !sanitizedData.isNull())
        m_pasteboard->writeString(type, sanitizedData);
}

void DataTransfer::updateFileList()
{
    ASSERT(canWriteData());

    m_fileList->m_files = filesFromPasteboardAndItemList();
}

void DataTransfer::didAddFileToItemList()
{
    ASSERT(canWriteData());
    if (!m_fileList)
        return;

    auto& newItem = m_itemList->items().last();
    ASSERT(newItem->isFile());
    m_fileList->append(*newItem->file());
}

DataTransferItemList& DataTransfer::items()
{
    if (!m_itemList)
        m_itemList = std::make_unique<DataTransferItemList>(*this);
    return *m_itemList;
}

Vector<String> DataTransfer::types() const
{
    return types(AddFilesType::Yes);
}

Vector<String> DataTransfer::typesForItemList() const
{
    return types(AddFilesType::No);
}

Vector<String> DataTransfer::types(AddFilesType addFilesType) const
{
    if (!canReadTypes())
        return { };
    
    if (!DeprecatedGlobalSettings::customPasteboardDataEnabled()) {
        auto types = m_pasteboard->typesForLegacyUnsafeBindings();
        ASSERT(!types.contains("Files"));
        if (m_pasteboard->containsFiles() && addFilesType == AddFilesType::Yes)
            types.append("Files");
        return types;
    }

    auto safeTypes = m_pasteboard->typesSafeForBindings(m_originIdentifier);
    bool hasFileBackedItem = m_itemList && m_itemList->hasItems() && notFound != m_itemList->items().findMatching([] (const auto& item) {
        return item->isFile();
    });

    if (hasFileBackedItem || m_pasteboard->containsFiles()) {
        Vector<String> types;
        if (addFilesType == AddFilesType::Yes)
            types.append(ASCIILiteral("Files"));
        if (safeTypes.contains("text/uri-list"))
            types.append(ASCIILiteral("text/uri-list"));
        return types;
    }

    ASSERT(!safeTypes.contains("Files"));
    return safeTypes;
}

Vector<Ref<File>> DataTransfer::filesFromPasteboardAndItemList() const
{
    bool addedFilesFromPasteboard = false;
    Vector<Ref<File>> files;
    if (!forDrag() || forFileDrag()) {
        WebCorePasteboardFileReader reader;
        m_pasteboard->read(reader);
        files = WTFMove(reader.files);
        addedFilesFromPasteboard = !files.isEmpty();
    }

    bool itemListContainsItems = false;
    if (m_itemList && m_itemList->hasItems()) {
        for (auto& item : m_itemList->items()) {
            if (auto file = item->file())
                files.append(file.releaseNonNull());
        }
        itemListContainsItems = true;
    }

    bool containsItemsAndFiles = itemListContainsItems && addedFilesFromPasteboard;
    ASSERT_UNUSED(containsItemsAndFiles, !containsItemsAndFiles);
    return files;
}

FileList& DataTransfer::files() const
{
    if (!canReadData()) {
        if (m_fileList)
            m_fileList->clear();
        else
            m_fileList = FileList::create();
        return *m_fileList;
    }

    if (!m_fileList)
        m_fileList = FileList::create(filesFromPasteboardAndItemList());

    return *m_fileList;
}

struct PasteboardFileTypeReader final : PasteboardFileReader {
    void readFilename(const String& filename)
    {
        types.add(File::contentTypeForFile(filename));
    }

    void readBuffer(const String&, const String& type, Ref<SharedBuffer>&&)
    {
        types.add(type);
    }

    HashSet<String, ASCIICaseInsensitiveHash> types;
};

bool DataTransfer::hasFileOfType(const String& type)
{
    ASSERT_WITH_SECURITY_IMPLICATION(canReadTypes());
    PasteboardFileTypeReader reader;
    m_pasteboard->read(reader);
    return reader.types.contains(type);
}

bool DataTransfer::hasStringOfType(const String& type)
{
    ASSERT_WITH_SECURITY_IMPLICATION(canReadTypes());

    return !type.isNull() && types().contains(type);
}

Ref<DataTransfer> DataTransfer::createForInputEvent(const String& plainText, const String& htmlText)
{
    auto pasteboard = std::make_unique<StaticPasteboard>();
    pasteboard->writeString(ASCIILiteral("text/plain"), plainText);
    pasteboard->writeString(ASCIILiteral("text/html"), htmlText);
    return adoptRef(*new DataTransfer(StoreMode::Readonly, WTFMove(pasteboard), Type::InputEvent));
}

void DataTransfer::commitToPasteboard(Pasteboard& nativePasteboard)
{
    ASSERT(is<StaticPasteboard>(*m_pasteboard) && !is<StaticPasteboard>(nativePasteboard));
    PasteboardCustomData customData = downcast<StaticPasteboard>(*m_pasteboard).takeCustomData();
    if (DeprecatedGlobalSettings::customPasteboardDataEnabled()) {
        customData.origin = m_originIdentifier;
        nativePasteboard.writeCustomData(customData);
        return;
    }

    for (auto& entry : customData.platformData)
        nativePasteboard.writeString(entry.key, entry.value);
    for (auto& entry : customData.sameOriginCustomData)
        nativePasteboard.writeString(entry.key, entry.value);
}

#if !ENABLE(DRAG_SUPPORT)

String DataTransfer::dropEffect() const
{
    return ASCIILiteral("none");
}

void DataTransfer::setDropEffect(const String&)
{
}

String DataTransfer::effectAllowed() const
{
    return ASCIILiteral("uninitialized");
}

void DataTransfer::setEffectAllowed(const String&)
{
}

void DataTransfer::setDragImage(Element*, int, int)
{
}

#else

Ref<DataTransfer> DataTransfer::createForDrag()
{
    return adoptRef(*new DataTransfer(StoreMode::ReadWrite, Pasteboard::createForDragAndDrop(), Type::DragAndDropData));
}

Ref<DataTransfer> DataTransfer::createForDragStartEvent(Document& document)
{
    auto dataTransfer = adoptRef(*new DataTransfer(StoreMode::ReadWrite, std::make_unique<StaticPasteboard>(), Type::DragAndDropData));
    dataTransfer->m_originIdentifier = document.originIdentifierForPasteboard();
    return dataTransfer;
}

Ref<DataTransfer> DataTransfer::createForDrop(Document& document, std::unique_ptr<Pasteboard>&& pasteboard, DragOperation sourceOperation, bool draggingFiles)
{
    auto dataTransfer = adoptRef(*new DataTransfer(DataTransfer::StoreMode::Readonly, WTFMove(pasteboard), draggingFiles ? Type::DragAndDropFiles : Type::DragAndDropData));
    dataTransfer->setSourceOperation(sourceOperation);
    dataTransfer->m_originIdentifier = document.originIdentifierForPasteboard();
    return dataTransfer;
}

Ref<DataTransfer> DataTransfer::createForUpdatingDropTarget(Document& document, std::unique_ptr<Pasteboard>&& pasteboard, DragOperation sourceOperation, bool draggingFiles)
{
    auto mode = DataTransfer::StoreMode::Protected;
#if ENABLE(DASHBOARD_SUPPORT)
    if (document.settings().usesDashboardBackwardCompatibilityMode() && document.securityOrigin().isLocal())
        mode = DataTransfer::StoreMode::Readonly;
#else
    UNUSED_PARAM(document);
#endif
    auto dataTransfer = adoptRef(*new DataTransfer(mode, WTFMove(pasteboard), draggingFiles ? Type::DragAndDropFiles : Type::DragAndDropData));
    dataTransfer->setSourceOperation(sourceOperation);
    dataTransfer->m_originIdentifier = document.originIdentifierForPasteboard();
    return dataTransfer;
}

void DataTransfer::setDragImage(Element* element, int x, int y)
{
    if (!forDrag() || !canWriteData())
        return;

    CachedImage* image = nullptr;
    if (is<HTMLImageElement>(element) && !element->isConnected())
        image = downcast<HTMLImageElement>(*element).cachedImage();

    m_dragLocation = IntPoint(x, y);

    if (m_dragImageLoader && m_dragImage)
        m_dragImageLoader->stopLoading(m_dragImage);
    m_dragImage = image;
    if (m_dragImage) {
        if (!m_dragImageLoader)
            m_dragImageLoader = std::make_unique<DragImageLoader>(this);
        m_dragImageLoader->startLoading(m_dragImage);
    }

    m_dragImageElement = image ? nullptr : element;

    updateDragImage();
}

void DataTransfer::updateDragImage()
{
    // Don't allow setting the image if we haven't started dragging yet; we'll rely on the dragging code
    // to install this drag image as part of getting the drag kicked off.
    if (!m_shouldUpdateDragImage)
        return;

    IntPoint computedHotSpot;
    auto computedImage = DragImage { createDragImage(computedHotSpot) };
    if (!computedImage)
        return;

    m_pasteboard->setDragImage(WTFMove(computedImage), computedHotSpot);
}

RefPtr<Element> DataTransfer::dragImageElement() const
{
    return m_dragImageElement;
}

#if !PLATFORM(MAC)

DragImageRef DataTransfer::createDragImage(IntPoint& location) const
{
    location = m_dragLocation;

    if (m_dragImage)
        return createDragImageFromImage(m_dragImage->image(), ImageOrientationDescription());

    if (m_dragImageElement) {
        if (Frame* frame = m_dragImageElement->document().frame())
            return createDragImageForNode(*frame, *m_dragImageElement);
    }

    // We do not have enough information to create a drag image, use the default icon.
    return nullptr;
}

#endif

DragImageLoader::DragImageLoader(DataTransfer* dataTransfer)
    : m_dataTransfer(dataTransfer)
{
}

void DragImageLoader::moveToDataTransfer(DataTransfer& newDataTransfer)
{
    m_dataTransfer = &newDataTransfer;
}

void DragImageLoader::startLoading(CachedResourceHandle<WebCore::CachedImage>& image)
{
    // FIXME: Does this really trigger a load? Does it need to?
    image->addClient(*this);
}

void DragImageLoader::stopLoading(CachedResourceHandle<WebCore::CachedImage>& image)
{
    image->removeClient(*this);
}

void DragImageLoader::imageChanged(CachedImage*, const IntRect*)
{
    m_dataTransfer->updateDragImage();
}

static DragOperation dragOpFromIEOp(const String& operation)
{
    if (operation == "uninitialized")
        return DragOperationEvery;
    if (operation == "none")
        return DragOperationNone;
    if (operation == "copy")
        return DragOperationCopy;
    if (operation == "link")
        return DragOperationLink;
    if (operation == "move")
        return (DragOperation)(DragOperationGeneric | DragOperationMove);
    if (operation == "copyLink")
        return (DragOperation)(DragOperationCopy | DragOperationLink);
    if (operation == "copyMove")
        return (DragOperation)(DragOperationCopy | DragOperationGeneric | DragOperationMove);
    if (operation == "linkMove")
        return (DragOperation)(DragOperationLink | DragOperationGeneric | DragOperationMove);
    if (operation == "all")
        return DragOperationEvery;
    return DragOperationPrivate; // really a marker for "no conversion"
}

static const char* IEOpFromDragOp(DragOperation operation)
{
    bool isGenericMove = operation & (DragOperationGeneric | DragOperationMove);

    if ((isGenericMove && (operation & DragOperationCopy) && (operation & DragOperationLink)) || operation == DragOperationEvery)
        return "all";
    if (isGenericMove && (operation & DragOperationCopy))
        return "copyMove";
    if (isGenericMove && (operation & DragOperationLink))
        return "linkMove";
    if ((operation & DragOperationCopy) && (operation & DragOperationLink))
        return "copyLink";
    if (isGenericMove)
        return "move";
    if (operation & DragOperationCopy)
        return "copy";
    if (operation & DragOperationLink)
        return "link";
    return "none";
}

DragOperation DataTransfer::sourceOperation() const
{
    DragOperation operation = dragOpFromIEOp(m_effectAllowed);
    ASSERT(operation != DragOperationPrivate);
    return operation;
}

DragOperation DataTransfer::destinationOperation() const
{
    DragOperation operation = dragOpFromIEOp(m_dropEffect);
    ASSERT(operation == DragOperationCopy || operation == DragOperationNone || operation == DragOperationLink || operation == (DragOperation)(DragOperationGeneric | DragOperationMove) || operation == DragOperationEvery);
    return operation;
}

void DataTransfer::setSourceOperation(DragOperation operation)
{
    ASSERT_ARG(operation, operation != DragOperationPrivate);
    m_effectAllowed = IEOpFromDragOp(operation);
}

void DataTransfer::setDestinationOperation(DragOperation operation)
{
    ASSERT_ARG(operation, operation == DragOperationCopy || operation == DragOperationNone || operation == DragOperationLink || operation == DragOperationGeneric || operation == DragOperationMove || operation == (DragOperation)(DragOperationGeneric | DragOperationMove));
    m_dropEffect = IEOpFromDragOp(operation);
}

String DataTransfer::dropEffect() const
{
    return m_dropEffect == "uninitialized" ? ASCIILiteral("none") : m_dropEffect;
}

void DataTransfer::setDropEffect(const String& effect)
{
    if (!forDrag())
        return;

    if (effect != "none" && effect != "copy" && effect != "link" && effect != "move")
        return;

    // FIXME: The spec allows this in all circumstances. There is probably no value
    // in ignoring attempts to change it.
    if (!canReadTypes())
        return;

    m_dropEffect = effect;
}

String DataTransfer::effectAllowed() const
{
    return m_effectAllowed;
}

void DataTransfer::setEffectAllowed(const String& effect)
{
    if (!forDrag())
        return;

    // Ignore any attempts to set it to an unknown value.
    if (dragOpFromIEOp(effect) == DragOperationPrivate)
        return;

    if (!canWriteData())
        return;

    m_effectAllowed = effect;
}

void DataTransfer::moveDragState(Ref<DataTransfer>&& other)
{
    RELEASE_ASSERT(is<StaticPasteboard>(other->pasteboard()));
    // We clear the platform pasteboard here to ensure that the pasteboard doesn't contain any data
    // that may have been written before starting the drag, and after ending the last drag session.
    // After pushing the static pasteboard's contents to the platform, the pasteboard should only
    // contain data that was in the static pasteboard.
    m_pasteboard->clear();
    other->commitToPasteboard(*m_pasteboard);

    m_dropEffect = other->m_dropEffect;
    m_effectAllowed = other->m_effectAllowed;
    m_dragLocation = other->m_dragLocation;
    m_dragImage = other->m_dragImage;
    m_dragImageElement = WTFMove(other->m_dragImageElement);
    m_dragImageLoader = WTFMove(other->m_dragImageLoader);
    if (m_dragImageLoader)
        m_dragImageLoader->moveToDataTransfer(*this);
    m_fileList = WTFMove(other->m_fileList);
}

bool DataTransfer::hasDragImage() const
{
    return m_dragImage || m_dragImageElement;
}

#endif // ENABLE(DRAG_SUPPORT)

} // namespace WebCore
