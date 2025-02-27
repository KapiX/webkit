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

#include "CacheStorageConnection.h"
#include "Document.h"
#include "Page.h"
#include "ServiceWorkerDebuggable.h"
#include "ServiceWorkerIdentifier.h"
#include "ServiceWorkerInspectorProxy.h"
#include "ServiceWorkerThread.h"
#include "WorkerDebuggerProxy.h"
#include "WorkerLoaderProxy.h"
#include <wtf/HashMap.h>

namespace WebCore {

class CacheStorageProvider;
class PageConfiguration;
class ServiceWorkerInspectorProxy;
struct ServiceWorkerContextData;

class ServiceWorkerThreadProxy final : public ThreadSafeRefCounted<ServiceWorkerThreadProxy>, public WorkerLoaderProxy, public WorkerDebuggerProxy {
public:
    template<typename... Args> static Ref<ServiceWorkerThreadProxy> create(Args&&... args)
    {
        return adoptRef(*new ServiceWorkerThreadProxy(std::forward<Args>(args)...));
    }

    ServiceWorkerIdentifier identifier() const { return m_serviceWorkerThread->identifier(); }
    ServiceWorkerThread& thread() { return m_serviceWorkerThread.get(); }
    ServiceWorkerInspectorProxy& inspectorProxy() { return m_inspectorProxy; }

    bool isTerminatingOrTerminated() const { return m_isTerminatingOrTerminated; }
    void setTerminatingOrTerminated(bool terminating) { m_isTerminatingOrTerminated = terminating; }

private:
    WEBCORE_EXPORT ServiceWorkerThreadProxy(PageConfiguration&&, const ServiceWorkerContextData&, PAL::SessionID, CacheStorageProvider&);

    // WorkerLoaderProxy
    bool postTaskForModeToWorkerGlobalScope(ScriptExecutionContext::Task&&, const String& mode) final;
    void postTaskToLoader(ScriptExecutionContext::Task&&) final;
    Ref<CacheStorageConnection> createCacheStorageConnection() final;

    // WorkerDebuggerProxy
    void postMessageToDebugger(const String&) final;
    void setResourceCachingDisabled(bool) final;

    UniqueRef<Page> m_page;
    Ref<Document> m_document;
    Ref<ServiceWorkerThread> m_serviceWorkerThread;
    CacheStorageProvider& m_cacheStorageProvider;
    RefPtr<CacheStorageConnection> m_cacheStorageConnection;
    PAL::SessionID m_sessionID;
    bool m_isTerminatingOrTerminated { false };

    ServiceWorkerInspectorProxy m_inspectorProxy;
#if ENABLE(REMOTE_INSPECTOR)
    std::unique_ptr<ServiceWorkerDebuggable> m_remoteDebuggable;
#endif
};

} // namespace WebKit

#endif // ENABLE(SERVICE_WORKER)
