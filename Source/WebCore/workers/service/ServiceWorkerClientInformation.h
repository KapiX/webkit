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

#if ENABLE(SERVICE_WORKER)

#include "ServiceWorkerClientData.h"
#include "ServiceWorkerClientIdentifier.h"
#include <wtf/Vector.h>

namespace WebCore {

class ScriptExecutionContext;

struct ServiceWorkerClientInformation {
    ServiceWorkerClientIdentifier identifier;
    ServiceWorkerClientData data;

    ServiceWorkerClientInformation isolatedCopy() const { return { identifier, data.isolatedCopy() }; }

    template<class Encoder> void encode(Encoder&) const;
    template<class Decoder> static std::optional<ServiceWorkerClientInformation> decode(Decoder&);
};

using ServiceWorkerClientsMatchAllCallback = WTF::CompletionHandler<void(Vector<ServiceWorkerClientInformation>&&)>;

template<class Encoder>
void ServiceWorkerClientInformation::encode(Encoder& encoder) const
{
    encoder << identifier << data;
}

template<class Decoder>
std::optional<ServiceWorkerClientInformation> ServiceWorkerClientInformation::decode(Decoder& decoder)
{
    std::optional<ServiceWorkerClientIdentifier> identifier;
    decoder >> identifier;
    if (!identifier)
        return std::nullopt;

    std::optional<ServiceWorkerClientData> data;
    decoder >> data;
    if (!data)
        return std::nullopt;

    return { { WTFMove(*identifier), WTFMove(*data) } };
}

} // namespace WebCore

#endif // ENABLE(SERVICE_WORKER)
