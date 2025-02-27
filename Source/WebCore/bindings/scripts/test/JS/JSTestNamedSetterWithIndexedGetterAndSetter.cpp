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
#include "JSTestNamedSetterWithIndexedGetterAndSetter.h"

#include "JSDOMAbstractOperations.h"
#include "JSDOMBinding.h"
#include "JSDOMConstructorNotConstructable.h"
#include "JSDOMConvertNumbers.h"
#include "JSDOMConvertStrings.h"
#include "JSDOMExceptionHandling.h"
#include "JSDOMOperation.h"
#include "JSDOMWrapperCache.h"
#include "URL.h"
#include <runtime/FunctionPrototype.h>
#include <runtime/JSCInlines.h>
#include <runtime/PropertyNameArray.h>
#include <wtf/GetPtr.h>


namespace WebCore {
using namespace JSC;

// Functions

JSC::EncodedJSValue JSC_HOST_CALL jsTestNamedSetterWithIndexedGetterAndSetterPrototypeFunctionNamedSetter(JSC::ExecState*);
JSC::EncodedJSValue JSC_HOST_CALL jsTestNamedSetterWithIndexedGetterAndSetterPrototypeFunctionIndexedSetter(JSC::ExecState*);

// Attributes

JSC::EncodedJSValue jsTestNamedSetterWithIndexedGetterAndSetterConstructor(JSC::ExecState*, JSC::EncodedJSValue, JSC::PropertyName);
bool setJSTestNamedSetterWithIndexedGetterAndSetterConstructor(JSC::ExecState*, JSC::EncodedJSValue, JSC::EncodedJSValue);

class JSTestNamedSetterWithIndexedGetterAndSetterPrototype : public JSC::JSNonFinalObject {
public:
    using Base = JSC::JSNonFinalObject;
    static JSTestNamedSetterWithIndexedGetterAndSetterPrototype* create(JSC::VM& vm, JSDOMGlobalObject* globalObject, JSC::Structure* structure)
    {
        JSTestNamedSetterWithIndexedGetterAndSetterPrototype* ptr = new (NotNull, JSC::allocateCell<JSTestNamedSetterWithIndexedGetterAndSetterPrototype>(vm.heap)) JSTestNamedSetterWithIndexedGetterAndSetterPrototype(vm, globalObject, structure);
        ptr->finishCreation(vm);
        return ptr;
    }

    DECLARE_INFO;
    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
    }

private:
    JSTestNamedSetterWithIndexedGetterAndSetterPrototype(JSC::VM& vm, JSC::JSGlobalObject*, JSC::Structure* structure)
        : JSC::JSNonFinalObject(vm, structure)
    {
    }

    void finishCreation(JSC::VM&);
};

using JSTestNamedSetterWithIndexedGetterAndSetterConstructor = JSDOMConstructorNotConstructable<JSTestNamedSetterWithIndexedGetterAndSetter>;

template<> JSValue JSTestNamedSetterWithIndexedGetterAndSetterConstructor::prototypeForStructure(JSC::VM& vm, const JSDOMGlobalObject& globalObject)
{
    UNUSED_PARAM(vm);
    return globalObject.functionPrototype();
}

template<> void JSTestNamedSetterWithIndexedGetterAndSetterConstructor::initializeProperties(VM& vm, JSDOMGlobalObject& globalObject)
{
    putDirect(vm, vm.propertyNames->prototype, JSTestNamedSetterWithIndexedGetterAndSetter::prototype(vm, globalObject), JSC::PropertyAttribute::DontDelete | JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
    putDirect(vm, vm.propertyNames->name, jsNontrivialString(&vm, String(ASCIILiteral("TestNamedSetterWithIndexedGetterAndSetter"))), JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
    putDirect(vm, vm.propertyNames->length, jsNumber(0), JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
}

template<> const ClassInfo JSTestNamedSetterWithIndexedGetterAndSetterConstructor::s_info = { "TestNamedSetterWithIndexedGetterAndSetter", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestNamedSetterWithIndexedGetterAndSetterConstructor) };

/* Hash table for prototype */

static const HashTableValue JSTestNamedSetterWithIndexedGetterAndSetterPrototypeTableValues[] =
{
    { "constructor", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsTestNamedSetterWithIndexedGetterAndSetterConstructor), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(setJSTestNamedSetterWithIndexedGetterAndSetterConstructor) } },
    { "namedSetter", static_cast<unsigned>(JSC::PropertyAttribute::Function), NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(jsTestNamedSetterWithIndexedGetterAndSetterPrototypeFunctionNamedSetter), (intptr_t) (2) } },
    { "indexedSetter", static_cast<unsigned>(JSC::PropertyAttribute::Function), NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(jsTestNamedSetterWithIndexedGetterAndSetterPrototypeFunctionIndexedSetter), (intptr_t) (1) } },
};

