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

#import "config.h"

#if WK_API_ENABLED

#import <WebKit/WKBundleNodeHandlePrivate.h>
#import <WebKit/WKBundlePage.h>
#import <WebKit/WKBundlePageUIClient.h>
#import <WebKit/WKDOMDocument.h>
#import <WebKit/WKDOMElement.h>
#import <WebKit/WKDOMNodePrivate.h>
#import <WebKit/WKDOMText.h>
#import <WebKit/WKString.h>
#import <WebKit/WKWebProcessPlugIn.h>
#import <WebKit/WKWebProcessPlugInBrowserContextControllerPrivate.h>
#import <WebKit/WKWebProcessPlugInFrame.h>
#import <WebKit/WKWebProcessPlugInFramePrivate.h>
#import <WebKit/WKWebProcessPlugInNodeHandle.h>
#import <WebKit/WKWebProcessPlugInScriptWorld.h>
#import <wtf/RetainPtr.h>

void didClickAlternativePresentationButton(WKBundlePageRef, WKBundleNodeHandleRef, WKTypeRef* userData, const void *)
{
    *userData = WKStringCreateWithUTF8CString("user data string");
}

@interface ClickAlternativePresentationButton : NSObject <WKWebProcessPlugIn>
@end

@implementation ClickAlternativePresentationButton

- (void)webProcessPlugIn:(WKWebProcessPlugInController *)plugInController didCreateBrowserContextController:(WKWebProcessPlugInBrowserContextController *)browserContextController
{
    WKBundlePageUIClientV4 client;
    memset(&client, 0, sizeof(client));
    client.base.version = 4;
    client.didClickAlternativePresentationButton = didClickAlternativePresentationButton;
    WKBundlePageSetUIClient([browserContextController _bundlePageRef], &client.base);

    WKDOMDocument *document = [browserContextController mainFrameDocument];
    WKDOMElement *inputElement = [document createElement:@"input"];
    [[document body] appendChild:inputElement];

    auto *jsContext = [[browserContextController mainFrame] jsContextForWorld:[WKWebProcessPlugInScriptWorld normalWorld]];
    auto *jsValue = [jsContext evaluateScript:@"document.querySelector('input')"];

    RetainPtr<NSArray<WKWebProcessPlugInNodeHandle *>> elements = @[[WKWebProcessPlugInNodeHandle nodeHandleWithJSValue:jsValue inContext:jsContext]];
    [[browserContextController mainFrame] substituteElements:elements.get() withAlternativePresentationButtonWithIdentifier:@"1"];

    [jsContext evaluateScript:@"alert('ready for click!')"];
}

@end

#endif // WK_API_ENABLED
