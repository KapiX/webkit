/*
 * Copyright (C) 2014 Haiku, inc.
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
 * THIS SOFTWARE IS PROVIDED BY MOTOROLA INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MOTOROLA INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebProcess.h"

#include "NotImplemented.h"
#include <WebCore/Language.h>

namespace WebKit {

void WebProcess::platformInitializeWebProcess(const WebProcessCreationParameters& parameters, IPC::MessageDecoder&)
{
#if ENABLE(SECCOMP_FILTERS)
    {
#if PLATFORM(EFL)
        SeccompFiltersWebProcessEfl seccompFilters(parameters);
#endif
        seccompFilters.initialize();
    }
#endif

    if (usesNetworkProcess())
        return;

    notImplemented();
}

void WebProcess::platformTerminate()
{
    if (!usesNetworkProcess())
        WebCore::removeLanguageChangeObserver(this);
}

void WebProcess::platformSetCacheModel(CacheModel cacheModel)
{
    notImplemented();
}

void WebProcess::platformClearResourceCaches(ResourceCachesToClear cachesToClear)
{
    if (cachesToClear == InMemoryResourceCachesOnly)
        return;

    // If we're using the network process then it is the only one that needs to clear the disk cache.
    if (usesNetworkProcess())
        return;

    notImplemented();
}

}
