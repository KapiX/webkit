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
#include "ExtendableEvent.h"

#if ENABLE(SERVICE_WORKER)

#include "JSDOMPromise.h"

namespace WebCore {

ExtendableEvent::ExtendableEvent(const AtomicString& type, const ExtendableEventInit& initializer, IsTrusted isTrusted)
    : Event(type, initializer, isTrusted)
{
}

ExtendableEvent::ExtendableEvent(const AtomicString& type, bool bubbles, bool cancelable)
    : Event(type, bubbles, cancelable)
{
}

ExtendableEvent::~ExtendableEvent()
{
}

// https://w3c.github.io/ServiceWorker/#dom-extendableevent-waituntil
ExceptionOr<void> ExtendableEvent::waitUntil(Ref<DOMPromise>&& promise)
{
    if (!isTrusted())
        return Exception { InvalidStateError, ASCIILiteral("Event is not trusted") };

    // If the pending promises count is zero and the dispatch flag is unset, throw an "InvalidStateError" DOMException.
    if (!m_pendingPromiseCount && !isBeingDispatched())
        return Exception { InvalidStateError, ASCIILiteral("Event is no longer being dispatched and has no pending promises") };

    addExtendLifetimePromise(WTFMove(promise));
    return { };
}

void ExtendableEvent::addExtendLifetimePromise(Ref<DOMPromise>&& promise)
{
    promise->whenSettled([this, protectedThis = makeRefPtr(this), settledPromise = promise.ptr()] () {
        --m_pendingPromiseCount;

        // FIXME: Let registration be the context object's relevant global object's associated service worker's containing service worker registration.
        // FIXME: If registration's uninstalling flag is set, invoke Try Clear Registration with registration.
        // FIXME: If registration is not null, invoke Try Activate with registration.

        if (m_pendingPromiseCount)
            return;

        auto settledPromises = WTFMove(m_extendLifetimePromises);
        if (auto handler = WTFMove(m_whenAllExtendLifetimePromisesAreSettledHandler))
            handler(WTFMove(settledPromises));
    });

    m_extendLifetimePromises.add(WTFMove(promise));
    ++m_pendingPromiseCount;
}

void ExtendableEvent::whenAllExtendLifetimePromisesAreSettled(WTF::Function<void(HashSet<Ref<DOMPromise>>&&)>&& handler)
{
    ASSERT_WITH_MESSAGE(target(), "Event has not been dispatched yet");
    ASSERT(!m_whenAllExtendLifetimePromisesAreSettledHandler);

    if (!m_pendingPromiseCount) {
        handler(WTFMove(m_extendLifetimePromises));
        return;
    }

    m_whenAllExtendLifetimePromisesAreSettledHandler = WTFMove(handler);
}

} // namespace WebCore

#endif // ENABLE(SERVICE_WORKER)
