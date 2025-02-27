/*
 * Copyright (C) 2006, 2016 Apple Inc.  All rights reserved.
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

#import "config.h"
#import "ResourceResponse.h"

#if PLATFORM(COCOA)

#import "HTTPParsers.h"
#import "WebCoreURLResponse.h"
#import <Foundation/Foundation.h>
#import <limits>
#import <pal/spi/cf/CFNetworkSPI.h>
#import <wtf/AutodrainedPool.h>
#import <wtf/NeverDestroyed.h>
#import <wtf/StdLibExtras.h>
#import <wtf/text/StringView.h>

namespace WebCore {

void ResourceResponse::initNSURLResponse() const
{
    if (!m_httpStatusCode || !m_url.protocolIsInHTTPFamily()) {
        // Work around a mistake in the NSURLResponse class - <rdar://problem/6875219>.
        // The init function takes an NSInteger, even though the accessor returns a long long.
        // For values that won't fit in an NSInteger, pass -1 instead.
        NSInteger expectedContentLength;
        if (m_expectedContentLength < 0 || m_expectedContentLength > std::numeric_limits<NSInteger>::max())
            expectedContentLength = -1;
        else
            expectedContentLength = static_cast<NSInteger>(m_expectedContentLength);

        NSString* encodingNSString = nsStringNilIfEmpty(m_textEncodingName);
        m_nsResponse = adoptNS([[NSURLResponse alloc] initWithURL:m_url MIMEType:m_mimeType expectedContentLength:expectedContentLength textEncodingName:encodingNSString]);
        return;
    }

    // FIXME: We lose the status text and the HTTP version here.
    NSMutableDictionary* headerDictionary = [NSMutableDictionary dictionary];
    for (auto& header : m_httpHeaderFields)
        [headerDictionary setObject:(NSString *)header.value forKey:(NSString *)header.key];

    m_nsResponse = adoptNS([[NSHTTPURLResponse alloc] initWithURL:m_url statusCode:m_httpStatusCode HTTPVersion:(NSString*)kCFHTTPVersion1_1 headerFields:headerDictionary]);

    // Mime type sniffing doesn't work with a synthesized response.
    [m_nsResponse.get() _setMIMEType:(NSString *)m_mimeType];
}

void ResourceResponse::disableLazyInitialization()
{
    lazyInit(AllFields);
}

CertificateInfo ResourceResponse::platformCertificateInfo() const
{
    ASSERT(m_nsResponse || source() == Source::ServiceWorker);
    CFURLResponseRef cfResponse = [m_nsResponse _CFURLResponse];

    if (!cfResponse)
        return { };

    CFDictionaryRef context = _CFURLResponseGetSSLCertificateContext(cfResponse);
    if (!context)
        return { };

    auto trustValue = CFDictionaryGetValue(context, kCFStreamPropertySSLPeerTrust);
    if (!trustValue)
        return { };
    ASSERT(CFGetTypeID(trustValue) == SecTrustGetTypeID());
    auto trust = (SecTrustRef)trustValue;

    SecTrustResultType trustResultType;
    OSStatus result = SecTrustGetTrustResult(trust, &trustResultType);
    if (result != errSecSuccess)
        return { };

    if (trustResultType == kSecTrustResultInvalid) {
        result = SecTrustEvaluate(trust, &trustResultType);
        if (result != errSecSuccess)
            return { };
    }

#if HAVE(SEC_TRUST_SERIALIZATION)
    return CertificateInfo(trust);
#else
    return CertificateInfo(CertificateInfo::certificateChainFromSecTrust(trust));
#endif
}

static NSString* const commonHeaderFields[] = {
    @"Age", @"Cache-Control", @"Content-Type", @"Date", @"Etag", @"Expires", @"Last-Modified", @"Pragma"
};

NSURLResponse *ResourceResponse::nsURLResponse() const
{
    if (!m_nsResponse && !m_isNull)
        initNSURLResponse();
    return m_nsResponse.get();
}

static void addToHTTPHeaderMap(const void* key, const void* value, void* context)
{
    HTTPHeaderMap* httpHeaderMap = (HTTPHeaderMap*)context;
    httpHeaderMap->set((CFStringRef)key, (CFStringRef)value);
}

static inline AtomicString stripLeadingAndTrailingDoubleQuote(const String& value)
{
    unsigned length = value.length();
    if (length < 2 || value[0u] != '"' || value[length - 1] != '"')
        return value;

    return StringView(value).substring(1, length - 2).toAtomicString();
}

enum class OnlyCommonHeaders { No, Yes };
static inline void initializeHTTPHeaders(OnlyCommonHeaders onlyCommonHeaders, NSHTTPURLResponse *httpResponse, HTTPHeaderMap& headersMap)
{
    headersMap.clear();
    auto messageRef = CFURLResponseGetHTTPResponse([httpResponse _CFURLResponse]);

    // Avoid calling [NSURLResponse allHeaderFields] to minimize copying (<rdar://problem/26778863>).
    auto headers = adoptCF(CFHTTPMessageCopyAllHeaderFields(messageRef));
    if (onlyCommonHeaders == OnlyCommonHeaders::Yes) {
        for (auto& commonHeader : commonHeaderFields) {
            const void* value;
            if (CFDictionaryGetValueIfPresent(headers.get(), commonHeader, &value))
                headersMap.set(commonHeader, (CFStringRef) value);
        }
        return;
    }
    CFDictionaryApplyFunction(headers.get(), addToHTTPHeaderMap, &headersMap);
}

static inline AtomicString extractHTTPStatusText(CFHTTPMessageRef messageRef)
{
    if (auto httpStatusLine = adoptCF(CFHTTPMessageCopyResponseStatusLine(messageRef)))
        return extractReasonPhraseFromHTTPStatusLine(httpStatusLine.get());

    static NeverDestroyed<AtomicString> defaultStatusText("OK", AtomicString::ConstructFromLiteral);
    return defaultStatusText;
}

void ResourceResponse::platformLazyInit(InitLevel initLevel)
{
    ASSERT(initLevel >= CommonFieldsOnly);

    if (m_initLevel >= initLevel)
        return;

    if (m_isNull || !m_nsResponse)
        return;
    
    AutodrainedPool pool;

    NSHTTPURLResponse *httpResponse = [m_nsResponse.get() isKindOfClass:[NSHTTPURLResponse class]] ? (NSHTTPURLResponse *)m_nsResponse.get() : nullptr;

    if (m_initLevel < CommonFieldsOnly) {
        m_url = [m_nsResponse.get() URL];
        m_mimeType = [m_nsResponse.get() MIMEType];
        m_expectedContentLength = [m_nsResponse.get() expectedContentLength];
        // Stripping double quotes as a workaround for <rdar://problem/8757088>, can be removed once that is fixed.
        m_textEncodingName = stripLeadingAndTrailingDoubleQuote([m_nsResponse.get() textEncodingName]);
        m_httpStatusCode = httpResponse ? [httpResponse statusCode] : 0;
    }
    if (httpResponse) {
        if (initLevel == AllFields) {
            auto messageRef = CFURLResponseGetHTTPResponse([httpResponse _CFURLResponse]);
            m_httpStatusText = extractHTTPStatusText(messageRef);
            m_httpVersion = String(adoptCF(CFHTTPMessageCopyVersion(messageRef)).get()).convertToASCIIUppercase();
            initializeHTTPHeaders(OnlyCommonHeaders::No, httpResponse, m_httpHeaderFields);
        } else
            initializeHTTPHeaders(OnlyCommonHeaders::Yes, httpResponse, m_httpHeaderFields);
    }

    m_initLevel = initLevel;
}

String ResourceResponse::platformSuggestedFilename() const
{
    return [nsURLResponse() suggestedFilename];
}

bool ResourceResponse::platformCompare(const ResourceResponse& a, const ResourceResponse& b)
{
    return a.nsURLResponse() == b.nsURLResponse();
}

} // namespace WebCore

#endif // PLATFORM(COCOA)
