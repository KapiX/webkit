/*
 * Copyright (C) 2010-2017 Apple Inc. All rights reserved.
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
#include "WebProcessCreationParameters.h"

#include "APIData.h"
#if PLATFORM(COCOA)
#include "ArgumentCodersCF.h"
#endif
#include "WebCoreArgumentCoders.h"

namespace WebKit {

WebProcessCreationParameters::WebProcessCreationParameters()
{
}

WebProcessCreationParameters::~WebProcessCreationParameters()
{
}

void WebProcessCreationParameters::encode(IPC::Encoder& encoder) const
{
    encoder << injectedBundlePath;
    encoder << injectedBundlePathExtensionHandle;
    encoder << additionalSandboxExtensionHandles;
    encoder << initializationUserData;
    encoder << applicationCacheDirectory;
    encoder << applicationCacheFlatFileSubdirectoryName;
    encoder << applicationCacheDirectoryExtensionHandle;
    encoder << webSQLDatabaseDirectory;
    encoder << webSQLDatabaseDirectoryExtensionHandle;
    encoder << mediaCacheDirectory;
    encoder << mediaCacheDirectoryExtensionHandle;
    encoder << javaScriptConfigurationDirectory;
    encoder << javaScriptConfigurationDirectoryExtensionHandle;
#if PLATFORM(MAC)
    encoder << uiProcessCookieStorageIdentifier;
#endif
#if PLATFORM(IOS)
    encoder << cookieStorageDirectoryExtensionHandle;
    encoder << containerCachesDirectoryExtensionHandle;
    encoder << containerTemporaryDirectoryExtensionHandle;
#endif
    encoder << mediaKeyStorageDirectory;
    encoder << webCoreLoggingChannels;
    encoder << mediaKeyStorageDirectoryExtensionHandle;
#if ENABLE(MEDIA_STREAM)
    encoder << audioCaptureExtensionHandle;
    encoder << shouldCaptureAudioInUIProcess;
#endif
    encoder << shouldUseTestingNetworkSession;
    encoder << urlSchemesRegisteredAsEmptyDocument;
    encoder << urlSchemesRegisteredAsSecure;
    encoder << urlSchemesRegisteredAsBypassingContentSecurityPolicy;
    encoder << urlSchemesForWhichDomainRelaxationIsForbidden;
    encoder << urlSchemesRegisteredAsLocal;
    encoder << urlSchemesRegisteredAsNoAccess;
    encoder << urlSchemesRegisteredAsDisplayIsolated;
    encoder << urlSchemesRegisteredAsCORSEnabled;
    encoder << urlSchemesRegisteredAsAlwaysRevalidated;
    encoder << urlSchemesRegisteredAsCachePartitioned;
    encoder.encodeEnum(cacheModel);
    encoder << shouldAlwaysUseComplexTextCodePath;
    encoder << shouldEnableMemoryPressureReliefLogging;
    encoder << shouldSuppressMemoryPressureHandler;
    encoder << shouldUseFontSmoothing;
    encoder << resourceLoadStatisticsEnabled;
    encoder << fontWhitelist;
    encoder << terminationTimeout;
    encoder << languages;
    encoder << textCheckerState;
    encoder << fullKeyboardAccessEnabled;
    encoder << defaultRequestTimeoutInterval;
#if PLATFORM(COCOA)
    encoder << uiProcessBundleIdentifier;
#endif
    encoder << presentingApplicationPID;
#if PLATFORM(COCOA)
    encoder << accessibilityEnhancedUserInterfaceEnabled;
    encoder << acceleratedCompositingPort;
    encoder << uiProcessBundleResourcePath;
    encoder << uiProcessBundleResourcePathExtensionHandle;
    encoder << shouldEnableJIT;
    encoder << shouldEnableFTLJIT;
    encoder << !!bundleParameterData;
    if (bundleParameterData)
        encoder << bundleParameterData->dataReference();
#endif

#if ENABLE(NOTIFICATIONS)
    encoder << notificationPermissions;
#endif

    encoder << plugInAutoStartOriginHashes;
    encoder << plugInAutoStartOrigins;
    encoder << memoryCacheDisabled;

#if ENABLE(SERVICE_CONTROLS)
    encoder << hasImageServices;
    encoder << hasSelectionServices;
    encoder << hasRichContentServices;
#endif

#if ENABLE(NETSCAPE_PLUGIN_API)
    encoder << pluginLoadClientPolicies;
#endif

#if PLATFORM(COCOA)
    IPC::encode(encoder, networkATSContext.get());
#endif

#if OS(LINUX)
    encoder << memoryPressureMonitorHandle;
#endif

#if PLATFORM(WAYLAND)
    encoder << waylandCompositorDisplayName;
#endif

#if USE(SOUP)
    encoder << proxySettings;
#endif
}

bool WebProcessCreationParameters::decode(IPC::Decoder& decoder, WebProcessCreationParameters& parameters)
{
    if (!decoder.decode(parameters.injectedBundlePath))
        return false;
    if (!decoder.decode(parameters.injectedBundlePathExtensionHandle))
        return false;
    if (!decoder.decode(parameters.additionalSandboxExtensionHandles))
        return false;
    if (!decoder.decode(parameters.initializationUserData))
        return false;
    if (!decoder.decode(parameters.applicationCacheDirectory))
        return false;
    if (!decoder.decode(parameters.applicationCacheFlatFileSubdirectoryName))
        return false;
    if (!decoder.decode(parameters.applicationCacheDirectoryExtensionHandle))
        return false;
    if (!decoder.decode(parameters.webSQLDatabaseDirectory))
        return false;
    if (!decoder.decode(parameters.webSQLDatabaseDirectoryExtensionHandle))
        return false;
    if (!decoder.decode(parameters.mediaCacheDirectory))
        return false;
    if (!decoder.decode(parameters.mediaCacheDirectoryExtensionHandle))
        return false;
    if (!decoder.decode(parameters.javaScriptConfigurationDirectory))
        return false;
    if (!decoder.decode(parameters.javaScriptConfigurationDirectoryExtensionHandle))
        return false;
#if PLATFORM(MAC)
    if (!decoder.decode(parameters.uiProcessCookieStorageIdentifier))
        return false;
#endif
#if PLATFORM(IOS)
    if (!decoder.decode(parameters.cookieStorageDirectoryExtensionHandle))
        return false;
    if (!decoder.decode(parameters.containerCachesDirectoryExtensionHandle))
        return false;
    if (!decoder.decode(parameters.containerTemporaryDirectoryExtensionHandle))
        return false;
#endif
    if (!decoder.decode(parameters.mediaKeyStorageDirectory))
        return false;
    if (!decoder.decode(parameters.webCoreLoggingChannels))
        return false;
    if (!decoder.decode(parameters.mediaKeyStorageDirectoryExtensionHandle))
        return false;
#if ENABLE(MEDIA_STREAM)
    if (!decoder.decode(parameters.audioCaptureExtensionHandle))
        return false;
    if (!decoder.decode(parameters.shouldCaptureAudioInUIProcess))
        return false;
#endif
    if (!decoder.decode(parameters.shouldUseTestingNetworkSession))
        return false;
    if (!decoder.decode(parameters.urlSchemesRegisteredAsEmptyDocument))
        return false;
    if (!decoder.decode(parameters.urlSchemesRegisteredAsSecure))
        return false;
    if (!decoder.decode(parameters.urlSchemesRegisteredAsBypassingContentSecurityPolicy))
        return false;
    if (!decoder.decode(parameters.urlSchemesForWhichDomainRelaxationIsForbidden))
        return false;
    if (!decoder.decode(parameters.urlSchemesRegisteredAsLocal))
        return false;
    if (!decoder.decode(parameters.urlSchemesRegisteredAsNoAccess))
        return false;
    if (!decoder.decode(parameters.urlSchemesRegisteredAsDisplayIsolated))
        return false;
    if (!decoder.decode(parameters.urlSchemesRegisteredAsCORSEnabled))
        return false;
    if (!decoder.decode(parameters.urlSchemesRegisteredAsAlwaysRevalidated))
        return false;
    if (!decoder.decode(parameters.urlSchemesRegisteredAsCachePartitioned))
        return false;
    if (!decoder.decodeEnum(parameters.cacheModel))
        return false;
    if (!decoder.decode(parameters.shouldAlwaysUseComplexTextCodePath))
        return false;
    if (!decoder.decode(parameters.shouldEnableMemoryPressureReliefLogging))
        return false;
    if (!decoder.decode(parameters.shouldSuppressMemoryPressureHandler))
        return false;
    if (!decoder.decode(parameters.shouldUseFontSmoothing))
        return false;
    if (!decoder.decode(parameters.resourceLoadStatisticsEnabled))
        return false;
    if (!decoder.decode(parameters.fontWhitelist))
        return false;
    if (!decoder.decode(parameters.terminationTimeout))
        return false;
    if (!decoder.decode(parameters.languages))
        return false;
    if (!decoder.decode(parameters.textCheckerState))
        return false;
    if (!decoder.decode(parameters.fullKeyboardAccessEnabled))
        return false;
    if (!decoder.decode(parameters.defaultRequestTimeoutInterval))
        return false;
#if PLATFORM(COCOA)
    if (!decoder.decode(parameters.uiProcessBundleIdentifier))
        return false;
#endif
    if (!decoder.decode(parameters.presentingApplicationPID))
        return false;
#if PLATFORM(COCOA)
    if (!decoder.decode(parameters.accessibilityEnhancedUserInterfaceEnabled))
        return false;
    if (!decoder.decode(parameters.acceleratedCompositingPort))
        return false;
    if (!decoder.decode(parameters.uiProcessBundleResourcePath))
        return false;
    if (!decoder.decode(parameters.uiProcessBundleResourcePathExtensionHandle))
        return false;
    if (!decoder.decode(parameters.shouldEnableJIT))
        return false;
    if (!decoder.decode(parameters.shouldEnableFTLJIT))
        return false;

    bool hasBundleParameterData;
    if (!decoder.decode(hasBundleParameterData))
        return false;

    if (hasBundleParameterData) {
        IPC::DataReference dataReference;
        if (!decoder.decode(dataReference))
            return false;

        parameters.bundleParameterData = API::Data::create(dataReference.data(), dataReference.size());
    }
#endif

#if ENABLE(NOTIFICATIONS)
    if (!decoder.decode(parameters.notificationPermissions))
        return false;
#endif

    if (!decoder.decode(parameters.plugInAutoStartOriginHashes))
        return false;
    if (!decoder.decode(parameters.plugInAutoStartOrigins))
        return false;
    if (!decoder.decode(parameters.memoryCacheDisabled))
        return false;

#if ENABLE(SERVICE_CONTROLS)
    if (!decoder.decode(parameters.hasImageServices))
        return false;
    if (!decoder.decode(parameters.hasSelectionServices))
        return false;
    if (!decoder.decode(parameters.hasRichContentServices))
        return false;
#endif

#if ENABLE(NETSCAPE_PLUGIN_API)
    if (!decoder.decode(parameters.pluginLoadClientPolicies))
        return false;
#endif

#if PLATFORM(COCOA)
    if (!IPC::decode(decoder, parameters.networkATSContext))
        return false;
#endif

#if OS(LINUX)
    if (!decoder.decode(parameters.memoryPressureMonitorHandle))
        return false;
#endif

#if PLATFORM(WAYLAND)
    if (!decoder.decode(parameters.waylandCompositorDisplayName))
        return false;
#endif

#if USE(SOUP)
    if (!decoder.decode(parameters.proxySettings))
        return false;
#endif

    return true;
}

} // namespace WebKit
