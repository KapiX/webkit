/*
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
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

#include "CurlRequestSchedulerClient.h"
#include "CurlResponse.h"
#include "CurlSSLVerifier.h"
#include "FileSystem.h"
#include "FormDataStreamCurl.h"
#include "NetworkLoadMetrics.h"
#include "ResourceRequest.h"
#include <wtf/Noncopyable.h>

namespace WebCore {

class CurlRequestClient;
class ResourceError;
class SharedBuffer;

class CurlRequest : public ThreadSafeRefCounted<CurlRequest>, public CurlRequestSchedulerClient {
    WTF_MAKE_NONCOPYABLE(CurlRequest);

public:
    static Ref<CurlRequest> create(const ResourceRequest& request, CurlRequestClient* client, bool shouldSuspend = false)
    {
        return adoptRef(*new CurlRequest(request, client, shouldSuspend));
    }

    virtual ~CurlRequest() = default;

    void setClient(CurlRequestClient* client) { m_client = client;  }
    void setUserPass(const String&, const String&);

    void start(bool isSyncRequest = false);
    void cancel();
    void suspend();
    void resume();

    bool isSyncRequest() const { return m_isSyncRequest; }
    bool isCompletedOrCancelled() const { return !m_curlHandle || m_cancelled; }


    // Processing for DidReceiveResponse
    void completeDidReceiveResponse();

    // Download
    void enableDownloadToFile();
    const String& getDownloadedFilePath();

    NetworkLoadMetrics getNetworkLoadMetrics() { return m_networkLoadMetrics.isolatedCopy(); }

private:
    enum class Action {
        None,
        ReceiveData,
        StartTransfer,
        FinishTransfer
    };

    CurlRequest(const ResourceRequest&, CurlRequestClient*, bool shouldSuspend);

    void retain() override { ref(); }
    void release() override { deref(); }
    CURL* handle() override { return m_curlHandle ? m_curlHandle->handle() : nullptr; }

    void startWithJobManager();

    void callClient(WTF::Function<void(CurlRequestClient*)>);

    // Transfer processing of Request body, Response header/body
    // Called by worker thread in case of async, main thread in case of sync.
    CURL* setupTransfer() override;
    CURLcode willSetupSslCtx(void*);
    size_t willSendData(char*, size_t, size_t);
    size_t didReceiveHeader(String&&);
    size_t didReceiveData(Ref<SharedBuffer>&&);
    void didCompleteTransfer(CURLcode) override;
    void didCancelTransfer() override;
    void finalizeTransfer();

    // For POST and PUT method 
    void resolveBlobReferences(ResourceRequest&);
    void setupPOST(ResourceRequest&);
    void setupPUT(ResourceRequest&);
    void setupFormData(ResourceRequest&, bool);

    // Processing for DidReceiveResponse
    bool needToInvokeDidReceiveResponse() const { return !m_didNotifyResponse || !m_didReturnFromNotify; }
    bool needToInvokeDidCancelTransfer() const { return m_didNotifyResponse && !m_didReturnFromNotify && m_actionAfterInvoke == Action::FinishTransfer; }
    void invokeDidReceiveResponseForFile(URL&);
    void invokeDidReceiveResponse(Action);
    void setRequestPaused(bool);
    void setCallbackPaused(bool);
    void pausedStatusChanged();
    bool isPaused() const { return m_isPausedOfRequest || m_isPausedOfCallback; };

    // Download
    void writeDataToDownloadFileIfEnabled(const SharedBuffer&);
    void closeDownloadFile();
    void cleanupDownloadFile();

    // Callback functions for curl
    static CURLcode willSetupSslCtxCallback(CURL*, void*, void*);
    static size_t willSendDataCallback(char*, size_t, size_t, void*);
    static size_t didReceiveHeaderCallback(char*, size_t, size_t, void*);
    static size_t didReceiveDataCallback(char*, size_t, size_t, void*);


    std::atomic<CurlRequestClient*> m_client { };
    bool m_isSyncRequest { false };
    bool m_cancelled { false };

    // Used by worker thread in case of async, and main thread in case of sync.
    ResourceRequest m_request;
    String m_user;
    String m_password;
    bool m_shouldSuspend { false };

    std::unique_ptr<CurlHandle> m_curlHandle;
    std::unique_ptr<FormDataStream> m_formDataStream;
    Vector<char> m_postBuffer;
    CurlSSLVerifier m_sslVerifier;

    CurlResponse m_response;
    bool m_didReceiveResponse { false };
    bool m_didNotifyResponse { false };
    bool m_didReturnFromNotify { false };
    Action m_actionAfterInvoke { Action::None };
    CURLcode m_finishedResultCode { CURLE_OK };

    bool m_isPausedOfRequest { false };
    bool m_isPausedOfCallback { false };

    Lock m_downloadMutex;
    bool m_isEnabledDownloadToFile { false };
    String m_downloadFilePath;
    FileSystem::PlatformFileHandle m_downloadFileHandle { FileSystem::invalidPlatformFileHandle };

    NetworkLoadMetrics m_networkLoadMetrics;
};

} // namespace WebCore
