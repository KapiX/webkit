/*
    This file is part of the WebKit open source project.
    This file has been generated by generate-bindings.pl. DO NOT MODIFY!

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "JSTestPluginInterface.h"

#include "JSDOMBinding.h"
#include "JSDOMConstructorNotConstructable.h"
#include "JSDOMExceptionHandling.h"
#include "JSDOMWrapperCache.h"
#include "JSPluginElementFunctions.h"
#include <runtime/FunctionPrototype.h>
#include <runtime/JSCInlines.h>
#include <wtf/GetPtr.h>


namespace WebCore {
using namespace JSC;

// Attributes

JSC::EncodedJSValue jsTestPluginInterfaceConstructor(JSC::ExecState*, JSC::EncodedJSValue, JSC::PropertyName);
bool setJSTestPluginInterfaceConstructor(JSC::ExecState*, JSC::EncodedJSValue, JSC::EncodedJSValue);

class JSTestPluginInterfacePrototype : public JSC::JSNonFinalObject {
public:
    using Base = JSC::JSNonFinalObject;
    static JSTestPluginInterfacePrototype* create(JSC::VM& vm, JSDOMGlobalObject* globalObject, JSC::Structure* structure)
    {
        JSTestPluginInterfacePrototype* ptr = new (NotNull, JSC::allocateCell<JSTestPluginInterfacePrototype>(vm.heap)) JSTestPluginInterfacePrototype(vm, globalObject, structure);
        ptr->finishCreation(vm);
        return ptr;
    }

    DECLARE_INFO;
    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
    }

private:
    JSTestPluginInterfacePrototype(JSC::VM& vm, JSC::JSGlobalObject*, JSC::Structure* structure)
        : JSC::JSNonFinalObject(vm, structure)
    {
    }

    void finishCreation(JSC::VM&);
};

using JSTestPluginInterfaceConstructor = JSDOMConstructorNotConstructable<JSTestPluginInterface>;

template<> JSValue JSTestPluginInterfaceConstructor::prototypeForStructure(JSC::VM& vm, const JSDOMGlobalObject& globalObject)
{
    UNUSED_PARAM(vm);
    return globalObject.functionPrototype();
}

template<> void JSTestPluginInterfaceConstructor::initializeProperties(VM& vm, JSDOMGlobalObject& globalObject)
{
    putDirect(vm, vm.propertyNames->prototype, JSTestPluginInterface::prototype(vm, globalObject), JSC::PropertyAttribute::DontDelete | JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
    putDirect(vm, vm.propertyNames->name, jsNontrivialString(&vm, String(ASCIILiteral("TestPluginInterface"))), JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
    putDirect(vm, vm.propertyNames->length, jsNumber(0), JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
}

template<> const ClassInfo JSTestPluginInterfaceConstructor::s_info = { "TestPluginInterface", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestPluginInterfaceConstructor) };

/* Hash table for prototype */

static const HashTableValue JSTestPluginInterfacePrototypeTableValues[] =
{
    { "constructor", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsTestPluginInterfaceConstructor), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(setJSTestPluginInterfaceConstructor) } },
};

const ClassInfo JSTestPluginInterfacePrototype::s_info = { "TestPluginInterfacePrototype", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestPluginInterfacePrototype) };

void JSTestPluginInterfacePrototype::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    reifyStaticProperties(vm, JSTestPluginInterface::info(), JSTestPluginInterfacePrototypeTableValues, *this);
}

const ClassInfo JSTestPluginInterface::s_info = { "TestPluginInterface", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestPluginInterface) };

JSTestPluginInterface::JSTestPluginInterface(Structure* structure, JSDOMGlobalObject& globalObject, Ref<TestPluginInterface>&& impl)
    : JSDOMWrapper<TestPluginInterface>(structure, globalObject, WTFMove(impl))
{
}

void JSTestPluginInterface::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    ASSERT(inherits(vm, info()));

}

JSObject* JSTestPluginInterface::createPrototype(VM& vm, JSDOMGlobalObject& globalObject)
{
    return JSTestPluginInterfacePrototype::create(vm, &globalObject, JSTestPluginInterfacePrototype::createStructure(vm, &globalObject, globalObject.objectPrototype()));
}

JSObject* JSTestPluginInterface::prototype(VM& vm, JSDOMGlobalObject& globalObject)
{
    return getDOMPrototype<JSTestPluginInterface>(vm, globalObject);
}

JSValue JSTestPluginInterface::getConstructor(VM& vm, const JSGlobalObject* globalObject)
{
    return getDOMConstructor<JSTestPluginInterfaceConstructor>(vm, *jsCast<const JSDOMGlobalObject*>(globalObject));
}

void JSTestPluginInterface::destroy(JSC::JSCell* cell)
{
    JSTestPluginInterface* thisObject = static_cast<JSTestPluginInterface*>(cell);
    thisObject->JSTestPluginInterface::~JSTestPluginInterface();
}

bool JSTestPluginInterface::getOwnPropertySlot(JSObject* object, ExecState* state, PropertyName propertyName, PropertySlot& slot)
{
    auto* thisObject = jsCast<JSTestPluginInterface*>(object);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());
    if (pluginElementCustomGetOwnPropertySlot(thisObject, state, propertyName, slot))
        return true;
    return JSObject::getOwnPropertySlot(object, state, propertyName, slot);
}

