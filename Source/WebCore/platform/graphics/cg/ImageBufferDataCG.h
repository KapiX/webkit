/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Google Inc. All rights reserved.
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

#pragma once

#include "Image.h"
#include <runtime/Uint8ClampedArray.h>
#include <wtf/CheckedArithmetic.h>
#include <wtf/RefPtr.h>
#include <wtf/RetainPtr.h>
#include <wtf/Vector.h>

#if PLATFORM(COCOA) && USE(CA) && !PLATFORM(IOS_SIMULATOR)
#define USE_IOSURFACE_CANVAS_BACKING_STORE 1
#endif

typedef struct CGColorSpace *CGColorSpaceRef;
typedef struct CGDataProvider *CGDataProviderRef;
typedef uint32_t CGBitmapInfo;

namespace WebCore {

class IOSurface;

struct ImageBufferData {
    IntSize backingStoreSize;
    Checked<unsigned, RecordOverflow> bytesPerRow;
    CGColorSpaceRef colorSpace;

    // Only for software ImageBuffers.
    void* data { nullptr };
    RetainPtr<CGDataProviderRef> dataProvider;
    CGBitmapInfo bitmapInfo;
    std::unique_ptr<GraphicsContext> context;

#if USE(IOSURFACE_CANVAS_BACKING_STORE)
    // Only for accelerated ImageBuffers.
    std::unique_ptr<IOSurface> surface;
#endif

    Vector<uint8_t> toBGRAData(bool accelerateRendering, int width, int height) const;
    RefPtr<Uint8ClampedArray> getData(AlphaPremultiplication, const IntRect&, const IntSize&, bool accelerateRendering, float resolutionScale) const;
    void putData(const Uint8ClampedArray& source, AlphaPremultiplication sourceFormat, const IntSize& sourceSize, const IntRect& sourceRect, const IntPoint& destPoint, const IntSize&, bool accelerateRendering, float resolutionScale);
};

} // namespace WebCore
