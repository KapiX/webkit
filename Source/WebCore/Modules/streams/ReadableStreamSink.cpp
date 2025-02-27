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
#include "ReadableStreamSink.h"

#include "BufferSource.h"
#include "DOMException.h"
#include "ReadableStream.h"
#include "SharedBuffer.h"

#if ENABLE(STREAMS_API)

namespace WebCore {

ReadableStreamToSharedBufferSink::ReadableStreamToSharedBufferSink(Callback&& callback)
    : m_callback { WTFMove(callback) }
{
}

void ReadableStreamToSharedBufferSink::pipeFrom(ReadableStream& stream)
{
    stream.pipeTo(*this);
}

void ReadableStreamToSharedBufferSink::enqueue(const BufferSource& buffer)
{
    if (!buffer.length())
        return;

    if (!m_data) {
        m_data = SharedBuffer::create(buffer.data(), buffer.length());
        return;
    }
    m_data->append(reinterpret_cast<const char*>(buffer.data()), buffer.length());
}

void ReadableStreamToSharedBufferSink::close()
{
    if (auto callback = WTFMove(m_callback))
        callback(WTFMove(m_data));
}

void ReadableStreamToSharedBufferSink::error(String&& message)
{
    if (auto callback = WTFMove(m_callback))
        callback(Exception { TypeError, WTFMove(message) });
}

} // namespace WebCore

#endif // ENABLE(STREAMS_API)
