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

#include "AnimationEffect.h"
#include "CSSPropertyBlendingClient.h"
#include "KeyframeList.h"
#include "RenderStyle.h"
#include <wtf/Ref.h>

namespace WebCore {

class Element;

class KeyframeEffect final : public AnimationEffect
    , public CSSPropertyBlendingClient {
public:
    static ExceptionOr<Ref<KeyframeEffect>> create(JSC::ExecState&, Element*, JSC::Strong<JSC::JSObject>&&);
    ~KeyframeEffect() { }

    Element* target() const { return m_target.get(); }
    ExceptionOr<void> setKeyframes(JSC::ExecState&, JSC::Strong<JSC::JSObject>&&);
    void getAnimatedStyle(std::unique_ptr<RenderStyle>& animatedStyle);
    void applyAtLocalTime(Seconds, RenderStyle&) override;
    void startOrStopAccelerated();
    bool isRunningAccelerated() const { return m_startedAccelerated; }

    RenderElement* renderer() const override;
    const RenderStyle& currentStyle() const override;
    bool isAccelerated() const override { return false; }
    bool filterFunctionListsMatch() const override { return false; }
    bool transformFunctionListsMatch() const override { return false; }
#if ENABLE(FILTERS_LEVEL_2)
    bool backdropFilterFunctionListsMatch() const override { return false; }
#endif

private:
    KeyframeEffect(Element*);
    ExceptionOr<void> processKeyframes(JSC::ExecState&, JSC::Strong<JSC::JSObject>&&);
    void computeStackingContextImpact();
    bool shouldRunAccelerated();

    RefPtr<Element> m_target;
    KeyframeList m_keyframes;
    bool m_triggersStackingContext { false };
    bool m_started { false };
    bool m_startedAccelerated { false };

};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_ANIMATION_EFFECT(KeyframeEffect, isKeyframeEffect());
