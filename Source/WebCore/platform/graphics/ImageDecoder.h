/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

#pragma once

#include "DecodingOptions.h"
#include "ImageOrientation.h"
#include "ImageTypes.h"
#include "IntPoint.h"
#include "IntSize.h"
#include "NativeImage.h"
#include <wtf/Optional.h>
#include <wtf/Seconds.h>
#include <wtf/text/WTFString.h>
#include <wtf/ThreadSafeRefCounted.h>

namespace WebCore {

class SharedBuffer;

class ImageDecoder : public ThreadSafeRefCounted<ImageDecoder> {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static RefPtr<ImageDecoder> create(SharedBuffer&, const String& mimeType, AlphaOption, GammaAndColorProfileOption);
    virtual ~ImageDecoder() = default;

    enum class MediaType {
        Image,
        Video,
    };
    static bool supportsMediaType(MediaType);

    virtual size_t bytesDecodedToDetermineProperties() const = 0;

    virtual EncodedDataStatus encodedDataStatus() const = 0;
    virtual bool isSizeAvailable() const { return encodedDataStatus() >= EncodedDataStatus::SizeAvailable; }
    virtual IntSize size() const = 0;
    virtual size_t frameCount() const = 0;
    virtual RepetitionCount repetitionCount() const = 0;
    virtual String uti() const { return emptyString(); }
    virtual String filenameExtension() const = 0;
    virtual std::optional<IntPoint> hotSpot() const = 0;

    virtual IntSize frameSizeAtIndex(size_t, SubsamplingLevel = SubsamplingLevel::Default) const = 0;
    virtual bool frameIsCompleteAtIndex(size_t) const = 0;
    virtual ImageOrientation frameOrientationAtIndex(size_t) const = 0;

    virtual Seconds frameDurationAtIndex(size_t) const = 0;
    virtual bool frameHasAlphaAtIndex(size_t) const = 0;
    virtual bool frameAllowSubsamplingAtIndex(size_t) const = 0;
    virtual unsigned frameBytesAtIndex(size_t, SubsamplingLevel = SubsamplingLevel::Default) const = 0;

    virtual NativeImagePtr createFrameImageAtIndex(size_t, SubsamplingLevel = SubsamplingLevel::Default, const DecodingOptions& = DecodingMode::Synchronous) = 0;

    virtual void setExpectedContentSize(long long) { }
    virtual void setData(SharedBuffer&, bool allDataReceived) = 0;
    virtual bool isAllDataReceived() const = 0;
    virtual void clearFrameBufferCache(size_t) = 0;

protected:
    ImageDecoder() = default;
};

}
