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

#import "config.h"
#import "RemoteNetworkingContext.h"

#import "CookieStorageUtilsCF.h"
#import "LegacyCustomProtocolManager.h"
#import "NetworkProcess.h"
#import "NetworkSession.h"
#import "NetworkSessionCreationParameters.h"
#import "SessionTracker.h"
#import "WebErrors.h"
#import "WebsiteDataStoreParameters.h"
#import <WebCore/NetworkStorageSession.h>
#import <WebCore/ResourceError.h>
#import <wtf/MainThread.h>

using namespace WebCore;

namespace WebKit {


RemoteNetworkingContext::~RemoteNetworkingContext()
{
}

bool RemoteNetworkingContext::isValid() const
{
    return true;
}

bool RemoteNetworkingContext::localFileContentSniffingEnabled() const
{
    return m_localFileContentSniffingEnabled;
}

NetworkStorageSession& RemoteNetworkingContext::storageSession() const
{
    if (auto session = NetworkStorageSession::storageSession(m_sessionID))
        return *session;
    // Some requests may still be coming shortly after NetworkProcess was told to destroy its session.
    LOG_ERROR("Invalid session ID. Please file a bug unless you just disabled private browsing, in which case it's an expected race.");
    return NetworkStorageSession::defaultStorageSession();
}

RetainPtr<CFDataRef> RemoteNetworkingContext::sourceApplicationAuditData() const
{
    return NetworkProcess::singleton().sourceApplicationAuditData();
}

String RemoteNetworkingContext::sourceApplicationIdentifier() const
{
    return SessionTracker::getIdentifierBase();
}

ResourceError RemoteNetworkingContext::blockedError(const ResourceRequest& request) const
{
    return WebKit::blockedError(request);
}

void RemoteNetworkingContext::ensureWebsiteDataStoreSession(WebsiteDataStoreParameters&& parameters)
{
    auto sessionID = parameters.networkSessionParameters.sessionID;
    if (NetworkStorageSession::storageSession(sessionID))
        return;

    String base;
    if (SessionTracker::getIdentifierBase().isNull())
        base = [[NSBundle mainBundle] bundleIdentifier];
    else
        base = SessionTracker::getIdentifierBase();

    if (!sessionID.isEphemeral())
        SandboxExtension::consumePermanently(parameters.cookieStoragePathExtensionHandle);

    RetainPtr<CFHTTPCookieStorageRef> uiProcessCookieStorage;
    if (!sessionID.isEphemeral() && !parameters.uiProcessCookieStorageIdentifier.isEmpty())
        uiProcessCookieStorage = cookieStorageFromIdentifyingData(parameters.uiProcessCookieStorageIdentifier);

    NetworkStorageSession::ensureSession(sessionID, base + '.' + String::number(sessionID.sessionID()), WTFMove(uiProcessCookieStorage));

    auto* session = NetworkStorageSession::storageSession(sessionID);
    for (const auto& cookie : parameters.pendingCookies)
        session->setCookie(cookie);

    if (!sessionID.isEphemeral() && !parameters.cacheStorageDirectory.isNull()) {
        SandboxExtension::consumePermanently(parameters.cacheStorageDirectoryExtensionHandle);
        session->setCacheStorageDirectory(WTFMove(parameters.cacheStorageDirectory));
        session->setCacheStoragePerOriginQuota(parameters.cacheStoragePerOriginQuota);
    }

#if USE(NETWORK_SESSION)
    parameters.networkSessionParameters.legacyCustomProtocolManager = NetworkProcess::singleton().supplement<LegacyCustomProtocolManager>();
    SessionTracker::setSession(sessionID, NetworkSession::create(WTFMove(parameters.networkSessionParameters)));
#endif
}

}