const ClassInfo JSTestNamedSetterWithIndexedGetterAndSetterPrototype::s_info = { "TestNamedSetterWithIndexedGetterAndSetterPrototype", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestNamedSetterWithIndexedGetterAndSetterPrototype) };

void JSTestNamedSetterWithIndexedGetterAndSetterPrototype::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    reifyStaticProperties(vm, JSTestNamedSetterWithIndexedGetterAndSetter::info(), JSTestNamedSetterWithIndexedGetterAndSetterPrototypeTableValues, *this);
}

const ClassInfo JSTestNamedSetterWithIndexedGetterAndSetter::s_info = { "TestNamedSetterWithIndexedGetterAndSetter", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestNamedSetterWithIndexedGetterAndSetter) };

JSTestNamedSetterWithIndexedGetterAndSetter::JSTestNamedSetterWithIndexedGetterAndSetter(Structure* structure, JSDOMGlobalObject& globalObject, Ref<TestNamedSetterWithIndexedGetterAndSetter>&& impl)
    : JSDOMWrapper<TestNamedSetterWithIndexedGetterAndSetter>(structure, globalObject, WTFMove(impl))
{
}

void JSTestNamedSetterWithIndexedGetterAndSetter::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    ASSERT(inherits(vm, info()));

}

JSObject* JSTestNamedSetterWithIndexedGetterAndSetter::createPrototype(VM& vm, JSDOMGlobalObject& globalObject)
{
    return JSTestNamedSetterWithIndexedGetterAndSetterPrototype::create(vm, &globalObject, JSTestNamedSetterWithIndexedGetterAndSetterPrototype::createStructure(vm, &globalObject, globalObject.objectPrototype()));
}

JSObject* JSTestNamedSetterWithIndexedGetterAndSetter::prototype(VM& vm, JSDOMGlobalObject& globalObject)
{
    return getDOMPrototype<JSTestNamedSetterWithIndexedGetterAndSetter>(vm, globalObject);
}

JSValue JSTestNamedSetterWithIndexedGetterAndSetter::getConstructor(VM& vm, const JSGlobalObject* globalObject)
{
    return getDOMConstructor<JSTestNamedSetterWithIndexedGetterAndSetterConstructor>(vm, *jsCast<const JSDOMGlobalObject*>(globalObject));
}

void JSTestNamedSetterWithIndexedGetterAndSetter::destroy(JSC::JSCell* cell)
{
    JSTestNamedSetterWithIndexedGetterAndSetter* thisObject = static_cast<JSTestNamedSetterWithIndexedGetterAndSetter*>(cell);
    thisObject->JSTestNamedSetterWithIndexedGetterAndSetter::~JSTestNamedSetterWithIndexedGetterAndSetter();
}

bool JSTestNamedSetterWithIndexedGetterAndSetter::getOwnPropertySlot(JSObject* object, ExecState* state, PropertyName propertyName, PropertySlot& slot)
{
    auto* thisObject = jsCast<JSTestNamedSetterWithIndexedGetterAndSetter*>(object);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());
    if (auto index = parseIndex(propertyName)) {
        if (index.value() < thisObject->wrapped().length()) {
            auto value = toJS<IDLDOMString>(*state, thisObject->wrapped().indexedSetter(index.value()));
            slot.setValue(thisObject, static_cast<unsigned>(0), value);
            return true;
        }
        return JSObject::getOwnPropertySlot(object, state, propertyName, slot);
    }
    using GetterIDLType = IDLDOMString;
    auto getterFunctor = [] (auto& thisObject, auto propertyName) -> std::optional<typename GetterIDLType::ImplementationType> {
        auto result = thisObject.wrapped().namedItem(propertyNameToAtomicString(propertyName));
        if (!GetterIDLType::isNullValue(result))
            return typename GetterIDLType::ImplementationType { GetterIDLType::extractValueFromNullable(result) };
        return std::nullopt;
    };
    if (auto namedProperty = accessVisibleNamedProperty<OverrideBuiltins::No>(*state, *thisObject, propertyName, getterFunctor)) {
        auto value = toJS<IDLDOMString>(*state, WTFMove(namedProperty.value()));
        slot.setValue(thisObject, static_cast<unsigned>(0), value);
        return true;
    }
    return JSObject::getOwnPropertySlot(object, state, propertyName, slot);
}

