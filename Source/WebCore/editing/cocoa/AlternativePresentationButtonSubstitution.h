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

#if ENABLE(ALTERNATIVE_PRESENTATION_BUTTON_ELEMENT)

#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/AtomicString.h>

namespace WebCore {

class AlternativePresentationButtonElement;
class Element;
class HTMLInputElement;
class StyledElement;

class AlternativePresentationButtonSubstitution {
public:
    AlternativePresentationButtonSubstitution(HTMLInputElement&, Vector<Ref<Element>>&& elementsToHide);
    AlternativePresentationButtonSubstitution(Element&, Vector<Ref<Element>>&& elementsToHide);

    void apply();
    void unapply();

    Vector<Ref<Element>> replacedElements();

private:
    void initializeSavedDisplayStyles(Vector<Ref<Element>>&&);

    Ref<Element> m_shadowHost;
    AtomicString m_savedShadowHostInputType;
    RefPtr<AlternativePresentationButtonElement> m_alternativePresentationButtonElement;

    struct InlineDisplayStyle {
        Ref<StyledElement> element;
        String value { };
        bool important { false };
        bool wasSpecified { false };
    };
    Vector<InlineDisplayStyle> m_savedDisplayStyles;

    bool m_isApplied { false };
};

} // namespace WebCore

#endif
