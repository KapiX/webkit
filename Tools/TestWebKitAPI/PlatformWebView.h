/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef PlatformWebView_h
#define PlatformWebView_h

#if USE(CG)
#include <CoreGraphics/CGGeometry.h>
#endif

#if PLATFORM(MAC)
#include <objc/objc.h>
#endif

#if defined(__APPLE__) && !PLATFORM(GTK)
#ifdef __OBJC__
@class WKView;
@class NSWindow;
#else
class WKView;
class NSWindow;
#endif
typedef WKView *PlatformWKView;
typedef NSWindow *PlatformWindow;
#elif PLATFORM(GTK)
typedef WKViewRef PlatformWKView;
typedef GtkWidget *PlatformWindow;
#elif PLATFORM(HAIKU)
typedef BView* PlatformWKView;
typedef BWindow* PlatformWindow;
#elif PLATFORM(WPE)
class HeadlessViewBackend;
struct wpe_view_backend;
typedef WKViewRef PlatformWKView;
typedef HeadlessViewBackend *PlatformWindow;
#endif

namespace TestWebKitAPI {

class PlatformWebView {
public:
    explicit PlatformWebView(WKPageConfigurationRef);
    explicit PlatformWebView(WKContextRef, WKPageGroupRef = 0);
    explicit PlatformWebView(WKPageRef relatedPage);
#if PLATFORM(MAC)
    explicit PlatformWebView(WKContextRef, WKPageGroupRef, Class wkViewSubclass);
#endif
    ~PlatformWebView();

    WKPageRef page() const;
    PlatformWKView platformView() const { return m_view; }
    void resizeTo(unsigned width, unsigned height);
    void focus();

    void simulateSpacebarKeyPress();
    void simulateAltKeyPress();
    void simulateRightClick(unsigned x, unsigned y);
    void simulateMouseMove(unsigned x, unsigned y, WKEventModifiers = 0);
#if PLATFORM(MAC)
    void simulateButtonClick(WKEventMouseButton, unsigned x, unsigned y, WKEventModifiers);
#endif

private:
#if PLATFORM(MAC)
    void initialize(WKPageConfigurationRef, Class wkViewSubclass);
#elif PLATFORM(GTK) || PLATFORM(WPE)
    void initialize(WKPageConfigurationRef);
#endif

    PlatformWKView m_view;
    PlatformWindow m_window;
#if PLATFORM(WPE)
    struct wpe_view_backend* m_backend { nullptr };
#endif
};

} // namespace TestWebKitAPI

#endif // PlatformWebView_h