bool JSTestNamedSetterWithIndexedGetterAndSetter::getOwnPropertySlotByIndex(JSObject* object, ExecState* state, unsigned index, PropertySlot& slot)
{
    auto* thisObject = jsCast<JSTestNamedSetterWithIndexedGetterAndSetter*>(object);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());
    if (LIKELY(index <= MAX_ARRAY_INDEX)) {
        if (index < thisObject->wrapped().length()) {
            auto value = toJS<IDLDOMString>(*state, thisObject->wrapped().indexedSetter(index));
            slot.setValue(thisObject, static_cast<unsigned>(0), value);
            return true;
        }
        return JSObject::getOwnPropertySlotByIndex(object, state, index, slot);
    }
    auto propertyName = Identifier::from(state, index);
    using GetterIDLType = IDLDOMString;
    auto getterFunctor = [] (auto& thisObject, auto propertyName) -> std::optional<typename GetterIDLType::ImplementationType> {
        auto result = thisObject.wrapped().namedItem(propertyNameToAtomicString(propertyName));
        if (!GetterIDLType::isNullValue(result))
            return typename GetterIDLType::ImplementationType { GetterIDLType::extractValueFromNullable(result) };
        return std::nullopt;
    };
    if (auto namedProperty = accessVisibleNamedProperty<OverrideBuiltins::No>(*state, *thisObject, propertyName, getterFunctor)) {
        auto value = toJS<IDLDOMString>(*state, WTFMove(namedProperty.value()));
        slot.setValue(thisObject, static_cast<unsigned>(0), value);
        return true;
    }
    return JSObject::getOwnPropertySlotByIndex(object, state, index, slot);
}

void JSTestNamedSetterWithIndexedGetterAndSetter::getOwnPropertyNames(JSObject* object, ExecState* state, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    auto* thisObject = jsCast<JSTestNamedSetterWithIndexedGetterAndSetter*>(object);
    ASSERT_GC_OBJECT_INHERITS(object, info());
    for (unsigned i = 0, count = thisObject->wrapped().length(); i < count; ++i)
        propertyNames.add(Identifier::from(state, i));
    for (auto& propertyName : thisObject->wrapped().supportedPropertyNames())
        propertyNames.add(Identifier::fromString(state, propertyName));
    JSObject::getOwnPropertyNames(object, state, propertyNames, mode);
}

bool JSTestNamedSetterWithIndexedGetterAndSetter::put(JSCell* cell, ExecState* state, PropertyName propertyName, JSValue value, PutPropertySlot& putPropertySlot)
{
    auto* thisObject = jsCast<JSTestNamedSetterWithIndexedGetterAndSetter*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());

    if (auto index = parseIndex(propertyName)) {
        auto throwScope = DECLARE_THROW_SCOPE(state->vm());
        auto nativeValue = convert<IDLDOMString>(*state, value);
        RETURN_IF_EXCEPTION(throwScope, true);
        thisObject->wrapped().indexedSetter(index.value(), WTFMove(nativeValue));
        return true;
    }

    if (!propertyName.isSymbol()) {
        PropertySlot slot { thisObject, PropertySlot::InternalMethodType::VMInquiry };
        JSValue prototype = thisObject->getPrototypeDirect(state->vm());
        if (!(prototype.isObject() && asObject(prototype)->getPropertySlot(state, propertyName, slot))) {
            auto throwScope = DECLARE_THROW_SCOPE(state->vm());
            auto nativeValue = convert<IDLDOMString>(*state, value);
            RETURN_IF_EXCEPTION(throwScope, true);
            thisObject->wrapped().namedSetter(propertyNameToString(propertyName), WTFMove(nativeValue));
            return true;
        }
    }

    return JSObject::put(thisObject, state, propertyName, value, putPropertySlot);
}

