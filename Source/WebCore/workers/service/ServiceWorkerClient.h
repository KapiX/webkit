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

#include "ContextDestructionObserver.h"
#include "ExceptionOr.h"
#include "ServiceWorkerClientData.h"
#include <heap/Strong.h>
#include <wtf/RefCounted.h>

namespace JSC {
class JSValue;
}

namespace WebCore {

class ServiceWorkerGlobalScope;

class ServiceWorkerClient : public RefCounted<ServiceWorkerClient>, public ContextDestructionObserver {
public:
    using Identifier = ServiceWorkerClientIdentifier;

    using Type = ServiceWorkerClientType;
    using FrameType = ServiceWorkerClientFrameType;

    static Ref<ServiceWorkerClient> getOrCreate(ServiceWorkerGlobalScope&, ServiceWorkerClientIdentifier, ServiceWorkerClientData&&);

    ~ServiceWorkerClient();

    const URL& url() const;
    FrameType frameType() const;
    Type type() const;
    String id() const;

    Identifier identifier() const { return m_identifier; }

    ExceptionOr<void> postMessage(ScriptExecutionContext&, JSC::JSValue message, Vector<JSC::Strong<JSC::JSObject>>&& transfer);

protected:
    ServiceWorkerClient(ServiceWorkerGlobalScope&, ServiceWorkerClientIdentifier, ServiceWorkerClientData&&);

    ServiceWorkerClientIdentifier m_identifier;
    ServiceWorkerClientData m_data;
};

} // namespace WebCore

#endif // ENABLE(SERVICE_WORKER)
