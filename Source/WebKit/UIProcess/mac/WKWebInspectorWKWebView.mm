/*
 * Copyright (C) 2010, 2014 Apple Inc. All rights reserved.
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

#import "config.h"
#import "WKWebInspectorWKWebView.h"

#if PLATFORM(MAC) && WK_API_ENABLED

#import "WKAPICast.h"
#import "WKInspectorPrivateMac.h"
#import "WKMutableArray.h"
#import "WKOpenPanelParametersRef.h"
#import "WKOpenPanelResultListener.h"
#import "WKRetainPtr.h"
#import "WKURLCF.h"
#import "WKWebViewInternal.h"
#import "WebPageProxy.h"

using namespace WebKit;

namespace WebKit {

static WKRect getWindowFrame(WKPageRef, const void* clientInfo)
{
    WKWebInspectorWKWebView *inspectorWKWebView = static_cast<WKWebInspectorWKWebView *>(const_cast<void*>(clientInfo));
    ASSERT(inspectorWKWebView);

    if (!inspectorWKWebView.window)
        return WKRectMake(0, 0, 0, 0);

    NSRect frame = inspectorWKWebView.frame;
    return WKRectMake(frame.origin.x, frame.origin.y, frame.size.width, frame.size.height);
}

static void setWindowFrame(WKPageRef, WKRect frame, const void* clientInfo)
{
    WKWebInspectorWKWebView *inspectorWKWebView = static_cast<WKWebInspectorWKWebView *>(const_cast<void*>(clientInfo));
    ASSERT(inspectorWKWebView);

    if (!inspectorWKWebView.window)
        return;

    [inspectorWKWebView.window setFrame:NSMakeRect(frame.origin.x, frame.origin.y, frame.size.width, frame.size.height) display:YES];
}

static unsigned long long exceededDatabaseQuota(WKPageRef, WKFrameRef, WKSecurityOriginRef, WKStringRef, WKStringRef, unsigned long long, unsigned long long, unsigned long long currentDatabaseUsage, unsigned long long expectedUsage, const void*)
{
    return std::max<unsigned long long>(expectedUsage, currentDatabaseUsage * 1.25);
}

static void runOpenPanel(WKPageRef page, WKFrameRef frame, WKOpenPanelParametersRef parameters, WKOpenPanelResultListenerRef listener, const void* clientInfo)
{
    WKWebInspectorWKWebView *inspectorWKWebView = static_cast<WKWebInspectorWKWebView *>(const_cast<void*>(clientInfo));
    ASSERT(inspectorWKWebView);

    NSOpenPanel *openPanel = [NSOpenPanel openPanel];
    [openPanel setAllowsMultipleSelection:WKOpenPanelParametersGetAllowsMultipleFiles(parameters)];

    WKRetain(listener);

    auto completionHandler = ^(NSInteger result) {
        if (result == NSModalResponseOK) {
            WKRetainPtr<WKMutableArrayRef> fileURLs = adoptWK(WKMutableArrayCreate());

            for (NSURL* nsURL in [openPanel URLs]) {
                WKRetainPtr<WKURLRef> wkURL = adoptWK(WKURLCreateWithCFURL(reinterpret_cast<CFURLRef>(nsURL)));
                WKArrayAppendItem(fileURLs.get(), wkURL.get());
            }

            WKOpenPanelResultListenerChooseFiles(listener, fileURLs.get());
        } else
            WKOpenPanelResultListenerCancel(listener);

        WKRelease(listener);
    };

    if (inspectorWKWebView.window)
        [openPanel beginSheetModalForWindow:inspectorWKWebView.window completionHandler:completionHandler];
    else
        completionHandler([openPanel runModal]);
}

} // namespace WebKit

@implementation WKWebInspectorWKWebView

- (instancetype)initWithFrame:(CGRect)frame configuration:(WKWebViewConfiguration *)configuration
{
    if (!(self = [super initWithFrame:frame configuration:configuration]))
        return nil;

    [self _setAutomaticallyAdjustsContentInsets:NO];

    WKPageUIClientV2 uiClient = {
        { 2, self },
        nullptr, // createNewPage_deprecatedForUseWithV0
        nullptr, // showPage
        nullptr, // closePage
        nullptr, // takeFocus
        nullptr, // focus
        nullptr, // unfocus
        nullptr, // runJavaScriptAlert
        nullptr, // runJavaScriptConfirm
        nullptr, // runJavaScriptPrompt
        nullptr, // setStatusText
        nullptr, // mouseDidMoveOverElement_deprecatedForUseWithV0
        nullptr, // missingPluginButtonClicked_deprecatedForUseWithV0
        nullptr, // didNotHandleKeyEvent
        nullptr, // didNotHandleWheelEvent
        nullptr, // areToolbarsVisible
        nullptr, // setToolbarsVisible
        nullptr, // isMenuBarVisible
        nullptr, // setMenuBarVisible
        nullptr, // isStatusBarVisible
        nullptr, // setStatusBarVisible
        nullptr, // isResizable
        nullptr, // setResizable
        getWindowFrame,
        setWindowFrame,
        nullptr, // runBeforeUnloadConfirmPanel
        nullptr, // didDraw
        nullptr, // pageDidScroll
        exceededDatabaseQuota,
        runOpenPanel,
        nullptr, // decidePolicyForGeolocationPermissionRequest
        nullptr, // headerHeight
        nullptr, // footerHeight
        nullptr, // drawHeader
        nullptr, // drawFooter
        nullptr, // printFrame
        nullptr, // runModal
        nullptr, // unused
        nullptr, // saveDataToFileInDownloadsFolder
        nullptr, // shouldInterruptJavaScript
        nullptr, // createPage
        nullptr, // mouseDidMoveOverElement
        nullptr, // decidePolicyForNotificationPermissionRequest
        nullptr, // unavailablePluginButtonClicked_deprecatedForUseWithV1
        nullptr, // showColorPicker
        nullptr, // hideColorPicker
        nullptr, // unavailablePluginButtonClicked
    };

    WebPageProxy* inspectorPage = self->_page.get();
    WKPageSetPageUIClient(toAPI(inspectorPage), &uiClient.base);

    return self;
}

- (NSInteger)tag
{
    return WKInspectorViewTag;
}

@end

#endif // PLATFORM(MAC)