bool JSTestNamedSetterWithIndexedGetterAndSetter::putByIndex(JSCell* cell, ExecState* state, unsigned index, JSValue value, bool shouldThrow)
{
    auto* thisObject = jsCast<JSTestNamedSetterWithIndexedGetterAndSetter*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());

    if (LIKELY(index <= MAX_ARRAY_INDEX)) {
        auto throwScope = DECLARE_THROW_SCOPE(state->vm());
        auto nativeValue = convert<IDLDOMString>(*state, value);
        RETURN_IF_EXCEPTION(throwScope, true);
        thisObject->wrapped().indexedSetter(index, WTFMove(nativeValue));
        return true;
    }

    auto propertyName = Identifier::from(state, index);
    PropertySlot slot { thisObject, PropertySlot::InternalMethodType::VMInquiry };
    JSValue prototype = thisObject->getPrototypeDirect(state->vm());
    if (!(prototype.isObject() && asObject(prototype)->getPropertySlot(state, propertyName, slot))) {
        auto throwScope = DECLARE_THROW_SCOPE(state->vm());
        auto nativeValue = convert<IDLDOMString>(*state, value);
        RETURN_IF_EXCEPTION(throwScope, true);
        thisObject->wrapped().namedSetter(propertyNameToString(propertyName), WTFMove(nativeValue));
        return true;
    }

    return JSObject::putByIndex(cell, state, index, value, shouldThrow);
}

bool JSTestNamedSetterWithIndexedGetterAndSetter::defineOwnProperty(JSObject* object, ExecState* state, PropertyName propertyName, const PropertyDescriptor& propertyDescriptor, bool shouldThrow)
{
    auto* thisObject = jsCast<JSTestNamedSetterWithIndexedGetterAndSetter*>(object);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());

    if (auto index = parseIndex(propertyName)) {
        if (!propertyDescriptor.isDataDescriptor())
            return false;
        auto throwScope = DECLARE_THROW_SCOPE(state->vm());
        auto nativeValue = convert<IDLDOMString>(*state, propertyDescriptor.value());
        RETURN_IF_EXCEPTION(throwScope, true);
        thisObject->wrapped().indexedSetter(index.value(), WTFMove(nativeValue));
        return true;
    }

    if (!propertyName.isSymbol()) {
        PropertySlot slot { thisObject, PropertySlot::InternalMethodType::VMInquiry };
        if (!JSObject::getOwnPropertySlot(thisObject, state, propertyName, slot)) {
            if (!propertyDescriptor.isDataDescriptor())
                return false;
            auto throwScope = DECLARE_THROW_SCOPE(state->vm());
            auto nativeValue = convert<IDLDOMString>(*state, propertyDescriptor.value());
            RETURN_IF_EXCEPTION(throwScope, true);
            thisObject->wrapped().namedSetter(propertyNameToString(propertyName), WTFMove(nativeValue));
            return true;
        }
    }

    PropertyDescriptor newPropertyDescriptor = propertyDescriptor;
    newPropertyDescriptor.setConfigurable(true);
    return JSObject::defineOwnProperty(object, state, propertyName, newPropertyDescriptor, shouldThrow);
}

template<> inline JSTestNamedSetterWithIndexedGetterAndSetter* IDLOperation<JSTestNamedSetterWithIndexedGetterAndSetter>::cast(ExecState& state)
{
    return jsDynamicDowncast<JSTestNamedSetterWithIndexedGetterAndSetter*>(state.vm(), state.thisValue());
}

EncodedJSValue jsTestNamedSetterWithIndexedGetterAndSetterConstructor(ExecState* state, EncodedJSValue thisValue, PropertyName)
{
    VM& vm = state->vm();
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    auto* prototype = jsDynamicDowncast<JSTestNamedSetterWithIndexedGetterAndSetterPrototype*>(vm, JSValue::decode(thisValue));
    if (UNLIKELY(!prototype))
        return throwVMTypeError(state, throwScope);
    return JSValue::encode(JSTestNamedSetterWithIndexedGetterAndSetter::getConstructor(state->vm(), prototype->globalObject()));
}

