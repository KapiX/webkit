/*
 * Copyright (C) 2010-2016 Apple Inc. All rights reserved.
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

#include "APIInjectedBundleBundleClient.h"
#include "APIObject.h"
#include "SandboxExtension.h"
#include <JavaScriptCore/JavaScript.h>
#include <WebCore/UserContentTypes.h>
#include <WebCore/UserScriptTypes.h>
#include <wtf/RefPtr.h>
#include <wtf/RetainPtr.h>
#include <wtf/text/WTFString.h>

#if USE(GLIB)
typedef struct _GModule GModule;
#endif

#if PLATFORM(HAIKU)
#include <kernel/image.h>
#endif

#if USE(FOUNDATION)
OBJC_CLASS NSBundle;
OBJC_CLASS NSMutableDictionary;
OBJC_CLASS WKWebProcessBundleParameters;
#endif

namespace API {
class Array;
class Data;
}

namespace IPC {
class Decoder;
class Connection;
class DataReference;
}

namespace WebKit {

#if USE(FOUNDATION)
typedef NSBundle *PlatformBundle;
#elif USE(GLIB)
typedef ::GModule* PlatformBundle;
#elif PLATFORM(HAIKU)
typedef image_id PlatformBundle;
#endif

class InjectedBundleScriptWorld;
class WebCertificateInfo;
class WebConnection;
class WebFrame;
class WebPage;
class WebPageGroupProxy;
struct WebProcessCreationParameters;

class InjectedBundle : public API::ObjectImpl<API::Object::Type::Bundle> {
public:
    static RefPtr<InjectedBundle> create(const WebProcessCreationParameters&, API::Object* initializationUserData);

    ~InjectedBundle();

    bool initialize(const WebProcessCreationParameters&, API::Object* initializationUserData);

    void setBundleParameter(const String&, const IPC::DataReference&);
    void setBundleParameters(const IPC::DataReference&);

    // API
    void setClient(std::unique_ptr<API::InjectedBundle::Client>&&);
    void postMessage(const String&, API::Object*);
    void postSynchronousMessage(const String&, API::Object*, RefPtr<API::Object>& returnData);

    WebConnection* webConnectionToUIProcess() const;

    // TestRunner only SPI
    void overrideBoolPreferenceForTestRunner(WebPageGroupProxy*, const String& preference, bool enabled);
    void setAllowUniversalAccessFromFileURLs(WebPageGroupProxy*, bool);
    void setAllowFileAccessFromFileURLs(WebPageGroupProxy*, bool);
    void setNeedsStorageAccessFromFileURLsQuirk(WebPageGroupProxy*, bool);
    void setMinimumLogicalFontSize(WebPageGroupProxy*, int size);
    void setFrameFlatteningEnabled(WebPageGroupProxy*, bool);
    void setAsyncFrameScrollingEnabled(WebPageGroupProxy*, bool);
    void setPluginsEnabled(WebPageGroupProxy*, bool);
    void setJavaScriptCanAccessClipboard(WebPageGroupProxy*, bool);
    void setPrivateBrowsingEnabled(WebPageGroupProxy*, bool);
    void setUseDashboardCompatibilityMode(WebPageGroupProxy*, bool);
    void setPopupBlockingEnabled(WebPageGroupProxy*, bool);
    void setAuthorAndUserStylesEnabled(WebPageGroupProxy*, bool);
    void setSpatialNavigationEnabled(WebPageGroupProxy*, bool);
    void addOriginAccessWhitelistEntry(const String&, const String&, const String&, bool);
    void removeOriginAccessWhitelistEntry(const String&, const String&, const String&, bool);
    void resetOriginAccessWhitelists();
    void setAsynchronousSpellCheckingEnabled(WebPageGroupProxy*, bool);
    int numberOfPages(WebFrame*, double, double);
    int pageNumberForElementById(WebFrame*, const String&, double, double);
    String pageSizeAndMarginsInPixels(WebFrame*, int, int, int, int, int, int, int);
    bool isPageBoxVisible(WebFrame*, int);
    void setUserStyleSheetLocation(WebPageGroupProxy*, const String&);
    void setWebNotificationPermission(WebPage*, const String& originString, bool allowed);
    void removeAllWebNotificationPermissions(WebPage*);
    uint64_t webNotificationID(JSContextRef, JSValueRef);
    Ref<API::Data> createWebDataFromUint8Array(JSContextRef, JSValueRef);

    // UserContent API
    void addUserScript(WebPageGroupProxy*, InjectedBundleScriptWorld*, String&& source, String&& url, API::Array* whitelist, API::Array* blacklist, WebCore::UserScriptInjectionTime, WebCore::UserContentInjectedFrames);
    void addUserStyleSheet(WebPageGroupProxy*, InjectedBundleScriptWorld*, const String& source, const String& url, API::Array* whitelist, API::Array* blacklist, WebCore::UserContentInjectedFrames);
    void removeUserScript(WebPageGroupProxy*, InjectedBundleScriptWorld*, const String& url);
    void removeUserStyleSheet(WebPageGroupProxy*, InjectedBundleScriptWorld*, const String& url);
    void removeUserScripts(WebPageGroupProxy*, InjectedBundleScriptWorld*);
    void removeUserStyleSheets(WebPageGroupProxy*, InjectedBundleScriptWorld*);
    void removeAllUserContent(WebPageGroupProxy*);

    // Garbage collection API
    void garbageCollectJavaScriptObjects();
    void garbageCollectJavaScriptObjectsOnAlternateThreadForDebugging(bool waitUntilDone);
    size_t javaScriptObjectsCount();

    // Callback hooks
    void didCreatePage(WebPage*);
    void willDestroyPage(WebPage*);
    void didInitializePageGroup(WebPageGroupProxy*);
    void didReceiveMessage(const String&, API::Object*);
    void didReceiveMessageToPage(WebPage*, const String&, API::Object*);

    static void reportException(JSContextRef, JSValueRef exception);

    static bool isProcessingUserGesture();

    void setTabKeyCyclesThroughElements(WebPage*, bool enabled);
    void setSerialLoadingEnabled(bool);
    void setCSSAnimationTriggersEnabled(bool);
    void setWebAnimationsEnabled(bool);
    void dispatchPendingLoadRequests();

#if PLATFORM(COCOA) && WK_API_ENABLED
    WKWebProcessBundleParameters *bundleParameters();
#endif

private:
    explicit InjectedBundle(const WebProcessCreationParameters&);

    String m_path;
    PlatformBundle m_platformBundle; // This is leaked right now, since we never unload the bundle/module.

    RefPtr<SandboxExtension> m_sandboxExtension;

    std::unique_ptr<API::InjectedBundle::Client> m_client;

#if PLATFORM(COCOA) && WK_API_ENABLED
    RetainPtr<WKWebProcessBundleParameters> m_bundleParameters;
#endif
};

} // namespace WebKit

