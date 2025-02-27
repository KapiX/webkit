/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2011, 2012 Apple Inc. All rights reserved.
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

#include "config.h"
#include "DeprecatedGlobalSettings.h"

#include "AudioSession.h"
#include "HTMLMediaElement.h"
#include "RuntimeApplicationChecks.h"
#include <wtf/NeverDestroyed.h>

#if ENABLE(MEDIA_STREAM)
#include "MockRealtimeMediaSourceCenter.h"

#if USE(AVFOUNDATION)
#include "RealtimeMediaSourceCenterMac.h"
#endif
#endif

#if PLATFORM(COCOA)
#include <wtf/spi/darwin/dyldSPI.h>
#endif

namespace WebCore {

#if USE(AVFOUNDATION)
bool DeprecatedGlobalSettings::gAVFoundationEnabled = true;
bool DeprecatedGlobalSettings::gAVFoundationNSURLSessionEnabled = true;
#endif

#if PLATFORM(COCOA)
bool DeprecatedGlobalSettings::gQTKitEnabled = false;
#endif

#if USE(GSTREAMER)
bool DeprecatedGlobalSettings::gGStreamerEnabled = true;
#endif

bool DeprecatedGlobalSettings::gMockScrollbarsEnabled = false;
bool DeprecatedGlobalSettings::gUsesOverlayScrollbars = false;
bool DeprecatedGlobalSettings::gMockScrollAnimatorEnabled = false;

#if ENABLE(MEDIA_STREAM)
bool DeprecatedGlobalSettings::gMockCaptureDevicesEnabled = false;
bool DeprecatedGlobalSettings::gMediaCaptureRequiresSecureConnection = true;
#endif

#if PLATFORM(WIN)
bool DeprecatedGlobalSettings::gShouldUseHighResolutionTimers = true;
#endif
    
bool DeprecatedGlobalSettings::gShouldRespectPriorityInCSSAttributeSetters = false;
bool DeprecatedGlobalSettings::gLowPowerVideoAudioBufferSizeEnabled = false;
bool DeprecatedGlobalSettings::gResourceLoadStatisticsEnabledEnabled = false;
bool DeprecatedGlobalSettings::gAllowsAnySSLCertificate = false;

#if PLATFORM(IOS)
bool DeprecatedGlobalSettings::gNetworkDataUsageTrackingEnabled = false;
bool DeprecatedGlobalSettings::gAVKitEnabled = false;
bool DeprecatedGlobalSettings::gShouldOptOutOfNetworkStateObservation = false;
#endif
bool DeprecatedGlobalSettings::gManageAudioSession = false;
bool DeprecatedGlobalSettings::gCustomPasteboardDataEnabled = false;

bool DeprecatedGlobalSettings::defaultCustomPasteboardDataEnabled()
{
#if PLATFORM(IOS) && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110300
    return IOSApplication::isMobileSafari() || dyld_get_program_sdk_version() >= DYLD_IOS_VERSION_11_3;
#elif PLATFORM(MAC) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 101300
    return MacApplication::isSafari() || dyld_get_program_sdk_version() > DYLD_MACOSX_VERSION_10_13;
#elif PLATFORM(MAC)
    return MacApplication::isSafari();
#else
    return false;
#endif
}

#if PLATFORM(WIN)
void DeprecatedGlobalSettings::setShouldUseHighResolutionTimers(bool shouldUseHighResolutionTimers)
{
    gShouldUseHighResolutionTimers = shouldUseHighResolutionTimers;
}
#endif

#if USE(AVFOUNDATION)
void DeprecatedGlobalSettings::setAVFoundationEnabled(bool enabled)
{
    if (gAVFoundationEnabled == enabled)
        return;

    gAVFoundationEnabled = enabled;
    HTMLMediaElement::resetMediaEngines();
}

void DeprecatedGlobalSettings::setAVFoundationNSURLSessionEnabled(bool enabled)
{
    if (gAVFoundationNSURLSessionEnabled == enabled)
        return;

    gAVFoundationNSURLSessionEnabled = enabled;
}
#endif

#if PLATFORM(COCOA)
void DeprecatedGlobalSettings::setQTKitEnabled(bool enabled)
{
    if (gQTKitEnabled == enabled)
        return;

    gQTKitEnabled = enabled;
    HTMLMediaElement::resetMediaEngines();
}
#endif

#if USE(GSTREAMER)
void DeprecatedGlobalSettings::setGStreamerEnabled(bool enabled)
{
    if (gGStreamerEnabled == enabled)
        return;

    gGStreamerEnabled = enabled;

#if ENABLE(VIDEO)
    HTMLMediaElement::resetMediaEngines();
#endif
}
#endif

#if ENABLE(MEDIA_STREAM)
bool DeprecatedGlobalSettings::mockCaptureDevicesEnabled()
{
    return gMockCaptureDevicesEnabled;
}

void DeprecatedGlobalSettings::setMockCaptureDevicesEnabled(bool enabled)
{
    gMockCaptureDevicesEnabled = enabled;
    MockRealtimeMediaSourceCenter::setMockRealtimeMediaSourceCenterEnabled(enabled);
}

bool DeprecatedGlobalSettings::mediaCaptureRequiresSecureConnection()
{
    return gMediaCaptureRequiresSecureConnection;
}

void DeprecatedGlobalSettings::setMediaCaptureRequiresSecureConnection(bool mediaCaptureRequiresSecureConnection)
{
    gMediaCaptureRequiresSecureConnection = mediaCaptureRequiresSecureConnection;
}
#endif

// It's very important that this setting doesn't change in the middle of a document's lifetime.
// The Mac port uses this flag when registering and deregistering platform-dependent scrollbar
// objects. Therefore, if this changes at an unexpected time, deregistration may not happen
// correctly, which may cause the platform to follow dangling pointers.
void DeprecatedGlobalSettings::setMockScrollbarsEnabled(bool flag)
{
    gMockScrollbarsEnabled = flag;
    // FIXME: This should update scroll bars in existing pages.
}

bool DeprecatedGlobalSettings::mockScrollbarsEnabled()
{
    return gMockScrollbarsEnabled;
}

void DeprecatedGlobalSettings::setUsesOverlayScrollbars(bool flag)
{
    gUsesOverlayScrollbars = flag;
    // FIXME: This should update scroll bars in existing pages.
}

bool DeprecatedGlobalSettings::usesOverlayScrollbars()
{
    return gUsesOverlayScrollbars;
}

void DeprecatedGlobalSettings::setUsesMockScrollAnimator(bool flag)
{
    gMockScrollAnimatorEnabled = flag;
}

bool DeprecatedGlobalSettings::usesMockScrollAnimator()
{
    return gMockScrollAnimatorEnabled;
}

void DeprecatedGlobalSettings::setShouldRespectPriorityInCSSAttributeSetters(bool flag)
{
    gShouldRespectPriorityInCSSAttributeSetters = flag;
}

bool DeprecatedGlobalSettings::shouldRespectPriorityInCSSAttributeSetters()
{
    return gShouldRespectPriorityInCSSAttributeSetters;
}

void DeprecatedGlobalSettings::setLowPowerVideoAudioBufferSizeEnabled(bool flag)
{
    gLowPowerVideoAudioBufferSizeEnabled = flag;
}

void DeprecatedGlobalSettings::setResourceLoadStatisticsEnabled(bool flag)
{
    gResourceLoadStatisticsEnabledEnabled = flag;
}

#if PLATFORM(IOS)
void DeprecatedGlobalSettings::setAudioSessionCategoryOverride(unsigned sessionCategory)
{
    AudioSession::sharedSession().setCategoryOverride(static_cast<AudioSession::CategoryType>(sessionCategory));
}

unsigned DeprecatedGlobalSettings::audioSessionCategoryOverride()
{
    return AudioSession::sharedSession().categoryOverride();
}

void DeprecatedGlobalSettings::setNetworkDataUsageTrackingEnabled(bool trackingEnabled)
{
    gNetworkDataUsageTrackingEnabled = trackingEnabled;
}

bool DeprecatedGlobalSettings::networkDataUsageTrackingEnabled()
{
    return gNetworkDataUsageTrackingEnabled;
}

static String& sharedNetworkInterfaceNameGlobal()
{
    static NeverDestroyed<String> networkInterfaceName;
    return networkInterfaceName;
}

void DeprecatedGlobalSettings::setNetworkInterfaceName(const String& networkInterfaceName)
{
    sharedNetworkInterfaceNameGlobal() = networkInterfaceName;
}

const String& DeprecatedGlobalSettings::networkInterfaceName()
{
    return sharedNetworkInterfaceNameGlobal();
}
#endif

bool DeprecatedGlobalSettings::globalConstRedeclarationShouldThrow()
{
#if PLATFORM(MAC)
    return !MacApplication::isIBooks();
#elif PLATFORM(IOS)
    return !IOSApplication::isIBooks();
#else
    return true;
#endif
}

void DeprecatedGlobalSettings::setAllowsAnySSLCertificate(bool allowAnySSLCertificate)
{
    gAllowsAnySSLCertificate = allowAnySSLCertificate;
}

bool DeprecatedGlobalSettings::allowsAnySSLCertificate()
{
    return gAllowsAnySSLCertificate;
}

} // namespace WebCore