bool setJSTestNamedSetterWithIndexedGetterAndSetterConstructor(ExecState* state, EncodedJSValue thisValue, EncodedJSValue encodedValue)
{
    VM& vm = state->vm();
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    auto* prototype = jsDynamicDowncast<JSTestNamedSetterWithIndexedGetterAndSetterPrototype*>(vm, JSValue::decode(thisValue));
    if (UNLIKELY(!prototype)) {
        throwVMTypeError(state, throwScope);
        return false;
    }
    // Shadowing a built-in constructor
    return prototype->putDirect(vm, vm.propertyNames->constructor, JSValue::decode(encodedValue));
}

static inline JSC::EncodedJSValue jsTestNamedSetterWithIndexedGetterAndSetterPrototypeFunctionNamedSetterBody(JSC::ExecState* state, typename IDLOperation<JSTestNamedSetterWithIndexedGetterAndSetter>::ClassParameter castedThis, JSC::ThrowScope& throwScope)
{
    UNUSED_PARAM(state);
    UNUSED_PARAM(throwScope);
    auto& impl = castedThis->wrapped();
    if (UNLIKELY(state->argumentCount() < 2))
        return throwVMError(state, throwScope, createNotEnoughArgumentsError(state));
    auto name = convert<IDLDOMString>(*state, state->uncheckedArgument(0));
    RETURN_IF_EXCEPTION(throwScope, encodedJSValue());
    auto value = convert<IDLDOMString>(*state, state->uncheckedArgument(1));
    RETURN_IF_EXCEPTION(throwScope, encodedJSValue());
    impl.namedSetter(WTFMove(name), WTFMove(value));
    return JSValue::encode(jsUndefined());
}

EncodedJSValue JSC_HOST_CALL jsTestNamedSetterWithIndexedGetterAndSetterPrototypeFunctionNamedSetter(ExecState* state)
{
    return IDLOperation<JSTestNamedSetterWithIndexedGetterAndSetter>::call<jsTestNamedSetterWithIndexedGetterAndSetterPrototypeFunctionNamedSetterBody>(*state, "namedSetter");
}

static inline JSC::EncodedJSValue jsTestNamedSetterWithIndexedGetterAndSetterPrototypeFunctionIndexedSetter1Body(JSC::ExecState* state, typename IDLOperation<JSTestNamedSetterWithIndexedGetterAndSetter>::ClassParameter castedThis, JSC::ThrowScope& throwScope)
{
    UNUSED_PARAM(state);
    UNUSED_PARAM(throwScope);
    auto& impl = castedThis->wrapped();
    auto index = convert<IDLUnsignedLong>(*state, state->uncheckedArgument(0));
    RETURN_IF_EXCEPTION(throwScope, encodedJSValue());
    auto value = convert<IDLDOMString>(*state, state->uncheckedArgument(1));
    RETURN_IF_EXCEPTION(throwScope, encodedJSValue());
    impl.indexedSetter(WTFMove(index), WTFMove(value));
    return JSValue::encode(jsUndefined());
}

static inline JSC::EncodedJSValue jsTestNamedSetterWithIndexedGetterAndSetterPrototypeFunctionIndexedSetter2Body(JSC::ExecState* state, typename IDLOperation<JSTestNamedSetterWithIndexedGetterAndSetter>::ClassParameter castedThis, JSC::ThrowScope& throwScope)
{
    UNUSED_PARAM(state);
    UNUSED_PARAM(throwScope);
    auto& impl = castedThis->wrapped();
    auto index = convert<IDLUnsignedLong>(*state, state->uncheckedArgument(0));
    RETURN_IF_EXCEPTION(throwScope, encodedJSValue());
    return JSValue::encode(toJS<IDLDOMString>(*state, impl.indexedSetter(WTFMove(index))));
}

static inline JSC::EncodedJSValue jsTestNamedSetterWithIndexedGetterAndSetterPrototypeFunctionIndexedSetterOverloadDispatcher(JSC::ExecState* state, typename IDLOperation<JSTestNamedSetterWithIndexedGetterAndSetter>::ClassParameter castedThis, JSC::ThrowScope& throwScope)
{
    UNUSED_PARAM(state);
    UNUSED_PARAM(throwScope);
    VM& vm = state->vm();
    UNUSED_PARAM(vm);
    size_t argsCount = std::min<size_t>(2, state->argumentCount());
    if (argsCount == 1) {
        return jsTestNamedSetterWithIndexedGetterAndSetterPrototypeFunctionIndexedSetter2Body(state, castedThis, throwScope);
    }
    if (argsCount == 2) {
        return jsTestNamedSetterWithIndexedGetterAndSetterPrototypeFunctionIndexedSetter1Body(state, castedThis, throwScope);
    }
    return argsCount < 1 ? throwVMError(state, throwScope, createNotEnoughArgumentsError(state)) : throwVMTypeError(state, throwScope);
}

