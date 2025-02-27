/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2015-2017 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PageDebuggerAgent.h"

#include "CachedResource.h"
#include "Document.h"
#include "EventListener.h"
#include "EventTarget.h"
#include "InspectorOverlay.h"
#include "InspectorPageAgent.h"
#include "InstrumentingAgents.h"
#include "MainFrame.h"
#include "Page.h"
#include "PageConsoleClient.h"
#include "PageScriptDebugServer.h"
#include "ScriptExecutionContext.h"
#include "ScriptState.h"
#include "Timer.h"
#include <inspector/InjectedScript.h>
#include <inspector/InjectedScriptManager.h>
#include <inspector/ScriptCallStack.h>
#include <inspector/ScriptCallStackFactory.h>
#include <wtf/NeverDestroyed.h>


namespace WebCore {

using namespace Inspector;

PageDebuggerAgent::PageDebuggerAgent(PageAgentContext& context, InspectorPageAgent* pageAgent, InspectorOverlay* overlay)
    : WebDebuggerAgent(context)
    , m_page(context.inspectedPage)
    , m_pageAgent(pageAgent)
    , m_overlay(overlay)
{
}

void PageDebuggerAgent::enable()
{
    WebDebuggerAgent::enable();
    m_instrumentingAgents.setPageDebuggerAgent(this);
}

void PageDebuggerAgent::disable(bool isBeingDestroyed)
{
    WebDebuggerAgent::disable(isBeingDestroyed);
    m_instrumentingAgents.setPageDebuggerAgent(nullptr);
}

String PageDebuggerAgent::sourceMapURLForScript(const Script& script)
{
    static NeverDestroyed<String> sourceMapHTTPHeader(MAKE_STATIC_STRING_IMPL("SourceMap"));
    static NeverDestroyed<String> sourceMapHTTPHeaderDeprecated(MAKE_STATIC_STRING_IMPL("X-SourceMap"));

    if (!script.url.isEmpty()) {
        CachedResource* resource = m_pageAgent->cachedResource(&m_page.mainFrame(), URL(ParsedURLString, script.url));
        if (resource) {
            String sourceMapHeader = resource->response().httpHeaderField(sourceMapHTTPHeader);
            if (!sourceMapHeader.isEmpty())
                return sourceMapHeader;

            sourceMapHeader = resource->response().httpHeaderField(sourceMapHTTPHeaderDeprecated);
            if (!sourceMapHeader.isEmpty())
                return sourceMapHeader;
        }
    }

    return InspectorDebuggerAgent::sourceMapURLForScript(script);
}

void PageDebuggerAgent::didClearAsyncStackTraceData()
{
    m_registeredEventListeners.clear();
    m_postMessageTimers.clear();
    m_nextEventListenerIdentifier = 1;
    m_nextPostMessageIdentifier = 1;
}

void PageDebuggerAgent::muteConsole()
{
    PageConsoleClient::mute();
}

void PageDebuggerAgent::unmuteConsole()
{
    PageConsoleClient::unmute();
}

void PageDebuggerAgent::breakpointActionLog(JSC::ExecState& state, const String& message)
{
    m_pageAgent->page().console().addMessage(MessageSource::JS, MessageLevel::Log, message, createScriptCallStack(&state));
}

InjectedScript PageDebuggerAgent::injectedScriptForEval(ErrorString& errorString, const int* executionContextId)
{
    if (!executionContextId) {
        JSC::ExecState* scriptState = mainWorldExecState(&m_pageAgent->mainFrame());
        return injectedScriptManager().injectedScriptFor(scriptState);
    }

    InjectedScript injectedScript = injectedScriptManager().injectedScriptForId(*executionContextId);
    if (injectedScript.hasNoValue())
        errorString = ASCIILiteral("Execution context with given id not found.");

    return injectedScript;
}

void PageDebuggerAgent::setOverlayMessage(ErrorString&, const String* message)
{
    m_overlay->setPausedInDebuggerMessage(message);
}

void PageDebuggerAgent::didClearMainFrameWindowObject()
{
    didClearGlobalObject();
}

void PageDebuggerAgent::mainFrameStartedLoading()
{
    if (isPaused()) {
        setSuppressAllPauses(true);
        ErrorString unused;
        resume(unused);
    }
}

void PageDebuggerAgent::mainFrameStoppedLoading()
{
    setSuppressAllPauses(false);
}

void PageDebuggerAgent::mainFrameNavigated()
{
    setSuppressAllPauses(false);
}

void PageDebuggerAgent::didAddEventListener(EventTarget& target, const AtomicString& eventType)
{
    if (!breakpointsActive())
        return;

    auto& eventListeners = target.eventListeners(eventType);
    const RefPtr<RegisteredEventListener>& listener = eventListeners.last();
    if (m_registeredEventListeners.contains(listener.get())) {
        ASSERT_NOT_REACHED();
        return;
    }

    JSC::ExecState* scriptState = target.scriptExecutionContext()->execState();
    if (!scriptState)
        return;

    int identifier = m_nextEventListenerIdentifier++;
    m_registeredEventListeners.set(listener.get(), identifier);

    didScheduleAsyncCall(scriptState, InspectorDebuggerAgent::AsyncCallType::EventListener, identifier, listener->isOnce());
}

void PageDebuggerAgent::willRemoveEventListener(EventTarget& target, const AtomicString& eventType, EventListener& listener, bool capture)
{
    auto& eventListeners = target.eventListeners(eventType);
    size_t listenerIndex = eventListeners.findMatching([&](auto& registeredListener) {
        return &registeredListener->callback() == &listener && registeredListener->useCapture() == capture;
    });

    if (listenerIndex == notFound)
        return;

    int identifier = m_registeredEventListeners.take(eventListeners[listenerIndex].get());
    didCancelAsyncCall(InspectorDebuggerAgent::AsyncCallType::EventListener, identifier);
}

void PageDebuggerAgent::willHandleEvent(const RegisteredEventListener& listener)
{
    auto it = m_registeredEventListeners.find(&listener);
    if (it == m_registeredEventListeners.end())
        return;

    willDispatchAsyncCall(InspectorDebuggerAgent::AsyncCallType::EventListener, it->value);
}

void PageDebuggerAgent::didRequestAnimationFrame(int callbackId, Document& document)
{
    if (!breakpointsActive())
        return;

    JSC::ExecState* scriptState = document.execState();
    if (!scriptState)
        return;

    didScheduleAsyncCall(scriptState, InspectorDebuggerAgent::AsyncCallType::RequestAnimationFrame, callbackId, true);
}

void PageDebuggerAgent::willFireAnimationFrame(int callbackId)
{
    willDispatchAsyncCall(InspectorDebuggerAgent::AsyncCallType::RequestAnimationFrame, callbackId);
}

void PageDebuggerAgent::didCancelAnimationFrame(int callbackId)
{
    didCancelAsyncCall(InspectorDebuggerAgent::AsyncCallType::RequestAnimationFrame, callbackId);
}

void PageDebuggerAgent::didPostMessage(const TimerBase& timer, JSC::ExecState& state)
{
    if (!breakpointsActive())
        return;

    if (m_postMessageTimers.contains(&timer)) {
        ASSERT_NOT_REACHED();
        return;
    }

    int postMessageIdentifier = m_nextPostMessageIdentifier++;
    m_postMessageTimers.set(&timer, postMessageIdentifier);

    didScheduleAsyncCall(&state, InspectorDebuggerAgent::AsyncCallType::PostMessage, postMessageIdentifier, true);
}

void PageDebuggerAgent::didFailPostMessage(const TimerBase& timer)
{
    auto it = m_postMessageTimers.find(&timer);
    if (it == m_postMessageTimers.end())
        return;

    didCancelAsyncCall(InspectorDebuggerAgent::AsyncCallType::PostMessage, it->value);

    m_postMessageTimers.remove(it);
}

void PageDebuggerAgent::willDispatchPostMessage(const TimerBase& timer)
{
    auto it = m_postMessageTimers.find(&timer);
    if (it == m_postMessageTimers.end())
        return;

    willDispatchAsyncCall(InspectorDebuggerAgent::AsyncCallType::PostMessage, it->value);
}

void PageDebuggerAgent::didDispatchPostMessage(const TimerBase& timer)
{
    auto it = m_postMessageTimers.find(&timer);
    if (it == m_postMessageTimers.end())
        return;

    didDispatchAsyncCall();

    m_postMessageTimers.remove(it);
}

} // namespace WebCore
