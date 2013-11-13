/*
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2007 Ryan Leavengood <leavengood@gmail.com>
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "Pasteboard.h"

#include "DocumentFragment.h"
#include "Editor.h"
#include "Frame.h"
#include "URL.h"
#include "NotImplemented.h"
#include "TextResourceDecoder.h"
#include "markup.h"
#include <support/Locker.h>
#include <Clipboard.h>
#include <Message.h>
#include <String.h>
#include <wtf/text/CString.h>


namespace WebCore {

PassOwnPtr<Pasteboard> Pasteboard::createForCopyAndPaste()
{
    return adoptPtr(new Pasteboard());
}

PassOwnPtr<Pasteboard> Pasteboard::createPrivate()
{
    return createForCopyAndPaste();
}

#if ENABLE(DRAG_SUPPORT)
PassOwnPtr<Pasteboard> Pasteboard::createForDragAndDrop()
{
    return createForCopyAndPaste();
}

PassOwnPtr<Pasteboard> Pasteboard::createForDragAndDrop(const DragData& dragData)
{
    return createForCopyAndPaste();
}
#endif

Pasteboard::Pasteboard()
{
}


// BClipboard unfortunately does not derive from BLocker, so we cannot use BAutolock.
class AutoClipboardLocker {
public:
    AutoClipboardLocker(BClipboard* clipboard)
        : m_clipboard(clipboard)
        , m_isLocked(clipboard && clipboard->Lock())
    {
    }

    ~AutoClipboardLocker()
    {
        if (m_isLocked)
            m_clipboard->Unlock();
    }

    bool isLocked() const
    {
        return m_isLocked;
    }

private:
    BClipboard* m_clipboard;
    bool m_isLocked;
};

bool Pasteboard::writeString(const String& type, const String& data)
{
    bool result = false;

    if (be_clipboard->Lock()) {
        BMessage* bdata = be_clipboard->Data();

        if (bdata) {
            bdata->RemoveName(BString(type).String());

            if (bdata->AddString(BString(type).String(), BString(data)) == B_OK)
                result = true;
        }

        be_clipboard->Commit();
        be_clipboard->Unlock();
    }

    return result;
}

void Pasteboard::writeSelection(Range* selectedRange, bool canSmartCopyOrDelete, Frame* frame, ShouldSerializeSelectedTextForClipboard)
{
    AutoClipboardLocker locker(be_clipboard);
    if (!locker.isLocked())
        return;

    be_clipboard->Clear();
    BMessage* data = be_clipboard->Data();
    if (!data)
        return;

    BString string(frame->editor().selectedText());

    // Replace unwanted representation of blank lines
    const char* utf8BlankLine = "\302\240\n";
    string.ReplaceAll(utf8BlankLine, "\n");

    data->AddData("text/plain", B_MIME_TYPE, string.String(), string.Length());

    BString markupString(createMarkup(selectedRange, 0, AnnotateForInterchange, false, ResolveNonLocalURLs));
    data->AddData("text/html", B_MIME_TYPE, markupString.String(), markupString.Length());

    be_clipboard->Commit();
}

void Pasteboard::writePlainText(const String& text, SmartReplaceOption smartReplaceOption)
{
    AutoClipboardLocker locker(be_clipboard);
    if (!locker.isLocked())
        return;

    be_clipboard->Clear();
    BMessage* data = be_clipboard->Data();
    if (!data)
        return;

    BString string(text);
    data->AddData("text/plain", B_MIME_TYPE, string.String(), string.Length());
    be_clipboard->Commit();
}

void Pasteboard::write(const PasteboardURL& url)
{
    ASSERT(!url.url.isEmpty());

    AutoClipboardLocker locker(be_clipboard);
    if (!locker.isLocked())
        return;

    be_clipboard->Clear();

    BMessage* data = be_clipboard->Data();
    if (!data)
        return;

    BString string(url.url.string());
    data->AddData("text/plain", B_MIME_TYPE, string.String(), string.Length());
    be_clipboard->Commit();
}

void Pasteboard::writeImage(Node*, const URL&, const String&)
{
    notImplemented();
}

bool Pasteboard::canSmartReplace()
{
    notImplemented();
    return false;
}

void Pasteboard::read(PasteboardPlainText& text)
{
    AutoClipboardLocker locker(be_clipboard);
    if (!locker.isLocked())
        return;

    BMessage* data = be_clipboard->Data();
    if (!data)
        return;

    const char* buffer = 0;
    ssize_t bufferLength;
    if (data->FindData("text/plain", B_MIME_TYPE, reinterpret_cast<const void**>(&buffer), &bufferLength) == B_OK)
        text.text = buffer;
}

PassRefPtr<DocumentFragment> Pasteboard::documentFragment(Frame* frame, PassRefPtr<Range> context,
                                                          bool allowPlainText, bool& chosePlainText)
{
    chosePlainText = false;

    AutoClipboardLocker locker(be_clipboard);
    if (!locker.isLocked())
        return 0;

    BMessage* data = be_clipboard->Data();
    if (!data)
        return 0;

    const char* buffer = 0;
    ssize_t bufferLength;
    if (data->FindData("text/html", B_MIME_TYPE, reinterpret_cast<const void**>(&buffer), &bufferLength) == B_OK) {
        RefPtr<TextResourceDecoder> decoder = TextResourceDecoder::create("text/plain", "UTF-8", true);
        String html = decoder->decode(buffer, bufferLength);
        html.append(decoder->flush());

        if (!html.isEmpty()) {
            RefPtr<DocumentFragment> fragment = createFragmentFromMarkup(frame->document(), html, "", DisallowScriptingContent);
            if (fragment)
                return fragment.release();
        }
    }

    if (!allowPlainText)
        return 0;

    if (data->FindData("text/plain", B_MIME_TYPE, reinterpret_cast<const void**>(&buffer), &bufferLength) == B_OK) {
        BString plainText(buffer, bufferLength);

        chosePlainText = true;
        RefPtr<DocumentFragment> fragment = createFragmentFromText(context.get(), plainText);
        if (fragment)
            return fragment.release();
    }

    return 0;
}


bool Pasteboard::hasData()
{
    bool result = false;

    if (be_clipboard->Lock()) {
        BMessage* data = be_clipboard->Data();

        if (data)
            result = !data->IsEmpty();

        be_clipboard->Unlock();
    }

    return result;
}

void Pasteboard::clear(const String& type)
{
    if (be_clipboard->Lock()) {
        BMessage* data = be_clipboard->Data();

        if (data) {
            data->RemoveName(BString(type).String());
            be_clipboard->Commit();
        }

        be_clipboard->Unlock();
    }
}

String Pasteboard::readString(const String& type)
{
    BString result;

    if (be_clipboard->Lock()) {
        BMessage* data = be_clipboard->Data();

        if (data)
            data->FindString(BString(type).String(), &result);

        be_clipboard->Unlock();
    }

    return result;
}

void Pasteboard::writePasteboard(const Pasteboard& sourcePasteboard)
{
    notImplemented();
}

void Pasteboard::clear()
{
    AutoClipboardLocker locker(be_clipboard);
    if (!locker.isLocked())
        return;

    be_clipboard->Clear();
    be_clipboard->Commit();
}

// Extensions beyond IE's API.
Vector<String> Pasteboard::types()
{
    Vector<String> result;

    if (be_clipboard->Lock()) {
        BMessage* data = be_clipboard->Data();

        if (data) {
            char* name;
            uint32 type;
            int32 count;

            for (int32 i = 0; data->GetInfo(B_ANY_TYPE, i, &name, &type, &count) == B_OK; i++)
                result.append(name);
        }

        be_clipboard->Unlock();
    }

    return result;
}

Vector<String> Pasteboard::readFilenames()
{
    notImplemented();
    return Vector<String>();
}

#if ENABLE(DRAG_SUPPORT)
void Pasteboard::setDragImage(BBitmap*, const IntPoint&)
{
    notImplemented();
}
#endif

} // namespace WebCore

