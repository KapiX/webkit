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

#include "ServiceWorkerContextData.h"
#include "ServiceWorkerFetch.h"
#include "ServiceWorkerIdentifier.h"
#include "WorkerThread.h"

namespace WebCore {

class CacheStorageProvider;
class ContentSecurityPolicyResponseHeaders;
class ExtendableEvent;
class MessagePortChannel;
class SerializedScriptValue;
class WorkerObjectProxy;
struct ServiceWorkerClientData;
struct ServiceWorkerClientIdentifier;
struct ServiceWorkerContextData;

using MessagePortChannelArray = Vector<std::unique_ptr<MessagePortChannel>, 1>;

class ServiceWorkerThread : public WorkerThread {
public:
    template<typename... Args> static Ref<ServiceWorkerThread> create(Args&&... args)
    {
        return adoptRef(*new ServiceWorkerThread(std::forward<Args>(args)...));
    }
    virtual ~ServiceWorkerThread();

    WorkerObjectProxy& workerObjectProxy() const { return m_workerObjectProxy; }

    WEBCORE_EXPORT void postFetchTask(Ref<ServiceWorkerFetch::Client>&&, ResourceRequest&&, FetchOptions&&);
    WEBCORE_EXPORT void postFetchTask(Ref<ServiceWorkerFetch::Client>&&, std::optional<ServiceWorkerClientIdentifier>&&, ResourceRequest&&, FetchOptions&&);
    WEBCORE_EXPORT void postMessageToServiceWorker(Ref<SerializedScriptValue>&&, std::unique_ptr<MessagePortChannelArray>&&, ServiceWorkerClientIdentifier sourceIdentifier, ServiceWorkerClientData&& sourceData);
    WEBCORE_EXPORT void postMessageToServiceWorker(Ref<SerializedScriptValue>&&, std::unique_ptr<MessagePortChannelArray>&&, ServiceWorkerData&& sourceData);

    void fireInstallEvent();
    void fireActivateEvent();

    const ServiceWorkerContextData& contextData() const { return m_data; }

    ServiceWorkerIdentifier identifier() const { return m_data.serviceWorkerIdentifier; }

protected:
    Ref<WorkerGlobalScope> createWorkerGlobalScope(const URL&, const String& identifier, const String& userAgent, bool isOnline, const ContentSecurityPolicyResponseHeaders&, bool shouldBypassMainWorldContentSecurityPolicy, Ref<SecurityOrigin>&& topOrigin, MonotonicTime timeOrigin, PAL::SessionID) final;
    void runEventLoop() override;

private:
    WEBCORE_EXPORT ServiceWorkerThread(const ServiceWorkerContextData&, PAL::SessionID, WorkerLoaderProxy&, WorkerDebuggerProxy&);

    ServiceWorkerContextData m_data;
    WorkerObjectProxy& m_workerObjectProxy;
};

} // namespace WebCore

#endif // ENABLE(SERVICE_WORKER)
