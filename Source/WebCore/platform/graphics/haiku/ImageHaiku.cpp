/*
 * Copyright (C) 2006 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2007 Ryan Leavengood <leavengood@gmail.com>
 * Copyright (C) 2008 Andrea Anzani <andrea.anzani@gmail.com>
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved.
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
#include "Image.h"

#include "BitmapImage.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "ImageObserver.h"
#include "IntRect.h"
#include "NotImplemented.h"
#include "SharedBuffer.h"
#include "TransformationMatrix.h"
#include <Application.h>
#include <Bitmap.h>
#include <View.h>

// This function loads resources from WebKit
Vector<char> loadResourceIntoArray(const char*);


namespace WebCore {

bool FrameData::clear(bool clearMetadata)
{
    if (clearMetadata)
        m_haveMetadata = false;

    if (m_image) {
        m_image = nullptr;
        return true;
    }

    return false;
}

WTF::PassRefPtr<Image> Image::loadPlatformResource(const char* name)
{
    Vector<char> array = loadResourceIntoArray(name);
    WTF::RefPtr<BitmapImage> image = BitmapImage::create();
    image->setData(SharedBuffer::create(array.data(), array.size()), true);

    return image.release();
}

void BitmapImage::invalidatePlatformData()
{
}

// Drawing Routines
void BitmapImage::draw(GraphicsContext& ctxt, const FloatRect& dst, const FloatRect& src,
    CompositeOperator op, BlendMode, ImageOrientationDescription)
{
    if (!m_source.initialized())
        return;

    // Spin the animation to the correct frame before we try to draw it, so we
    // don't draw an old frame and then immediately need to draw a newer one,
    // causing flicker and wasting CPU.
    startAnimation();

    NativeImagePtr image = nativeImageForCurrentFrame();
    if (!image || !image->IsValid()) // If the image hasn't fully loaded.
        return;

    ctxt.save();
    ctxt.setCompositeOperation(op);

    BRect srcRect(src);
    BRect dstRect(dst);

    // Test using example site at
    // http://www.meyerweb.com/eric/css/edge/complexspiral/demo.html
    ctxt.platformContext()->SetDrawingMode(B_OP_ALPHA);
    uint32 options = 0;
    if (ctxt.imageInterpolationQuality() == InterpolationDefault
        || ctxt.imageInterpolationQuality() > InterpolationLow) {
        options |= B_FILTER_BITMAP_BILINEAR;
    }
    ctxt.platformContext()->DrawBitmapAsync(image.get(), srcRect, dstRect, options);
    ctxt.restore();

    if (imageObserver())
        imageObserver()->didDraw(this);
}

void Image::drawPattern(GraphicsContext& context, const FloatRect& tileRect,
    const AffineTransform& patternTransform, const FloatPoint& phase,
    const FloatSize& size, CompositeOperator op, const FloatRect& dstRect,
    BlendMode)
{
    NativeImagePtr image = nativeImageForCurrentFrame();
    if (!image || !image->IsValid()) // If the image hasn't fully loaded.
        return;

    // Figure out if the image has any alpha transparency, we can use faster drawing if not
    bool hasAlpha = false;

    uint8* bits = reinterpret_cast<uint8*>(image->Bits());
    uint32 width = image->Bounds().IntegerWidth() + 1;
    uint32 height = image->Bounds().IntegerHeight() + 1;

    uint32 bytesPerRow = image->BytesPerRow();
    for (uint32 y = 0; y < height && !hasAlpha; y++) {
        uint8* p = bits;
        for (uint32 x = 0; x < width && !hasAlpha; x++) {
            hasAlpha = p[3] < 255;
            p += 4;
        }
        bits += bytesPerRow;
    }

    context.save();
    if (hasAlpha)
        context.platformContext()->SetDrawingMode(B_OP_ALPHA);
    else
        context.platformContext()->SetDrawingMode(B_OP_COPY);

    context.clip(enclosingIntRect(dstRect));
    float currentW = phase.x();
    BRect bTileRect(tileRect);
    // FIXME app_server doesn't support B_TILE_BITMAP in DrawBitmap calls. This
    // would be much faster (#11196)
    while (currentW < dstRect.x() + dstRect.width()) {
        float currentH = phase.y();
        while (currentH < dstRect.y() + dstRect.height()) {
            BRect bDstRect(currentW, currentH, currentW + width - 1, currentH + height - 1);
            context.platformContext()->DrawBitmapAsync(image.get(), bTileRect, bDstRect);
            currentH += height;
        }
        currentW += width;
    }
    context.restore();

    if (imageObserver())
        imageObserver()->didDraw(this);
}

NativeImagePtr BitmapImage::getBBitmap()
{
    if (!haveFrameImageAtIndex(0))
		return NULL;

    return frameImageAtIndex(0);
}

NativeImagePtr BitmapImage::getFirstBBitmapOfSize(const IntSize& size)
{
    size_t count = frameCount();
    for (size_t i = 0; i < count; ++i) {
        NativeImagePtr bitmap = frameImageAtIndex(i);
        if (bitmap && bitmap->Bounds().IntegerWidth() + 1 == size.width()
            && bitmap->Bounds().IntegerHeight() + 1 == size.height()) {
            return bitmap;
        }
    }

    // Fallback to the default getBBitmap if we can't find the right size
    return getBBitmap();
}

} // namespace WebCore