EncodedJSValue JSC_HOST_CALL jsTestNamedSetterWithIndexedGetterAndSetterPrototypeFunctionIndexedSetter(ExecState* state)
{
    return IDLOperation<JSTestNamedSetterWithIndexedGetterAndSetter>::call<jsTestNamedSetterWithIndexedGetterAndSetterPrototypeFunctionIndexedSetterOverloadDispatcher>(*state, "indexedSetter");
}

bool JSTestNamedSetterWithIndexedGetterAndSetterOwner::isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown> handle, void*, SlotVisitor& visitor)
{
    UNUSED_PARAM(handle);
    UNUSED_PARAM(visitor);
    return false;
}

void JSTestNamedSetterWithIndexedGetterAndSetterOwner::finalize(JSC::Handle<JSC::Unknown> handle, void* context)
{
    auto* jsTestNamedSetterWithIndexedGetterAndSetter = static_cast<JSTestNamedSetterWithIndexedGetterAndSetter*>(handle.slot()->asCell());
    auto& world = *static_cast<DOMWrapperWorld*>(context);
    uncacheWrapper(world, &jsTestNamedSetterWithIndexedGetterAndSetter->wrapped(), jsTestNamedSetterWithIndexedGetterAndSetter);
}

#if ENABLE(BINDING_INTEGRITY)
#if PLATFORM(WIN)
#pragma warning(disable: 4483)
extern "C" { extern void (*const __identifier("??_7TestNamedSetterWithIndexedGetterAndSetter@WebCore@@6B@")[])(); }
#else
extern "C" { extern void* _ZTVN7WebCore41TestNamedSetterWithIndexedGetterAndSetterE[]; }
#endif
#endif

JSC::JSValue toJSNewlyCreated(JSC::ExecState*, JSDOMGlobalObject* globalObject, Ref<TestNamedSetterWithIndexedGetterAndSetter>&& impl)
{

#if ENABLE(BINDING_INTEGRITY)
    void* actualVTablePointer = *(reinterpret_cast<void**>(impl.ptr()));
#if PLATFORM(WIN)
    void* expectedVTablePointer = reinterpret_cast<void*>(__identifier("??_7TestNamedSetterWithIndexedGetterAndSetter@WebCore@@6B@"));
#else
    void* expectedVTablePointer = &_ZTVN7WebCore41TestNamedSetterWithIndexedGetterAndSetterE[2];
#endif

    // If this fails TestNamedSetterWithIndexedGetterAndSetter does not have a vtable, so you need to add the
    // ImplementationLacksVTable attribute to the interface definition
    static_assert(std::is_polymorphic<TestNamedSetterWithIndexedGetterAndSetter>::value, "TestNamedSetterWithIndexedGetterAndSetter is not polymorphic");

    // If you hit this assertion you either have a use after free bug, or
    // TestNamedSetterWithIndexedGetterAndSetter has subclasses. If TestNamedSetterWithIndexedGetterAndSetter has subclasses that get passed
    // to toJS() we currently require TestNamedSetterWithIndexedGetterAndSetter you to opt out of binding hardening
    // by adding the SkipVTableValidation attribute to the interface IDL definition
    RELEASE_ASSERT(actualVTablePointer == expectedVTablePointer);
#endif
    return createWrapper<TestNamedSetterWithIndexedGetterAndSetter>(globalObject, WTFMove(impl));
}

JSC::JSValue toJS(JSC::ExecState* state, JSDOMGlobalObject* globalObject, TestNamedSetterWithIndexedGetterAndSetter& impl)
{
    return wrap(state, globalObject, impl);
}

TestNamedSetterWithIndexedGetterAndSetter* JSTestNamedSetterWithIndexedGetterAndSetter::toWrapped(JSC::VM& vm, JSC::JSValue value)
{
    if (auto* wrapper = jsDynamicDowncast<JSTestNamedSetterWithIndexedGetterAndSetter*>(vm, value))
        return &wrapper->wrapped();
    return nullptr;
}

}
