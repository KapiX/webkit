/*
 * Copyright (C) 2014-2017 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "InteractionInformationAtPosition.h"

#import "ArgumentCodersCF.h"
#import "WebCoreArgumentCoders.h"
#import <pal/spi/cocoa/DataDetectorsCoreSPI.h>
#import <pal/spi/cocoa/NSKeyedArchiverSPI.h>
#import <wtf/SoftLinking.h>

SOFT_LINK_PRIVATE_FRAMEWORK(DataDetectorsCore)
SOFT_LINK_CLASS(DataDetectorsCore, DDScannerResult)

namespace WebKit {

#if PLATFORM(IOS)

void InteractionInformationAtPosition::encode(IPC::Encoder& encoder) const
{
    encoder << request;

    encoder << nodeAtPositionIsAssistedNode;
#if ENABLE(DATA_INTERACTION)
    encoder << hasSelectionAtPosition;
#endif
    encoder << isSelectable;
    encoder << isNearMarkedText;
    encoder << touchCalloutEnabled;
    encoder << isLink;
    encoder << isImage;
    encoder << isAttachment;
    encoder << isAnimatedImage;
    encoder << isElement;
    encoder << adjustedPointForNodeRespondingToClickEvents;
    encoder << url;
    encoder << imageURL;
    encoder << title;
    encoder << idAttribute;
    encoder << bounds;
    encoder << textBefore;
    encoder << textAfter;
    encoder << linkIndicator;

    ShareableBitmap::Handle handle;
    if (image)
        image->createHandle(handle, SharedMemory::Protection::ReadOnly);
    encoder << handle;
#if ENABLE(DATA_DETECTION)
    encoder << isDataDetectorLink;
    if (isDataDetectorLink) {
        encoder << dataDetectorIdentifier;
#if (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED < 101200)
        RetainPtr<NSMutableData> data = adoptNS([[NSMutableData alloc] init]);
        auto archiver = secureArchiverFromMutableData(data.get());
        [archiver encodeObject:dataDetectorResults.get() forKey:@"dataDetectorResults"];
        [archiver finishEncoding];
        
        IPC::encode(encoder, reinterpret_cast<CFDataRef>(data.get()));
#else
        auto archiver = secureArchiver();
        [archiver encodeObject:dataDetectorResults.get() forKey:@"dataDetectorResults"];

        IPC::encode(encoder, reinterpret_cast<CFDataRef>(archiver.get().encodedData));
#endif
    }
#endif
}

bool InteractionInformationAtPosition::decode(IPC::Decoder& decoder, InteractionInformationAtPosition& result)
{
    if (!decoder.decode(result.request))
        return false;

    if (!decoder.decode(result.nodeAtPositionIsAssistedNode))
        return false;

#if ENABLE(DATA_INTERACTION)
    if (!decoder.decode(result.hasSelectionAtPosition))
        return false;
#endif

    if (!decoder.decode(result.isSelectable))
        return false;

    if (!decoder.decode(result.isNearMarkedText))
        return false;

    if (!decoder.decode(result.touchCalloutEnabled))
        return false;

    if (!decoder.decode(result.isLink))
        return false;

    if (!decoder.decode(result.isImage))
        return false;

    if (!decoder.decode(result.isAttachment))
        return false;
    
    if (!decoder.decode(result.isAnimatedImage))
        return false;
    
    if (!decoder.decode(result.isElement))
        return false;

    if (!decoder.decode(result.adjustedPointForNodeRespondingToClickEvents))
        return false;

    if (!decoder.decode(result.url))
        return false;

    if (!decoder.decode(result.imageURL))
        return false;

    if (!decoder.decode(result.title))
        return false;

    if (!decoder.decode(result.idAttribute))
        return false;
    
    if (!decoder.decode(result.bounds))
        return false;

    if (!decoder.decode(result.textBefore))
        return false;
    
    if (!decoder.decode(result.textAfter))
        return false;
    
    std::optional<WebCore::TextIndicatorData> linkIndicator;
    decoder >> linkIndicator;
    if (!linkIndicator)
        return false;
    result.linkIndicator = WTFMove(*linkIndicator);

    ShareableBitmap::Handle handle;
    if (!decoder.decode(handle))
        return false;

    if (!handle.isNull())
        result.image = ShareableBitmap::create(handle, SharedMemory::Protection::ReadOnly);

#if ENABLE(DATA_DETECTION)
    if (!decoder.decode(result.isDataDetectorLink))
        return false;
    
    if (result.isDataDetectorLink) {
        if (!decoder.decode(result.dataDetectorIdentifier))
            return false;
        RetainPtr<CFDataRef> data;
        if (!IPC::decode(decoder, data))
            return false;
        
        auto unarchiver = secureUnarchiverFromData((NSData *)data.get());
        @try {
            result.dataDetectorResults = [unarchiver decodeObjectOfClasses:[NSSet setWithArray:@[ [NSArray class], getDDScannerResultClass()] ] forKey:@"dataDetectorResults"];
        } @catch (NSException *exception) {
            LOG_ERROR("Failed to decode NSArray of DDScanResult: %@", exception);
            return false;
        }
        
        [unarchiver finishDecoding];
    }
#endif

    return true;
}

void InteractionInformationAtPosition::mergeCompatibleOptionalInformation(const InteractionInformationAtPosition& oldInformation)
{
    if (oldInformation.request.point != request.point)
        return;

    if (oldInformation.request.includeSnapshot && !request.includeSnapshot)
        image = oldInformation.image;

    if (oldInformation.request.includeLinkIndicator && !request.includeLinkIndicator)
        linkIndicator = oldInformation.linkIndicator;
}

#endif // PLATFORM(IOS)

}
