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

#include "config.h"
#include "SWServerWorker.h"

#if ENABLE(SERVICE_WORKER)

#include <wtf/NeverDestroyed.h>

namespace WebCore {

static HashMap<ServiceWorkerIdentifier, SWServerWorker*>& allWorkers()
{
    static NeverDestroyed<HashMap<ServiceWorkerIdentifier, SWServerWorker*>> workers;
    return workers;
}

SWServerWorker* SWServerWorker::existingWorkerForIdentifier(ServiceWorkerIdentifier identifier)
{
    return allWorkers().get(identifier);
}

SWServerWorker::SWServerWorker(SWServer& server, SWServerRegistration& registration, SWServerToContextConnectionIdentifier contextConnectionIdentifier, const URL& scriptURL, const String& script, WorkerType type, ServiceWorkerIdentifier identifier)
    : m_server(server)
    , m_registrationKey(registration.key())
    , m_contextConnectionIdentifier(contextConnectionIdentifier)
    , m_data { identifier, scriptURL, ServiceWorkerState::Redundant, type, registration.identifier() }
    , m_script(script)
{
    auto result = allWorkers().add(identifier, this);
    ASSERT_UNUSED(result, result.isNewEntry);
}

SWServerWorker::~SWServerWorker()
{
    auto taken = allWorkers().take(identifier());
    ASSERT_UNUSED(taken, taken == this);
}

ServiceWorkerContextData SWServerWorker::contextData() const
{
    auto* registration = m_server.getRegistration(m_registrationKey);
    ASSERT(registration);

    return { std::nullopt, registration->data(), m_data.identifier, m_script, m_data.scriptURL, m_data.type };
}

void SWServerWorker::terminate()
{
    m_server.terminateWorker(*this);
}

const ClientOrigin& SWServerWorker::origin() const
{
    if (!m_origin)
        m_origin = ClientOrigin { m_registrationKey.topOrigin(), SecurityOriginData::fromSecurityOrigin(SecurityOrigin::create(m_data.scriptURL)) };

    return *m_origin;
}

void SWServerWorker::scriptContextFailedToStart(const std::optional<ServiceWorkerJobDataIdentifier>& jobDataIdentifier, const String& message)
{
    m_server.scriptContextFailedToStart(jobDataIdentifier, *this, message);
}

void SWServerWorker::scriptContextStarted(const std::optional<ServiceWorkerJobDataIdentifier>& jobDataIdentifier)
{
    m_server.scriptContextStarted(jobDataIdentifier, *this);
}

void SWServerWorker::didFinishInstall(const std::optional<ServiceWorkerJobDataIdentifier>& jobDataIdentifier, bool wasSuccessful)
{
    m_server.didFinishInstall(jobDataIdentifier, *this, wasSuccessful);
}

void SWServerWorker::didFinishActivation()
{
    m_server.didFinishActivation(*this);
}

void SWServerWorker::contextTerminated()
{
    m_server.workerContextTerminated(*this);
}

std::optional<ServiceWorkerClientData> SWServerWorker::findClientByIdentifier(ServiceWorkerClientIdentifier clientId)
{
    return m_server.findClientByIdentifier(origin(), clientId);
}

void SWServerWorker::matchAll(const ServiceWorkerClientQueryOptions& options, ServiceWorkerClientsMatchAllCallback&& callback)
{
    return m_server.matchAll(*this, options, WTFMove(callback));
}

void SWServerWorker::claim()
{
    return m_server.claim(*this);
}

void SWServerWorker::skipWaiting()
{
    m_isSkipWaitingFlagSet = true;

    auto* registration = m_server.getRegistration(m_registrationKey);
    ASSERT(registration);
    registration->tryActivate();
}

void SWServerWorker::setHasPendingEvents(bool hasPendingEvents)
{
    if (m_hasPendingEvents == hasPendingEvents)
        return;

    m_hasPendingEvents = hasPendingEvents;
    if (m_hasPendingEvents)
        return;

    // Do tryClear/tryActivate, as per https://w3c.github.io/ServiceWorker/#wait-until-method.
    auto* registration = m_server.getRegistration(m_registrationKey);
    if (!registration)
        return;

    if (registration->isUninstalling() && registration->tryClear())
        return;
    registration->tryActivate();
}

} // namespace WebCore

#endif // ENABLE(SERVICE_WORKER)
