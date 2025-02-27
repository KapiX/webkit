/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#include "config.h"
#include "NetworkResourceLoadParameters.h"

#include "ArgumentCoders.h"
#include "DataReference.h"
#include "WebCoreArgumentCoders.h"
#include <WebCore/SecurityOriginData.h>

using namespace WebCore;

namespace WebKit {

void NetworkResourceLoadParameters::encode(IPC::Encoder& encoder) const
{
    encoder << identifier;
    encoder << webPageID;
    encoder << webFrameID;
    encoder << sessionID;
    encoder << request;

    encoder << static_cast<bool>(request.httpBody());
    if (request.httpBody()) {
        request.httpBody()->encode(encoder);

        const Vector<FormDataElement>& elements = request.httpBody()->elements();
        size_t fileCount = 0;
        for (size_t i = 0, count = elements.size(); i < count; ++i) {
            if (elements[i].m_type == FormDataElement::Type::EncodedFile)
                ++fileCount;
        }

        SandboxExtension::HandleArray requestBodySandboxExtensions;
        requestBodySandboxExtensions.allocate(fileCount);
        size_t extensionIndex = 0;
        for (size_t i = 0, count = elements.size(); i < count; ++i) {
            const FormDataElement& element = elements[i];
            if (element.m_type == FormDataElement::Type::EncodedFile) {
                const String& path = element.m_shouldGenerateFile ? element.m_generatedFilename : element.m_filename;
                SandboxExtension::createHandle(path, SandboxExtension::Type::ReadOnly, requestBodySandboxExtensions[extensionIndex++]);
            }
        }
        encoder << requestBodySandboxExtensions;
    }

    if (request.url().isLocalFile()) {
        SandboxExtension::Handle requestSandboxExtension;
        SandboxExtension::createHandle(request.url().fileSystemPath(), SandboxExtension::Type::ReadOnly, requestSandboxExtension);
        encoder << requestSandboxExtension;
    }

    encoder.encodeEnum(contentSniffingPolicy);
    encoder.encodeEnum(contentEncodingSniffingPolicy);
    encoder.encodeEnum(storedCredentialsPolicy);
    encoder.encodeEnum(clientCredentialPolicy);
    encoder.encodeEnum(shouldPreconnectOnly);
    encoder << shouldFollowRedirects;
    encoder << shouldClearReferrerOnHTTPSToHTTPRedirect;
    encoder << defersLoading;
    encoder << needsCertificateInfo;
    encoder << maximumBufferingTime;
    encoder << derivedCachedDataTypesToRetrieve;

    encoder << static_cast<bool>(sourceOrigin);
    if (sourceOrigin)
        encoder << SecurityOriginData::fromSecurityOrigin(*sourceOrigin);
    encoder.encodeEnum(mode);
    encoder << cspResponseHeaders;

#if ENABLE(CONTENT_EXTENSIONS)
    encoder << mainDocumentURL;
    encoder << contentRuleLists;
#endif
}

bool NetworkResourceLoadParameters::decode(IPC::Decoder& decoder, NetworkResourceLoadParameters& result)
{
    if (!decoder.decode(result.identifier))
        return false;

    if (!decoder.decode(result.webPageID))
        return false;

    if (!decoder.decode(result.webFrameID))
        return false;

    if (!decoder.decode(result.sessionID))
        return false;

    if (!decoder.decode(result.request))
        return false;

    bool hasHTTPBody;
    if (!decoder.decode(hasHTTPBody))
        return false;

    if (hasHTTPBody) {
        RefPtr<FormData> formData = FormData::decode(decoder);
        if (!formData)
            return false;
        result.request.setHTTPBody(WTFMove(formData));

        SandboxExtension::HandleArray requestBodySandboxExtensionHandles;
        if (!decoder.decode(requestBodySandboxExtensionHandles))
            return false;
        for (size_t i = 0; i < requestBodySandboxExtensionHandles.size(); ++i) {
            if (auto extension = SandboxExtension::create(requestBodySandboxExtensionHandles[i]))
                result.requestBodySandboxExtensions.append(WTFMove(extension));
        }
    }

    if (result.request.url().isLocalFile()) {
        SandboxExtension::Handle resourceSandboxExtensionHandle;
        if (!decoder.decode(resourceSandboxExtensionHandle))
            return false;
        result.resourceSandboxExtension = SandboxExtension::create(resourceSandboxExtensionHandle);
    }

    if (!decoder.decodeEnum(result.contentSniffingPolicy))
        return false;
    if (!decoder.decodeEnum(result.contentEncodingSniffingPolicy))
        return false;
    if (!decoder.decodeEnum(result.storedCredentialsPolicy))
        return false;
    if (!decoder.decodeEnum(result.clientCredentialPolicy))
        return false;
    if (!decoder.decodeEnum(result.shouldPreconnectOnly))
        return false;
    if (!decoder.decode(result.shouldFollowRedirects))
        return false;
    if (!decoder.decode(result.shouldClearReferrerOnHTTPSToHTTPRedirect))
        return false;
    if (!decoder.decode(result.defersLoading))
        return false;
    if (!decoder.decode(result.needsCertificateInfo))
        return false;
    if (!decoder.decode(result.maximumBufferingTime))
        return false;
    if (!decoder.decode(result.derivedCachedDataTypesToRetrieve))
        return false;

    bool hasSourceOrigin;
    if (!decoder.decode(hasSourceOrigin))
        return false;
    if (hasSourceOrigin) {
        std::optional<SecurityOriginData> sourceOriginData;
        decoder >> sourceOriginData;
        if (!sourceOriginData)
            return false;
        ASSERT(!sourceOriginData->isEmpty());
        result.sourceOrigin = sourceOriginData->securityOrigin();
    }
    if (!decoder.decodeEnum(result.mode))
        return false;
    if (!decoder.decode(result.cspResponseHeaders))
        return false;

#if ENABLE(CONTENT_EXTENSIONS)
    if (!decoder.decode(result.mainDocumentURL))
        return false;

    std::optional<Vector<std::pair<String, WebCompiledContentRuleListData>>> contentRuleLists;
    decoder >> contentRuleLists;
    if (!contentRuleLists)
        return false;
    result.contentRuleLists = WTFMove(*contentRuleLists);
#endif

    return true;
}
    
} // namespace WebKit
