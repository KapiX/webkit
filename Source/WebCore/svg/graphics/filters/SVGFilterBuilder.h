/*
 * Copyright (C) 2008 Alex Mathews <possessedpenguinbob@gmail.com>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
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

#include "FilterEffect.h"
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/text/AtomicStringHash.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class RenderObject;

class SVGFilterBuilder {
public:
    typedef HashSet<FilterEffect*> FilterEffectSet;

    SVGFilterBuilder(RefPtr<FilterEffect> sourceGraphic);

    void add(const AtomicString& id, RefPtr<FilterEffect>);

    RefPtr<FilterEffect> getEffectById(const AtomicString& id) const;
    FilterEffect* lastEffect() const { return m_lastEffect.get(); }

    void appendEffectToEffectReferences(RefPtr<FilterEffect>&&, RenderObject*);

    inline FilterEffectSet& effectReferences(FilterEffect* effect)
    {
        // Only allowed for effects belongs to this builder.
        ASSERT(m_effectReferences.contains(effect));
        return m_effectReferences.find(effect)->value;
    }

    // Required to change the attributes of a filter during an svgAttributeChanged.
    inline FilterEffect* effectByRenderer(RenderObject* object) { return m_effectRenderer.get(object); }

    void clearEffects();
    void clearResultsRecursive(FilterEffect*);

private:
    inline void addBuiltinEffects()
    {
        for (auto& effect : m_builtinEffects.values())
            m_effectReferences.add(effect, FilterEffectSet());
    }

    HashMap<AtomicString, RefPtr<FilterEffect>> m_builtinEffects;
    HashMap<AtomicString, RefPtr<FilterEffect>> m_namedEffects;
    // The value is a list, which contains those filter effects,
    // which depends on the key filter effect.
    HashMap<RefPtr<FilterEffect>, FilterEffectSet> m_effectReferences;
    HashMap<RenderObject*, FilterEffect*> m_effectRenderer;

    RefPtr<FilterEffect> m_lastEffect;
};
    
} // namespace WebCore
