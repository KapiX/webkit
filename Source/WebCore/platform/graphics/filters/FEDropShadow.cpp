/*
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "FEDropShadow.h"

#include "FEGaussianBlur.h"
#include "Filter.h"
#include "GraphicsContext.h"
#include "ShadowBlur.h"
#include <runtime/Uint8ClampedArray.h>
#include <wtf/MathExtras.h>
#include <wtf/text/TextStream.h>

namespace WebCore {
    
FEDropShadow::FEDropShadow(Filter& filter, float stdX, float stdY, float dx, float dy, const Color& shadowColor, float shadowOpacity)
    : FilterEffect(filter)
    , m_stdX(stdX)
    , m_stdY(stdY)
    , m_dx(dx)
    , m_dy(dy)
    , m_shadowColor(shadowColor)
    , m_shadowOpacity(shadowOpacity)
{
}

Ref<FEDropShadow> FEDropShadow::create(Filter& filter, float stdX, float stdY, float dx, float dy, const Color& shadowColor, float shadowOpacity)
{
    return adoptRef(*new FEDropShadow(filter, stdX, stdY, dx, dy, shadowColor, shadowOpacity));
}

void FEDropShadow::determineAbsolutePaintRect()
{
    Filter& filter = this->filter();

    FloatRect absolutePaintRect = inputEffect(0)->absolutePaintRect();
    FloatRect absoluteOffsetPaintRect(absolutePaintRect);
    absoluteOffsetPaintRect.move(filter.applyHorizontalScale(m_dx), filter.applyVerticalScale(m_dy));
    absolutePaintRect.unite(absoluteOffsetPaintRect);

    IntSize kernelSize = FEGaussianBlur::calculateKernelSize(filter, FloatPoint(m_stdX, m_stdY));

    // We take the half kernel size and multiply it with three, because we run box blur three times.
    absolutePaintRect.inflateX(3 * kernelSize.width() * 0.5f);
    absolutePaintRect.inflateY(3 * kernelSize.height() * 0.5f);

    if (clipsToBounds())
        absolutePaintRect.intersect(maxEffectRect());
    else
        absolutePaintRect.unite(maxEffectRect());

    setAbsolutePaintRect(enclosingIntRect(absolutePaintRect));
}

void FEDropShadow::platformApplySoftware()
{
    FilterEffect* in = inputEffect(0);

    ImageBuffer* resultImage = createImageBufferResult();
    if (!resultImage)
        return;

    Filter& filter = this->filter();
    FloatSize blurRadius(2 * filter.applyHorizontalScale(m_stdX), 2 * filter.applyVerticalScale(m_stdY));
    blurRadius.scale(filter.filterScale());
    FloatSize offset(filter.applyHorizontalScale(m_dx), filter.applyVerticalScale(m_dy));

    FloatRect drawingRegion = drawingRegionOfInputImage(in->absolutePaintRect());
    FloatRect drawingRegionWithOffset(drawingRegion);
    drawingRegionWithOffset.move(offset);

    ImageBuffer* sourceImage = in->imageBufferResult();
    if (!sourceImage)
        return;

    GraphicsContext& resultContext = resultImage->context();
    resultContext.setAlpha(m_shadowOpacity);
    resultContext.drawImageBuffer(*sourceImage, drawingRegionWithOffset);
    resultContext.setAlpha(1);

    ShadowBlur contextShadow(blurRadius, offset, m_shadowColor);

    // TODO: Direct pixel access to ImageBuffer would avoid copying the ImageData.
    IntRect shadowArea(IntPoint(), resultImage->internalSize());
    auto srcPixelArray = resultImage->getPremultipliedImageData(shadowArea, nullptr, ImageBuffer::BackingStoreCoordinateSystem);
    if (!srcPixelArray)
        return;

    contextShadow.blurLayerImage(srcPixelArray->data(), shadowArea.size(), 4 * shadowArea.size().width());

    resultImage->putByteArray(*srcPixelArray, AlphaPremultiplication::Premultiplied, shadowArea.size(), shadowArea, IntPoint(), ImageBuffer::BackingStoreCoordinateSystem);

    resultContext.setCompositeOperation(CompositeSourceIn);
    resultContext.fillRect(FloatRect(FloatPoint(), absolutePaintRect().size()), m_shadowColor);
    resultContext.setCompositeOperation(CompositeDestinationOver);

    resultImage->context().drawImageBuffer(*sourceImage, drawingRegion);
}

TextStream& FEDropShadow::externalRepresentation(TextStream& ts, RepresentationType representation) const
{
    ts << indent <<"[feDropShadow";
    FilterEffect::externalRepresentation(ts, representation);
    ts << " stdDeviation=\"" << m_stdX << ", " << m_stdY << "\" dx=\"" << m_dx << "\" dy=\"" << m_dy << "\" flood-color=\"" << m_shadowColor.nameForRenderTreeAsText() <<"\" flood-opacity=\"" << m_shadowOpacity << "]\n";

    TextStream::IndentScope indentScope(ts);
    inputEffect(0)->externalRepresentation(ts, representation);
    return ts;
}
    
} // namespace WebCore
