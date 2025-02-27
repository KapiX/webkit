/*
 * Copyright (C) 2013, 2015, 2016 Apple Inc. All Rights Reserved.
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

#pragma once

#if ENABLE(REMOTE_INSPECTOR)

#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/Lock.h>
#include <wtf/ProcessID.h>
#include <wtf/text/WTFString.h>

#if PLATFORM(COCOA)
#include "RemoteInspectorXPCConnection.h"
#include <wtf/RetainPtr.h>

OBJC_CLASS NSDictionary;
OBJC_CLASS NSString;
typedef RetainPtr<NSDictionary> TargetListing;
#endif

#if PLATFORM(HAIKU)
typedef void* TargetListing;
#endif

#if USE(GLIB)
#include <wtf/glib/GRefPtr.h>
typedef GRefPtr<GVariant> TargetListing;
typedef struct _GCancellable GCancellable;
typedef struct _GDBusConnection GDBusConnection;
typedef struct _GDBusInterfaceVTable GDBusInterfaceVTable;
#endif

namespace Inspector {

class RemoteAutomationTarget;
class RemoteConnectionToTarget;
class RemoteControllableTarget;
class RemoteInspectionTarget;
class RemoteInspectorClient;

class JS_EXPORT_PRIVATE RemoteInspector final
#if PLATFORM(COCOA)
    : public RemoteInspectorXPCConnection::Client
#endif
{
public:
    class Client {
    public:
        struct Capabilities {
            bool remoteAutomationAllowed : 1;
            String browserName;
            String browserVersion;
        };

        virtual ~Client() { }
        virtual bool remoteAutomationAllowed() const = 0;
        virtual String browserName() const { return { }; }
        virtual String browserVersion() const { return { }; }
        virtual void requestAutomationSession(const String& sessionIdentifier) = 0;
#if PLATFORM(COCOA)
        virtual void requestAutomationSessionWithCapabilities(NSString *sessionIdentifier, NSDictionary *forwardedCapabilities) = 0;
#endif
    };

    static void startDisabled();
    static RemoteInspector& singleton();
    friend class NeverDestroyed<RemoteInspector>;

    void registerTarget(RemoteControllableTarget*);
    void unregisterTarget(RemoteControllableTarget*);
    void updateTarget(RemoteControllableTarget*);
    void sendMessageToRemote(unsigned targetIdentifier, const String& message);

    RemoteInspector::Client* client() const { return m_client; }
    void setClient(RemoteInspector::Client*);
    void clientCapabilitiesDidChange();
    std::optional<RemoteInspector::Client::Capabilities> clientCapabilities() const { return m_clientCapabilities; }

    void setupFailed(unsigned targetIdentifier);
    void setupCompleted(unsigned targetIdentifier);
    bool waitingForAutomaticInspection(unsigned targetIdentifier);
    void updateAutomaticInspectionCandidate(RemoteInspectionTarget*);

    bool enabled() const { return m_enabled; }
    bool hasActiveDebugSession() const { return m_hasActiveDebugSession; }

    void start();
    void stop();

#if PLATFORM(COCOA)
    bool hasParentProcessInformation() const { return m_parentProcessIdentifier != 0; }
    ProcessID parentProcessIdentifier() const { return m_parentProcessIdentifier; }
    RetainPtr<CFDataRef> parentProcessAuditData() const { return m_parentProcessAuditData; }
    void setParentProcessInformation(ProcessID, RetainPtr<CFDataRef> auditData);
    void setParentProcessInfomationIsDelayed();
#endif

    void updateTargetListing(unsigned targetIdentifier);

#if USE(GLIB)
    void requestAutomationSession(const char* sessionID);
    void setup(unsigned targetIdentifier);
    void sendMessageToTarget(unsigned targetIdentifier, const char* message);
#endif

private:
    RemoteInspector();

    unsigned nextAvailableTargetIdentifier();

    enum class StopSource { API, XPCMessage };
    void stopInternal(StopSource);

#if PLATFORM(COCOA)
    void setupXPCConnectionIfNeeded();
#endif
#if USE(GLIB)
    void setupConnection(GRefPtr<GDBusConnection>&&);
    static const GDBusInterfaceVTable s_interfaceVTable;

    void receivedGetTargetListMessage();
    void receivedSetupMessage(unsigned targetIdentifier);
    void receivedDataMessage(unsigned targetIdentifier, const char* message);
    void receivedCloseMessage(unsigned targetIdentifier);
    void receivedAutomationSessionRequestMessage(const char* sessionID);
#endif

    TargetListing listingForTarget(const RemoteControllableTarget&) const;
    TargetListing listingForInspectionTarget(const RemoteInspectionTarget&) const;
    TargetListing listingForAutomationTarget(const RemoteAutomationTarget&) const;

    void pushListingsNow();
    void pushListingsSoon();

    void updateTargetListing(const RemoteControllableTarget&);

    void updateHasActiveDebugSession();
    void updateClientCapabilities();

    void sendAutomaticInspectionCandidateMessage();

#if PLATFORM(COCOA)
    void xpcConnectionReceivedMessage(RemoteInspectorXPCConnection*, NSString *messageName, NSDictionary *userInfo) override;
    void xpcConnectionFailed(RemoteInspectorXPCConnection*) override;
    void xpcConnectionUnhandledMessage(RemoteInspectorXPCConnection*, xpc_object_t) override;

    void receivedSetupMessage(NSDictionary *userInfo);
    void receivedDataMessage(NSDictionary *userInfo);
    void receivedDidCloseMessage(NSDictionary *userInfo);
    void receivedGetListingMessage(NSDictionary *userInfo);
    void receivedIndicateMessage(NSDictionary *userInfo);
    void receivedProxyApplicationSetupMessage(NSDictionary *userInfo);
    void receivedConnectionDiedMessage(NSDictionary *userInfo);
    void receivedAutomaticInspectionConfigurationMessage(NSDictionary *userInfo);
    void receivedAutomaticInspectionRejectMessage(NSDictionary *userInfo);
    void receivedAutomationSessionRequestMessage(NSDictionary *userInfo);
#endif

    static bool startEnabled;

    // Targets can be registered from any thread at any time.
    // Any target can send messages over the XPC connection.
    // So lock access to all maps and state as they can change
    // from any thread.
    Lock m_mutex;

    HashMap<unsigned, RemoteControllableTarget*> m_targetMap;
    HashMap<unsigned, RefPtr<RemoteConnectionToTarget>> m_targetConnectionMap;
    HashMap<unsigned, TargetListing> m_targetListingMap;

#if PLATFORM(COCOA)
    RefPtr<RemoteInspectorXPCConnection> m_relayConnection;
#endif
#if USE(GLIB)
    GRefPtr<GDBusConnection> m_dbusConnection;
    GRefPtr<GCancellable> m_cancellable;
#endif

    RemoteInspector::Client* m_client { nullptr };
    std::optional<RemoteInspector::Client::Capabilities> m_clientCapabilities;

#if PLATFORM(COCOA)
    dispatch_queue_t m_xpcQueue;
#endif
    unsigned m_nextAvailableTargetIdentifier { 1 };
    int m_notifyToken { 0 };
    bool m_enabled { false };
    bool m_hasActiveDebugSession { false };
    bool m_pushScheduled { false };

    ProcessID m_parentProcessIdentifier { 0 };
#if PLATFORM(COCOA)
    RetainPtr<CFDataRef> m_parentProcessAuditData;
#endif
    bool m_shouldSendParentProcessInformation { false };
    bool m_automaticInspectionEnabled { false };
    bool m_automaticInspectionPaused { false };
    unsigned m_automaticInspectionCandidateTargetIdentifier { 0 };
};

} // namespace Inspector

#endif // ENABLE(REMOTE_INSPECTOR)
