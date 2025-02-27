/*
 * Copyright (C) 2008 Alex Mathews <possessedpenguinbob@gmail.com>
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2010 Zoltan Herczeg <zherczeg@webkit.org>
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

#include "FloatPoint3D.h"
#include <wtf/RefCounted.h>

namespace WTF {
class TextStream;
}

namespace WebCore {

enum LightType {
    LS_DISTANT,
    LS_POINT,
    LS_SPOT
};

class LightSource : public RefCounted<LightSource> {
public:
    struct ComputedLightingData {
        FloatPoint3D lightVector;
        FloatPoint3D colorVector;
        float lightVectorLength;
    };

    struct PaintingData {
        ComputedLightingData intialLightingData;
        FloatPoint3D directionVector;
        float coneCutOffLimit;
        float coneFullLight;
        int specularExponent;
    };

    LightSource(LightType type)
        : m_type(type)
    { }

    virtual ~LightSource() = default;

    LightType type() const { return m_type; }
    virtual WTF::TextStream& externalRepresentation(WTF::TextStream&) const = 0;

    virtual void initPaintingData(PaintingData&) = 0;
    // z is a float number, since it is the alpha value scaled by a user
    // specified "surfaceScale" constant, which type is <number> in the SVG standard
    virtual ComputedLightingData computePixelLightingData(const PaintingData&, int x, int y, float z) const = 0;

    virtual bool setAzimuth(float) { return false; }
    virtual bool setElevation(float) { return false; }
    virtual bool setX(float) { return false; }
    virtual bool setY(float) { return false; }
    virtual bool setZ(float) { return false; }
    virtual bool setPointsAtX(float) { return false; }
    virtual bool setPointsAtY(float) { return false; }
    virtual bool setPointsAtZ(float) { return false; }
    virtual bool setSpecularExponent(float) { return false; }
    virtual bool setLimitingConeAngle(float) { return false; }

private:
    LightType m_type;
};

} // namespace WebCore

#define SPECIALIZE_TYPE_TRAITS_LIGHTSOURCE(ClassName, Type) \
SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::ClassName) \
    static bool isType(const WebCore::LightSource& source) { return source.type() == WebCore::Type; } \
SPECIALIZE_TYPE_TRAITS_END()
