/*
 * Copyright (C) 2007-2009, 2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "JSDOMBinding.h"
#include "JSDOMGlobalObject.h"
#include "ScriptExecutionContext.h"
#include <heap/Strong.h>
#include <heap/StrongInlines.h>
#include <runtime/JSObject.h>
#include <wtf/Threading.h>

namespace WebCore {

// We have to clean up this data on the context thread because unprotecting a
// JSObject on the wrong thread without synchronization would corrupt the heap
// (and synchronization would be slow).

class JSCallbackData {
public:
    enum class CallbackType { Function, Object, FunctionOrObject };

    JSDOMGlobalObject* globalObject() { return m_globalObject.get(); }

protected:
    explicit JSCallbackData(JSDOMGlobalObject* globalObject)
        : m_globalObject(globalObject)
#ifndef NDEBUG
        , m_thread(Thread::current())
#endif
    {
    }
    
    ~JSCallbackData()
    {
#if !PLATFORM(IOS)
        ASSERT(m_thread.ptr() == &Thread::current());
#endif
    }
    
    static JSC::JSValue invokeCallback(JSDOMGlobalObject&, JSC::JSObject* callback, JSC::JSValue thisValue, JSC::MarkedArgumentBuffer&, CallbackType, JSC::PropertyName functionName, NakedPtr<JSC::Exception>& returnedException);

private:
    JSC::Weak<JSDOMGlobalObject> m_globalObject;
#ifndef NDEBUG
    Ref<Thread> m_thread;
#endif
};

class JSCallbackDataStrong : public JSCallbackData {
public:
    JSCallbackDataStrong(JSC::JSObject* callback, JSDOMGlobalObject* globalObject, void*)
        : JSCallbackData(globalObject)
        , m_callback(globalObject->vm(), callback)
    {
    }

    JSC::JSObject* callback() { return m_callback.get(); }

    JSC::JSValue invokeCallback(JSC::JSValue thisValue, JSC::MarkedArgumentBuffer& args, CallbackType callbackType, JSC::PropertyName functionName, NakedPtr<JSC::Exception>& returnedException)
    {
        auto* globalObject = this->globalObject();
        if (!globalObject)
            return { };

        return JSCallbackData::invokeCallback(*globalObject, callback(), thisValue, args, callbackType, functionName, returnedException);
    }

private:
    JSC::Strong<JSC::JSObject> m_callback;
};

class JSCallbackDataWeak : public JSCallbackData {
public:
    JSCallbackDataWeak(JSC::JSObject* callback, JSDOMGlobalObject* globalObject, void* owner)
        : JSCallbackData(globalObject)
        , m_callback(callback, &m_weakOwner, owner)
    {
    }

    JSC::JSObject* callback() { return m_callback.get(); }

    JSC::JSValue invokeCallback(JSC::JSValue thisValue, JSC::MarkedArgumentBuffer& args, CallbackType callbackType, JSC::PropertyName functionName, NakedPtr<JSC::Exception>& returnedException)
    {
        auto* globalObject = this->globalObject();
        if (!globalObject)
            return { };

        return JSCallbackData::invokeCallback(*globalObject, callback(), thisValue, args, callbackType, functionName, returnedException);
    }

    void visitJSFunction(JSC::SlotVisitor&);

private:
    class WeakOwner : public JSC::WeakHandleOwner {
        bool isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown>, void* context, JSC::SlotVisitor&) override;
    };
    WeakOwner m_weakOwner;
    JSC::Weak<JSC::JSObject> m_callback;
};

class DeleteCallbackDataTask : public ScriptExecutionContext::Task {
public:
    template <typename CallbackDataType>
    explicit DeleteCallbackDataTask(CallbackDataType* data)
        : ScriptExecutionContext::Task(ScriptExecutionContext::Task::CleanupTask, [data = std::unique_ptr<CallbackDataType>(data)] (ScriptExecutionContext&) {
        })
    {
    }
};

} // namespace WebCore
