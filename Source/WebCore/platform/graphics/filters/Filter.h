/*
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#pragma once

#include "FloatSize.h"
#include "ImageBuffer.h"

namespace WebCore {

class Filter : public RefCounted<Filter> {
public:
    Filter(const AffineTransform& absoluteTransform, float filterScale = 1)
        : m_absoluteTransform(absoluteTransform)
        , m_filterScale(filterScale)
    { }
    virtual ~Filter() = default;

    void setSourceImage(std::unique_ptr<ImageBuffer> sourceImage) { m_sourceImage = WTFMove(sourceImage); }
    ImageBuffer* sourceImage() { return m_sourceImage.get(); }

    FloatSize filterResolution() const { return m_filterResolution; }
    void setFilterResolution(const FloatSize& filterResolution) { m_filterResolution = filterResolution; }

    float filterScale() const { return m_filterScale; }
    void setFilterScale(float scale) { m_filterScale = scale; }

    const AffineTransform& absoluteTransform() const { return m_absoluteTransform; }

    RenderingMode renderingMode() const { return m_renderingMode; }
    void setRenderingMode(RenderingMode renderingMode) { m_renderingMode = renderingMode; }

    virtual bool isSVGFilter() const { return false; }

    virtual float applyHorizontalScale(float value) const { return value * m_filterResolution.width(); }
    virtual float applyVerticalScale(float value) const { return value * m_filterResolution.height(); }
    
    virtual FloatRect sourceImageRect() const = 0;
    virtual FloatRect filterRegion() const = 0;

protected:
    explicit Filter(const FloatSize& filterResolution)
        : m_filterResolution(filterResolution)
    {
    }

private:
    std::unique_ptr<ImageBuffer> m_sourceImage;
    FloatSize m_filterResolution;
    AffineTransform m_absoluteTransform;
    RenderingMode m_renderingMode { Unaccelerated };
    float m_filterScale { 1 };
};

} // namespace WebCore
