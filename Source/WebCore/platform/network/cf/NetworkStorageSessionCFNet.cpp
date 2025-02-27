/*
 * Copyright (C) 2012-2017 Apple Inc. All rights reserved.
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
#include "NetworkStorageSession.h"

#include <wtf/MainThread.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/ProcessID.h>

#if PLATFORM(COCOA)
#include "PublicSuffix.h"
#include "ResourceRequest.h"
#else
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#endif

namespace WebCore {

static bool cookieStoragePartitioningEnabled;
static bool storageAccessAPIEnabled;

static RetainPtr<CFURLStorageSessionRef> createCFStorageSessionForIdentifier(CFStringRef identifier)
{
    auto storageSession = adoptCF(_CFURLStorageSessionCreate(kCFAllocatorDefault, identifier, nullptr));

    if (!storageSession)
        return nullptr;

    auto cache = adoptCF(_CFURLStorageSessionCopyCache(kCFAllocatorDefault, storageSession.get()));
    if (!cache)
        return nullptr;

    CFURLCacheSetDiskCapacity(cache.get(), 0);

    auto sharedCache = adoptCF(CFURLCacheCopySharedURLCache());
    CFURLCacheSetMemoryCapacity(cache.get(), CFURLCacheMemoryCapacity(sharedCache.get()));

    auto cookieStorage = adoptCF(_CFURLStorageSessionCopyCookieStorage(kCFAllocatorDefault, storageSession.get()));
    if (!cookieStorage)
        return nullptr;

    auto sharedCookieStorage = _CFHTTPCookieStorageGetDefault(kCFAllocatorDefault);
    auto sharedPolicy = CFHTTPCookieStorageGetCookieAcceptPolicy(sharedCookieStorage);
    CFHTTPCookieStorageSetCookieAcceptPolicy(cookieStorage.get(), sharedPolicy);

    return storageSession;
}

NetworkStorageSession::NetworkStorageSession(PAL::SessionID sessionID, RetainPtr<CFURLStorageSessionRef>&& platformSession, RetainPtr<CFHTTPCookieStorageRef>&& platformCookieStorage)
    : m_sessionID(sessionID)
    , m_platformSession(WTFMove(platformSession))
{
    m_platformCookieStorage = platformCookieStorage ? WTFMove(platformCookieStorage) : cookieStorage();
}

static std::unique_ptr<NetworkStorageSession>& defaultNetworkStorageSession()
{
    ASSERT(isMainThread());
    static NeverDestroyed<std::unique_ptr<NetworkStorageSession>> session;
    return session;
}

void NetworkStorageSession::switchToNewTestingSession()
{
    // Session name should be short enough for shared memory region name to be under the limit, otehrwise sandbox rules won't work (see <rdar://problem/13642852>).
    String sessionName = String::format("WebKit Test-%u", static_cast<uint32_t>(getCurrentProcessID()));

    RetainPtr<CFURLStorageSessionRef> session;
#if PLATFORM(COCOA)
    session = adoptCF(createPrivateStorageSession(sessionName.createCFString().get()));
#else
    session = adoptCF(wkCreatePrivateStorageSession(sessionName.createCFString().get(), defaultStorageSession().platformSession()));
#endif

    RetainPtr<CFHTTPCookieStorageRef> cookieStorage;
    if (session)
        cookieStorage = adoptCF(_CFURLStorageSessionCopyCookieStorage(kCFAllocatorDefault, session.get()));

    defaultNetworkStorageSession() = std::make_unique<NetworkStorageSession>(PAL::SessionID::defaultSessionID(), WTFMove(session), WTFMove(cookieStorage));
}

NetworkStorageSession& NetworkStorageSession::defaultStorageSession()
{
    if (!defaultNetworkStorageSession())
        defaultNetworkStorageSession() = std::make_unique<NetworkStorageSession>(PAL::SessionID::defaultSessionID(), nullptr, nullptr);
    return *defaultNetworkStorageSession();
}

void NetworkStorageSession::ensureSession(PAL::SessionID sessionID, const String& identifierBase, RetainPtr<CFHTTPCookieStorageRef>&& cookieStorage)
{
    auto addResult = globalSessionMap().add(sessionID, nullptr);
    if (!addResult.isNewEntry)
        return;

    RetainPtr<CFStringRef> cfIdentifier = String(identifierBase + ".PrivateBrowsing").createCFString();

    RetainPtr<CFURLStorageSessionRef> storageSession;
    if (sessionID.isEphemeral()) {
#if PLATFORM(COCOA)
        storageSession = adoptCF(createPrivateStorageSession(cfIdentifier.get()));
#else
        storageSession = adoptCF(wkCreatePrivateStorageSession(cfIdentifier.get(), defaultNetworkStorageSession()->platformSession()));
#endif
    } else
        storageSession = createCFStorageSessionForIdentifier(cfIdentifier.get());

    if (!cookieStorage && storageSession)
        cookieStorage = adoptCF(_CFURLStorageSessionCopyCookieStorage(kCFAllocatorDefault, storageSession.get()));

    addResult.iterator->value = std::make_unique<NetworkStorageSession>(sessionID, WTFMove(storageSession), WTFMove(cookieStorage));
}

void NetworkStorageSession::ensureSession(PAL::SessionID sessionID, const String& identifierBase)
{
    ensureSession(sessionID, identifierBase, nullptr);
}

RetainPtr<CFHTTPCookieStorageRef> NetworkStorageSession::cookieStorage() const
{
    if (m_platformCookieStorage)
        return m_platformCookieStorage;

    if (m_platformSession)
        return adoptCF(_CFURLStorageSessionCopyCookieStorage(kCFAllocatorDefault, m_platformSession.get()));

#if USE(CFURLCONNECTION)
    return _CFHTTPCookieStorageGetDefault(kCFAllocatorDefault);
#else
    // When using NSURLConnection, we also use its shared cookie storage.
    return nullptr;
#endif
}

void NetworkStorageSession::setCookieStoragePartitioningEnabled(bool enabled)
{
    cookieStoragePartitioningEnabled = enabled;
}

void NetworkStorageSession::setStorageAccessAPIEnabled(bool enabled)
{
    storageAccessAPIEnabled = enabled;
}

#if HAVE(CFNETWORK_STORAGE_PARTITIONING)

String NetworkStorageSession::cookieStoragePartition(const ResourceRequest& request) const
{
    return cookieStoragePartition(request.firstPartyForCookies(), request.url());
}

static inline String getPartitioningDomain(const URL& url) 
{
#if ENABLE(PUBLIC_SUFFIX_LIST)
    auto domain = topPrivatelyControlledDomain(url.host());
    if (domain.isEmpty())
        domain = url.host();
#else
    auto domain = url.host();
#endif
    return domain;
}

String NetworkStorageSession::cookieStoragePartition(const URL& firstPartyForCookies, const URL& resource) const
{
    if (!cookieStoragePartitioningEnabled)
        return emptyString();
    
    auto resourceDomain = getPartitioningDomain(resource);
    if (!shouldPartitionCookies(resourceDomain))
        return emptyString();

    auto firstPartyDomain = getPartitioningDomain(firstPartyForCookies);
    if (firstPartyDomain == resourceDomain)
        return emptyString();

    if (storageAccessAPIEnabled && isStorageAccessGranted(resourceDomain, firstPartyDomain))
        return emptyString();

    return firstPartyDomain;
}

bool NetworkStorageSession::shouldPartitionCookies(const String& topPrivatelyControlledDomain) const
{
    if (topPrivatelyControlledDomain.isEmpty())
        return false;

    return m_topPrivatelyControlledDomainsToPartition.contains(topPrivatelyControlledDomain);
}

bool NetworkStorageSession::shouldBlockThirdPartyCookies(const String& topPrivatelyControlledDomain) const
{
    if (topPrivatelyControlledDomain.isEmpty())
        return false;

    return m_topPrivatelyControlledDomainsToBlock.contains(topPrivatelyControlledDomain);
}

bool NetworkStorageSession::shouldBlockCookies(const ResourceRequest& request) const
{
    if (!cookieStoragePartitioningEnabled)
        return false;

    return shouldBlockCookies(request.firstPartyForCookies(), request.url());
}
    
bool NetworkStorageSession::shouldBlockCookies(const URL& firstPartyForCookies, const URL& resource) const
{
    if (!cookieStoragePartitioningEnabled)
        return false;
    
    auto firstPartyDomain = getPartitioningDomain(firstPartyForCookies);
    if (firstPartyDomain.isEmpty())
        return false;

    auto resourceDomain = getPartitioningDomain(resource);
    if (resourceDomain.isEmpty())
        return false;

    if (firstPartyDomain == resourceDomain)
        return false;

    return shouldBlockThirdPartyCookies(resourceDomain);
}

void NetworkStorageSession::setPrevalentDomainsToPartitionOrBlockCookies(const Vector<String>& domainsToPartition, const Vector<String>& domainsToBlock, const Vector<String>& domainsToNeitherPartitionNorBlock, bool clearFirst)
{
    if (clearFirst) {
        m_topPrivatelyControlledDomainsToPartition.clear();
        m_topPrivatelyControlledDomainsToBlock.clear();
        m_domainsGrantedStorageAccess.clear();
    }

    for (auto& domain : domainsToPartition) {
        m_topPrivatelyControlledDomainsToPartition.add(domain);
        if (!clearFirst)
            m_topPrivatelyControlledDomainsToBlock.remove(domain);
    }

    for (auto& domain : domainsToBlock) {
        // FIXME: https://bugs.webkit.org/show_bug.cgi?id=177394
        // m_topPrivatelyControlledDomainsToBlock.add(domain);
        // if (!clearFirst)
        //     m_topPrivatelyControlledDomainsToPartition.remove(domain);
        m_topPrivatelyControlledDomainsToPartition.add(domain);
    }
    
    if (!clearFirst) {
        for (auto& domain : domainsToNeitherPartitionNorBlock) {
            m_topPrivatelyControlledDomainsToPartition.remove(domain);
            m_topPrivatelyControlledDomainsToBlock.remove(domain);
        }
    }
}

void NetworkStorageSession::removePrevalentDomains(const Vector<String>& domains)
{
    for (auto& domain : domains) {
        m_topPrivatelyControlledDomainsToPartition.remove(domain);
        m_topPrivatelyControlledDomainsToBlock.remove(domain);
    }
}

bool NetworkStorageSession::isStorageAccessGranted(const String& resourceDomain, const String& firstPartyDomain) const
{
    auto it = m_domainsGrantedStorageAccess.find(firstPartyDomain);
    if (it == m_domainsGrantedStorageAccess.end())
        return false;

    return it->value.contains(resourceDomain);
}

void NetworkStorageSession::setStorageAccessGranted(const String& resourceDomain, const String& firstPartyDomain, bool value)
{
    auto iterator = m_domainsGrantedStorageAccess.find(firstPartyDomain);
    if (value) {
        if (iterator == m_domainsGrantedStorageAccess.end())
            m_domainsGrantedStorageAccess.add(firstPartyDomain, HashSet<String>({ resourceDomain }));
        else
            iterator->value.add(resourceDomain);
    } else {
        if (iterator == m_domainsGrantedStorageAccess.end())
            return;
        iterator->value.remove(resourceDomain);
        if (iterator->value.isEmpty())
            m_domainsGrantedStorageAccess.remove(firstPartyDomain);
    }
}

#endif // HAVE(CFNETWORK_STORAGE_PARTITIONING)

#if !PLATFORM(COCOA)
void NetworkStorageSession::setCookies(const Vector<Cookie>&, const URL&, const URL&)
{
    // FIXME: Implement this. <https://webkit.org/b/156298>
}
#endif

}
