/*
 * Copyright (C) 2014 Igalia S.L.
 * Copyright (C) 2016 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include "APIObject.h"
#include <WebCore/CaptureDevice.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {
class SecurityOrigin;
}

namespace WebKit {

class UserMediaPermissionRequestManagerProxy;

class UserMediaPermissionRequestProxy : public API::ObjectImpl<API::Object::Type::UserMediaPermissionRequest> {
public:
    static Ref<UserMediaPermissionRequestProxy> create(UserMediaPermissionRequestManagerProxy& manager, uint64_t userMediaID, uint64_t mainFrameID, uint64_t frameID, Ref<WebCore::SecurityOrigin>&& userMediaDocumentOrigin, Ref<WebCore::SecurityOrigin>&& topLevelDocumentOrigin, Vector<WebCore::CaptureDevice>&& videoDevices, Vector<WebCore::CaptureDevice>&& audioDevices, String&& deviceIDHashSalt)
    {
        return adoptRef(*new UserMediaPermissionRequestProxy(manager, userMediaID, mainFrameID, frameID, WTFMove(userMediaDocumentOrigin), WTFMove(topLevelDocumentOrigin), WTFMove(videoDevices), WTFMove(audioDevices), WTFMove(deviceIDHashSalt)));
    }

    void allow(const String& videoDeviceUID, const String& audioDeviceUID);
    void allow();

    enum class UserMediaAccessDenialReason { NoConstraints, UserMediaDisabled, NoCaptureDevices, InvalidConstraint, HardwareError, PermissionDenied, OtherFailure };
    void deny(UserMediaAccessDenialReason);

    void invalidate();

    bool requiresAudio() const { return m_eligibleAudioDevices.size(); }
    bool requiresVideo() const { return m_eligibleVideoDevices.size(); }

    Vector<String> videoDeviceUIDs() const;
    Vector<String> audioDeviceUIDs() const;

    uint64_t mainFrameID() const { return m_mainFrameID; }
    uint64_t frameID() const { return m_frameID; }
    WebCore::SecurityOrigin& topLevelDocumentSecurityOrigin() { return m_topLevelDocumentSecurityOrigin.get(); }
    WebCore::SecurityOrigin& userMediaDocumentSecurityOrigin() { return m_userMediaDocumentSecurityOrigin.get(); }

    const String& deviceIdentifierHashSalt() const { return m_deviceIdentifierHashSalt; }

private:
    UserMediaPermissionRequestProxy(UserMediaPermissionRequestManagerProxy&, uint64_t userMediaID, uint64_t mainFrameID, uint64_t frameID, Ref<WebCore::SecurityOrigin>&& userMediaDocumentOrigin, Ref<WebCore::SecurityOrigin>&& topLevelDocumentOrigin, Vector<WebCore::CaptureDevice>&& videoDevices, Vector<WebCore::CaptureDevice>&& audioDevices, String&&);

    UserMediaPermissionRequestManagerProxy* m_manager;
    uint64_t m_userMediaID;
    uint64_t m_mainFrameID;
    uint64_t m_frameID;
    Ref<WebCore::SecurityOrigin> m_userMediaDocumentSecurityOrigin;
    Ref<WebCore::SecurityOrigin> m_topLevelDocumentSecurityOrigin;
    Vector<WebCore::CaptureDevice> m_eligibleVideoDevices;
    Vector<WebCore::CaptureDevice> m_eligibleAudioDevices;
    String m_deviceIdentifierHashSalt;
};

} // namespace WebKit