bool JSTestPluginInterface::getOwnPropertySlotByIndex(JSObject* object, ExecState* state, unsigned index, PropertySlot& slot)
{
    auto* thisObject = jsCast<JSTestPluginInterface*>(object);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());
    auto propertyName = Identifier::from(state, index);
    if (pluginElementCustomGetOwnPropertySlot(thisObject, state, propertyName, slot))
        return true;
    return JSObject::getOwnPropertySlotByIndex(object, state, index, slot);
}

bool JSTestPluginInterface::put(JSCell* cell, ExecState* state, PropertyName propertyName, JSValue value, PutPropertySlot& putPropertySlot)
{
    auto* thisObject = jsCast<JSTestPluginInterface*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());

    bool putResult = false;
    if (pluginElementCustomPut(thisObject, state, propertyName, value, putPropertySlot, putResult))
        return putResult;

    return JSObject::put(thisObject, state, propertyName, value, putPropertySlot);
}

bool JSTestPluginInterface::putByIndex(JSCell* cell, ExecState* state, unsigned index, JSValue value, bool shouldThrow)
{
    auto* thisObject = jsCast<JSTestPluginInterface*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());

    auto propertyName = Identifier::from(state, index);
    PutPropertySlot putPropertySlot(thisObject, shouldThrow);
    bool putResult = false;
    if (pluginElementCustomPut(thisObject, state, propertyName, value, putPropertySlot, putResult))
        return putResult;

    return JSObject::putByIndex(cell, state, index, value, shouldThrow);
}

CallType JSTestPluginInterface::getCallData(JSCell* cell, CallData& callData)
{
    auto* thisObject = jsCast<JSTestPluginInterface*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());

    return pluginElementCustomGetCallData(thisObject, callData);
}

EncodedJSValue jsTestPluginInterfaceConstructor(ExecState* state, EncodedJSValue thisValue, PropertyName)
{
    VM& vm = state->vm();
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    auto* prototype = jsDynamicDowncast<JSTestPluginInterfacePrototype*>(vm, JSValue::decode(thisValue));
    if (UNLIKELY(!prototype))
        return throwVMTypeError(state, throwScope);
    return JSValue::encode(JSTestPluginInterface::getConstructor(state->vm(), prototype->globalObject()));
}

bool setJSTestPluginInterfaceConstructor(ExecState* state, EncodedJSValue thisValue, EncodedJSValue encodedValue)
{
    VM& vm = state->vm();
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    auto* prototype = jsDynamicDowncast<JSTestPluginInterfacePrototype*>(vm, JSValue::decode(thisValue));
    if (UNLIKELY(!prototype)) {
        throwVMTypeError(state, throwScope);
        return false;
    }
    // Shadowing a built-in constructor
    return prototype->putDirect(vm, vm.propertyNames->constructor, JSValue::decode(encodedValue));
}

bool JSTestPluginInterfaceOwner::isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown> handle, void*, SlotVisitor& visitor)
{
    UNUSED_PARAM(handle);
    UNUSED_PARAM(visitor);
    return false;
}

void JSTestPluginInterfaceOwner::finalize(JSC::Handle<JSC::Unknown> handle, void* context)
{
    auto* jsTestPluginInterface = static_cast<JSTestPluginInterface*>(handle.slot()->asCell());
    auto& world = *static_cast<DOMWrapperWorld*>(context);
    uncacheWrapper(world, &jsTestPluginInterface->wrapped(), jsTestPluginInterface);
}

#if ENABLE(BINDING_INTEGRITY)
#if PLATFORM(WIN)
#pragma warning(disable: 4483)
extern "C" { extern void (*const __identifier("??_7TestPluginInterface@WebCore@@6B@")[])(); }
#else
extern "C" { extern void* _ZTVN7WebCore19TestPluginInterfaceE[]; }
#endif
#endif

JSC::JSValue toJSNewlyCreated(JSC::ExecState*, JSDOMGlobalObject* globalObject, Ref<TestPluginInterface>&& impl)
{

#if ENABLE(BINDING_INTEGRITY)
    void* actualVTablePointer = *(reinterpret_cast<void**>(impl.ptr()));
#if PLATFORM(WIN)
    void* expectedVTablePointer = reinterpret_cast<void*>(__identifier("??_7TestPluginInterface@WebCore@@6B@"));
#else
    void* expectedVTablePointer = &_ZTVN7WebCore19TestPluginInterfaceE[2];
#endif

    // If this fails TestPluginInterface does not have a vtable, so you need to add the
    // ImplementationLacksVTable attribute to the interface definition
    static_assert(std::is_polymorphic<TestPluginInterface>::value, "TestPluginInterface is not polymorphic");

    // If you hit this assertion you either have a use after free bug, or
    // TestPluginInterface has subclasses. If TestPluginInterface has subclasses that get passed
    // to toJS() we currently require TestPluginInterface you to opt out of binding hardening
    // by adding the SkipVTableValidation attribute to the interface IDL definition
    RELEASE_ASSERT(actualVTablePointer == expectedVTablePointer);
#endif
    return createWrapper<TestPluginInterface>(globalObject, WTFMove(impl));
}

JSC::JSValue toJS(JSC::ExecState* state, JSDOMGlobalObject* globalObject, TestPluginInterface& impl)
{
    return wrap(state, globalObject, impl);
}

TestPluginInterface* JSTestPluginInterface::toWrapped(JSC::VM& vm, JSC::JSValue value)
{
    if (auto* wrapper = jsDynamicDowncast<JSTestPluginInterface*>(vm, value))
        return &wrapper->wrapped();
    return nullptr;
}

}
