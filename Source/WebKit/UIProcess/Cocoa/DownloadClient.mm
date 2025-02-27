/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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
#import "DownloadClient.h"

#if WK_API_ENABLED

#import "AuthenticationChallengeProxy.h"
#import "AuthenticationDecisionListener.h"
#import "CompletionHandlerCallChecker.h"
#import "DownloadProxy.h"
#import "WKNSURLAuthenticationChallenge.h"
#import "WKNSURLExtras.h"
#import "WebCredential.h"
#import "_WKDownloadDelegate.h"
#import "_WKDownloadInternal.h"
#import <WebCore/ResourceError.h>
#import <WebCore/ResourceResponse.h>
#import <wtf/BlockPtr.h>

namespace WebKit {

static inline _WKDownload *wrapper(DownloadProxy& download)
{
    ASSERT([download.wrapper() isKindOfClass:[_WKDownload class]]);
    return (_WKDownload *)download.wrapper();
}

DownloadClient::DownloadClient(id <_WKDownloadDelegate> delegate)
    : m_delegate(delegate)
{
    m_delegateMethods.downloadDidStart = [delegate respondsToSelector:@selector(_downloadDidStart:)];
    m_delegateMethods.downloadDidReceiveResponse = [delegate respondsToSelector:@selector(_download:didReceiveResponse:)];
    m_delegateMethods.downloadDidReceiveData = [delegate respondsToSelector:@selector(_download:didReceiveData:)];
    m_delegateMethods.downloadDecideDestinationWithSuggestedFilenameAllowOverwrite = [delegate respondsToSelector:@selector(_download:decideDestinationWithSuggestedFilename:allowOverwrite:)];
    m_delegateMethods.downloadDecideDestinationWithSuggestedFilenameCompletionHandler = [delegate respondsToSelector:@selector(_download:decideDestinationWithSuggestedFilename:completionHandler:)];
    m_delegateMethods.downloadDidFinish = [delegate respondsToSelector:@selector(_downloadDidFinish:)];
    m_delegateMethods.downloadDidFail = [delegate respondsToSelector:@selector(_download:didFailWithError:)];
    m_delegateMethods.downloadDidCancel = [delegate respondsToSelector:@selector(_downloadDidCancel:)];
    m_delegateMethods.downloadDidReceiveServerRedirectToURL = [delegate respondsToSelector:@selector(_download:didReceiveServerRedirectToURL:)];
    m_delegateMethods.downloadDidReceiveAuthenticationChallengeCompletionHandler = [delegate respondsToSelector:@selector(_download:didReceiveAuthenticationChallenge:completionHandler:)];
    m_delegateMethods.downloadShouldDecodeSourceDataOfMIMEType = [delegate respondsToSelector:@selector(_download:shouldDecodeSourceDataOfMIMEType:)];
    m_delegateMethods.downloadDidCreateDestination = [delegate respondsToSelector:@selector(_download:didCreateDestination:)];
    m_delegateMethods.downloadProcessDidCrash = [delegate respondsToSelector:@selector(_downloadProcessDidCrash:)];
}

void DownloadClient::didStart(WebProcessPool&, DownloadProxy& downloadProxy)
{
    if (m_delegateMethods.downloadDidStart)
        [m_delegate _downloadDidStart:wrapper(downloadProxy)];
}

void DownloadClient::didReceiveResponse(WebProcessPool&, DownloadProxy& downloadProxy, const WebCore::ResourceResponse& response)
{
    if (m_delegateMethods.downloadDidReceiveResponse)
        [m_delegate _download:wrapper(downloadProxy) didReceiveResponse:response.nsURLResponse()];
}

void DownloadClient::didReceiveData(WebProcessPool&, DownloadProxy& downloadProxy, uint64_t length)
{
    if (m_delegateMethods.downloadDidReceiveData)
        [m_delegate _download:wrapper(downloadProxy) didReceiveData:length];
}

void DownloadClient::didReceiveAuthenticationChallenge(WebProcessPool&, DownloadProxy& downloadProxy, AuthenticationChallengeProxy& authenticationChallenge)
{
    if (!m_delegateMethods.downloadDidReceiveAuthenticationChallengeCompletionHandler) {
        authenticationChallenge.listener()->performDefaultHandling();
        return;
    }

    [m_delegate _download:wrapper(downloadProxy) didReceiveAuthenticationChallenge:wrapper(authenticationChallenge) completionHandler:BlockPtr<void(NSURLSessionAuthChallengeDisposition, NSURLCredential *)>::fromCallable([authenticationChallenge = makeRef(authenticationChallenge), checker = CompletionHandlerCallChecker::create(m_delegate.get().get(), @selector(_download:didReceiveAuthenticationChallenge:completionHandler:))] (NSURLSessionAuthChallengeDisposition disposition, NSURLCredential *credential) {
        if (checker->completionHandlerHasBeenCalled())
            return;
        checker->didCallCompletionHandler();
        switch (disposition) {
        case NSURLSessionAuthChallengeUseCredential: {
            RefPtr<WebCredential> webCredential;
            if (credential)
                webCredential = WebCredential::create(WebCore::Credential(credential));
            
            authenticationChallenge->listener()->useCredential(webCredential.get());
            break;
        }
            
        case NSURLSessionAuthChallengePerformDefaultHandling:
            authenticationChallenge->listener()->performDefaultHandling();
            break;
            
        case NSURLSessionAuthChallengeCancelAuthenticationChallenge:
            authenticationChallenge->listener()->cancel();
            break;
            
        case NSURLSessionAuthChallengeRejectProtectionSpace:
            authenticationChallenge->listener()->rejectProtectionSpaceAndContinue();
            break;
            
        default:
            [NSException raise:NSInvalidArgumentException format:@"Invalid NSURLSessionAuthChallengeDisposition (%ld)", (long)disposition];
        }
    }).get()];
}

#if !USE(NETWORK_SESSION)
bool DownloadClient::shouldDecodeSourceDataOfMIMEType(WebProcessPool&, DownloadProxy& downloadProxy, const String& mimeType)
{
    if (m_delegateMethods.downloadShouldDecodeSourceDataOfMIMEType)
        return [m_delegate _download:wrapper(downloadProxy) shouldDecodeSourceDataOfMIMEType:mimeType];
    return true;
}
#endif

void DownloadClient::didCreateDestination(WebProcessPool&, DownloadProxy& downloadProxy, const String& destination)
{
    if (m_delegateMethods.downloadDidCreateDestination)
        [m_delegate _download:wrapper(downloadProxy) didCreateDestination:destination];
}

void DownloadClient::processDidCrash(WebProcessPool&, DownloadProxy& downloadProxy)
{
    if (m_delegateMethods.downloadProcessDidCrash)
        [m_delegate _downloadProcessDidCrash:wrapper(downloadProxy)];
}

void DownloadClient::decideDestinationWithSuggestedFilename(WebProcessPool&, DownloadProxy& downloadProxy, const String& filename, Function<void(AllowOverwrite, String)>&& completionHandler)
{
    if (!m_delegateMethods.downloadDecideDestinationWithSuggestedFilenameAllowOverwrite && !m_delegateMethods.downloadDecideDestinationWithSuggestedFilenameCompletionHandler)
        return completionHandler(AllowOverwrite::No, { });

    if (m_delegateMethods.downloadDecideDestinationWithSuggestedFilenameAllowOverwrite) {
        BOOL allowOverwrite = NO;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        NSString *destination = [m_delegate _download:wrapper(downloadProxy) decideDestinationWithSuggestedFilename:filename allowOverwrite:&allowOverwrite];
#pragma clang diagnostic pop
        completionHandler(allowOverwrite ? AllowOverwrite::Yes : AllowOverwrite::No, destination);
    } else {
        [m_delegate _download:wrapper(downloadProxy) decideDestinationWithSuggestedFilename:filename completionHandler:BlockPtr<void(BOOL, NSString *)>::fromCallable([checker = CompletionHandlerCallChecker::create(m_delegate.get().get(), @selector(_download:decideDestinationWithSuggestedFilename:completionHandler:)), completionHandler = WTFMove(completionHandler)] (BOOL allowOverwrite, NSString *destination) {
            if (checker->completionHandlerHasBeenCalled())
                return;
            checker->didCallCompletionHandler();
            completionHandler(allowOverwrite ? AllowOverwrite::Yes : AllowOverwrite::No, destination);
        }).get()];
    }
}

void DownloadClient::didFinish(WebProcessPool&, DownloadProxy& downloadProxy)
{
    if (m_delegateMethods.downloadDidFinish)
        [m_delegate _downloadDidFinish:wrapper(downloadProxy)];
}

void DownloadClient::didFail(WebProcessPool&, DownloadProxy& downloadProxy, const WebCore::ResourceError& error)
{
    if (m_delegateMethods.downloadDidFail)
        [m_delegate _download:wrapper(downloadProxy) didFailWithError:error.nsError()];
}

void DownloadClient::didCancel(WebProcessPool&, DownloadProxy& downloadProxy)
{
    if (m_delegateMethods.downloadDidCancel)
        [m_delegate _downloadDidCancel:wrapper(downloadProxy)];
}

void DownloadClient::willSendRequest(WebProcessPool&, DownloadProxy& downloadProxy, WebCore::ResourceRequest&& request, const WebCore::ResourceResponse&, CompletionHandler<void(WebCore::ResourceRequest&&)>&& completionHandler)
{
    if (m_delegateMethods.downloadDidReceiveServerRedirectToURL)
        [m_delegate _download:wrapper(downloadProxy) didReceiveServerRedirectToURL:[NSURL _web_URLWithWTFString:request.url().string()]];

    completionHandler(WTFMove(request));
}

} // namespace WebKit

#endif // WK_API_ENABLED
