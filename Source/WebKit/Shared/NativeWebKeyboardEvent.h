/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
 * Copyright (C) 2011, 2012 Igalia S.L
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

#ifndef NativeWebKeyboardEvent_h
#define NativeWebKeyboardEvent_h

#include "WebEvent.h"

#if USE(APPKIT)
#include <wtf/RetainPtr.h>
OBJC_CLASS NSView;

namespace WebCore {
struct KeypressCommand;
}
#endif

#if PLATFORM(GTK)
#include "InputMethodFilter.h"
#include <WebCore/CompositionResults.h>
#include <WebCore/GUniquePtrGtk.h>
typedef union _GdkEvent GdkEvent;
#endif

#if PLATFORM(HAIKU)
#include <Message.h>
#endif

#if PLATFORM(IOS)
#include <wtf/RetainPtr.h>
OBJC_CLASS WebEvent;
#endif

#if PLATFORM(WPE)
struct wpe_input_keyboard_event;
#endif

namespace WebKit {

class NativeWebKeyboardEvent : public WebKeyboardEvent {
public:
#if USE(APPKIT)
    NativeWebKeyboardEvent(NSEvent *, bool handledByInputMethod, bool replacesSoftSpace, const Vector<WebCore::KeypressCommand>&);
#elif PLATFORM(GTK)
    NativeWebKeyboardEvent(const NativeWebKeyboardEvent&);
    NativeWebKeyboardEvent(GdkEvent*, const WebCore::CompositionResults&, InputMethodFilter::EventFakedForComposition, Vector<String>&& commands);
#elif PLATFORM(IOS)
    NativeWebKeyboardEvent(::WebEvent *);
#elif PLATFORM(WPE)
    NativeWebKeyboardEvent(struct wpe_input_keyboard_event*);
#endif

#if USE(APPKIT)
    NSEvent *nativeEvent() const { return m_nativeEvent.get(); }
#elif PLATFORM(GTK)
    GdkEvent* nativeEvent() const { return m_nativeEvent.get(); }
    const WebCore::CompositionResults& compositionResults() const  { return m_compositionResults; }
    bool isFakeEventForComposition() const { return m_fakeEventForComposition; }
#elif PLATFORM(HAIKU)
    const BMessage* nativeEvent() const { return m_nativeEvent; }
#elif PLATFORM(IOS)
    ::WebEvent* nativeEvent() const { return m_nativeEvent.get(); }
#elif PLATFORM(WPE)
    const void* nativeEvent() const { return nullptr; }
#endif

private:
#if USE(APPKIT)
    RetainPtr<NSEvent> m_nativeEvent;
#elif PLATFORM(GTK)
    GUniquePtr<GdkEvent> m_nativeEvent;
    WebCore::CompositionResults m_compositionResults;
    bool m_fakeEventForComposition;
#elif PLATFORM(HAIKU)
    BMessage* m_nativeEvent;
#elif PLATFORM(IOS)
    RetainPtr<::WebEvent> m_nativeEvent;
#endif
};

} // namespace WebKit

#endif // NativeWebKeyboardEvent_h
