/*
 * Copyright (C) 2009 Alex Milowski (alex@milowski.com). All rights reserved.
 * Copyright (C) 2010 François Sausset (sausset@gmail.com). All rights reserved.
 * Copyright (C) 2016 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if ENABLE(MATHML)

#include "MathMLFractionElement.h"
#include "RenderMathMLBlock.h"

namespace WebCore {

class MathMLFractionElement;

class RenderMathMLFraction final : public RenderMathMLBlock {
    WTF_MAKE_ISO_ALLOCATED(RenderMathMLFraction);
public:
    RenderMathMLFraction(MathMLFractionElement&, RenderStyle&&);

    float relativeLineThickness() const { return m_defaultLineThickness ? m_lineThickness / m_defaultLineThickness : LayoutUnit(0); }

private:
    bool isRenderMathMLFraction() const final { return true; }
    const char* renderName() const final { return "RenderMathMLFraction"; }

    void computePreferredLogicalWidths() final;
    void layoutBlock(bool relayoutChildren, LayoutUnit pageLogicalHeight = 0) final;
    std::optional<int> firstLineBaseline() const final;
    void paint(PaintInfo&, const LayoutPoint&) final;
    RenderMathMLOperator* unembellishedOperator() final;

    MathMLFractionElement& element() const { return static_cast<MathMLFractionElement&>(nodeForNonAnonymous()); }

    bool isStack() const { return !m_lineThickness; }
    bool isValid() const;
    RenderBox& numerator() const;
    RenderBox& denominator() const;
    LayoutUnit horizontalOffset(RenderBox&, MathMLFractionElement::FractionAlignment);
    void updateLineThickness();
    struct FractionParameters {
        LayoutUnit numeratorGapMin;
        LayoutUnit denominatorGapMin;
        LayoutUnit numeratorMinShiftUp;
        LayoutUnit denominatorMinShiftDown;
    };
    FractionParameters fractionParameters();
    struct StackParameters {
        LayoutUnit gapMin;
        LayoutUnit topShiftUp;
        LayoutUnit bottomShiftDown;
    };
    StackParameters stackParameters();

    LayoutUnit m_ascent;
    LayoutUnit m_defaultLineThickness { 1 };
    LayoutUnit m_lineThickness;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_RENDER_OBJECT(RenderMathMLFraction, isRenderMathMLFraction())

#endif // ENABLE(MATHML)
