/*
 * Copyright (C) 2015-2017 Apple Inc. All rights reserved.
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

#pragma once

#include "GenericArguments.h"
#include "JSCInlines.h"

namespace JSC {

template<typename Type>
void GenericArguments<Type>::visitChildren(JSCell* thisCell, SlotVisitor& visitor)
{
    Type* thisObject = static_cast<Type*>(thisCell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());
    
    if (thisObject->m_modifiedArgumentsDescriptor)
        visitor.markAuxiliary(thisObject->m_modifiedArgumentsDescriptor.get());
}

template<typename Type>
bool GenericArguments<Type>::getOwnPropertySlot(JSObject* object, ExecState* exec, PropertyName ident, PropertySlot& slot)
{
    Type* thisObject = jsCast<Type*>(object);
    VM& vm = exec->vm();
    
    if (!thisObject->overrodeThings()) {
        if (ident == vm.propertyNames->length) {
            slot.setValue(thisObject, static_cast<unsigned>(PropertyAttribute::DontEnum), jsNumber(thisObject->internalLength()));
            return true;
        }
        if (ident == vm.propertyNames->callee) {
            slot.setValue(thisObject, static_cast<unsigned>(PropertyAttribute::DontEnum), thisObject->callee().get());
            return true;
        }
        if (ident == vm.propertyNames->iteratorSymbol) {
            slot.setValue(thisObject, static_cast<unsigned>(PropertyAttribute::DontEnum), thisObject->globalObject()->arrayProtoValuesFunction());
            return true;
        }
    }
    
    if (std::optional<uint32_t> index = parseIndex(ident))
        return GenericArguments<Type>::getOwnPropertySlotByIndex(thisObject, exec, *index, slot);

    return Base::getOwnPropertySlot(thisObject, exec, ident, slot);
}

template<typename Type>
bool GenericArguments<Type>::getOwnPropertySlotByIndex(JSObject* object, ExecState* exec, unsigned index, PropertySlot& slot)
{
    Type* thisObject = jsCast<Type*>(object);
    
    if (!thisObject->isModifiedArgumentDescriptor(index) && thisObject->isMappedArgument(index)) {
        slot.setValue(thisObject, static_cast<unsigned>(PropertyAttribute::None), thisObject->getIndexQuickly(index));
        return true;
    }
    
    bool result = Base::getOwnPropertySlotByIndex(object, exec, index, slot);
    
    if (thisObject->isMappedArgument(index)) {
        ASSERT(result);
        slot.setValue(thisObject, slot.attributes(), thisObject->getIndexQuickly(index));
        return true;
    }
    
    return result;
}

template<typename Type>
void GenericArguments<Type>::getOwnPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& array, EnumerationMode mode)
{
    Type* thisObject = jsCast<Type*>(object);

    if (array.includeStringProperties()) {
        for (unsigned i = 0; i < thisObject->internalLength(); ++i) {
            if (!thisObject->isMappedArgument(i))
                continue;
            array.add(Identifier::from(exec, i));
        }
    }

    if (mode.includeDontEnumProperties() && !thisObject->overrodeThings()) {
        VM& vm = exec->vm();
        array.add(vm.propertyNames->length);
        array.add(vm.propertyNames->callee);
        if (array.includeSymbolProperties())
            array.add(vm.propertyNames->iteratorSymbol);
    }
    Base::getOwnPropertyNames(thisObject, exec, array, mode);
}

template<typename Type>
bool GenericArguments<Type>::put(JSCell* cell, ExecState* exec, PropertyName ident, JSValue value, PutPropertySlot& slot)
{
    Type* thisObject = jsCast<Type*>(cell);
    VM& vm = exec->vm();
    
    if (!thisObject->overrodeThings()
        && (ident == vm.propertyNames->length
            || ident == vm.propertyNames->callee
            || ident == vm.propertyNames->iteratorSymbol)) {
        thisObject->overrideThings(vm);
        PutPropertySlot dummy = slot; // This put is not cacheable, so we shadow the slot that was given to us.
        return Base::put(thisObject, exec, ident, value, dummy);
    }

    // https://tc39.github.io/ecma262/#sec-arguments-exotic-objects-set-p-v-receiver
    // Fall back to the OrdinarySet when the receiver is altered from the thisObject.
    if (UNLIKELY(isThisValueAltered(slot, thisObject)))
        return ordinarySetSlow(exec, thisObject, ident, value, slot.thisValue(), slot.isStrictMode());
    
    std::optional<uint32_t> index = parseIndex(ident);
    if (index && thisObject->isMappedArgument(index.value())) {
        thisObject->setIndexQuickly(vm, index.value(), value);
        return true;
    }
    
    return Base::put(thisObject, exec, ident, value, slot);
}

template<typename Type>
bool GenericArguments<Type>::putByIndex(JSCell* cell, ExecState* exec, unsigned index, JSValue value, bool shouldThrow)
{
    Type* thisObject = jsCast<Type*>(cell);
    VM& vm = exec->vm();

    if (thisObject->isMappedArgument(index)) {
        thisObject->setIndexQuickly(vm, index, value);
        return true;
    }
    
    return Base::putByIndex(cell, exec, index, value, shouldThrow);
}

template<typename Type>
bool GenericArguments<Type>::deleteProperty(JSCell* cell, ExecState* exec, PropertyName ident)
{
    Type* thisObject = jsCast<Type*>(cell);
    VM& vm = exec->vm();
    
    if (!thisObject->overrodeThings()
        && (ident == vm.propertyNames->length
            || ident == vm.propertyNames->callee
            || ident == vm.propertyNames->iteratorSymbol))
        thisObject->overrideThings(vm);
    
    if (std::optional<uint32_t> index = parseIndex(ident))
        return GenericArguments<Type>::deletePropertyByIndex(thisObject, exec, *index);

    return Base::deleteProperty(thisObject, exec, ident);
}

template<typename Type>
bool GenericArguments<Type>::deletePropertyByIndex(JSCell* cell, ExecState* exec, unsigned index)
{
    Type* thisObject = jsCast<Type*>(cell);
    VM& vm = exec->vm();

    bool propertyMightBeInJSObjectStorage = thisObject->isModifiedArgumentDescriptor(index) || !thisObject->isMappedArgument(index);
    bool deletedProperty = true;
    if (propertyMightBeInJSObjectStorage)
        deletedProperty = Base::deletePropertyByIndex(cell, exec, index);

    if (deletedProperty) {
        // Deleting an indexed property unconditionally unmaps it.
        if (thisObject->isMappedArgument(index)) {
            // We need to check that the property was mapped so we don't write to random memory.
            thisObject->unmapArgument(vm, index);
        }
        thisObject->setModifiedArgumentDescriptor(vm, index);
    }

    return deletedProperty;
}

template<typename Type>
bool GenericArguments<Type>::defineOwnProperty(JSObject* object, ExecState* exec, PropertyName ident, const PropertyDescriptor& descriptor, bool shouldThrow)
{
    Type* thisObject = jsCast<Type*>(object);
    VM& vm = exec->vm();
    auto scope = DECLARE_THROW_SCOPE(vm);
    
    if (ident == vm.propertyNames->length
        || ident == vm.propertyNames->callee
        || ident == vm.propertyNames->iteratorSymbol)
        thisObject->overrideThingsIfNecessary(vm);
    else {
        std::optional<uint32_t> optionalIndex = parseIndex(ident);
        if (optionalIndex) {
            uint32_t index = optionalIndex.value();
            if (!descriptor.isAccessorDescriptor() && thisObject->isMappedArgument(optionalIndex.value())) {
                // If the property is not deleted and we are using a non-accessor descriptor, then
                // make sure that the aliased argument sees the value.
                if (descriptor.value())
                    thisObject->setIndexQuickly(vm, index, descriptor.value());
            
                // If the property is not deleted and we are using a non-accessor, writable,
                // configurable and enumerable descriptor and isn't modified, then we are done.
                // The argument continues to be aliased.
                if (descriptor.writable() && descriptor.configurable() && descriptor.enumerable() && !thisObject->isModifiedArgumentDescriptor(index))
                    return true;
                
                if (!thisObject->isModifiedArgumentDescriptor(index)) {
                    // If it is a new entry, we need to put direct to initialize argument[i] descriptor properly
                    JSValue value = thisObject->getIndexQuickly(index);
                    ASSERT(value);
                    object->putDirectMayBeIndex(exec, ident, value);
                    scope.assertNoException();

                    thisObject->setModifiedArgumentDescriptor(vm, index);
                }
            }
            
            if (thisObject->isMappedArgument(index)) {
                // Just unmap arguments if its descriptor contains {writable: false}.
                // Check https://tc39.github.io/ecma262/#sec-createunmappedargumentsobject
                // and https://tc39.github.io/ecma262/#sec-createmappedargumentsobject to verify that all data
                // property from arguments object are {writable: true, configurable: true, enumerable: true} by default
                if ((descriptor.writablePresent() && !descriptor.writable()) || descriptor.isAccessorDescriptor()) {
                    if (!descriptor.isAccessorDescriptor()) {
                        JSValue value = thisObject->getIndexQuickly(index);
                        ASSERT(value);
                        object->putDirectMayBeIndex(exec, ident, value);
                        scope.assertNoException();
                    }
                    thisObject->unmapArgument(vm, index);
                    thisObject->setModifiedArgumentDescriptor(vm, index);
                }
            }
        }
    }

    // Now just let the normal object machinery do its thing.
    scope.release();
    return Base::defineOwnProperty(object, exec, ident, descriptor, shouldThrow);
}

template<typename Type>
void GenericArguments<Type>::initModifiedArgumentsDescriptor(VM& vm, unsigned argsLength)
{
    RELEASE_ASSERT(!m_modifiedArgumentsDescriptor);

    if (argsLength) {
        void* backingStore = vm.gigacageAuxiliarySpace(m_modifiedArgumentsDescriptor.kind).allocateNonVirtual(WTF::roundUpToMultipleOf<8>(argsLength), nullptr, AllocationFailureMode::Assert);
        bool* modifiedArguments = static_cast<bool*>(backingStore);
        m_modifiedArgumentsDescriptor.set(vm, this, modifiedArguments);
        for (unsigned i = argsLength; i--;)
            modifiedArguments[i] = false;
    }
}

template<typename Type>
void GenericArguments<Type>::initModifiedArgumentsDescriptorIfNecessary(VM& vm, unsigned argsLength)
{
    if (!m_modifiedArgumentsDescriptor)
        initModifiedArgumentsDescriptor(vm, argsLength);
}

template<typename Type>
void GenericArguments<Type>::setModifiedArgumentDescriptor(VM& vm, unsigned index, unsigned length)
{
    initModifiedArgumentsDescriptorIfNecessary(vm, length);
    if (index < length)
        m_modifiedArgumentsDescriptor[index] = true;
}

template<typename Type>
bool GenericArguments<Type>::isModifiedArgumentDescriptor(unsigned index, unsigned length)
{
    if (!m_modifiedArgumentsDescriptor)
        return false;
    if (index < length)
        return m_modifiedArgumentsDescriptor[index];
    return false;
}

template<typename Type>
void GenericArguments<Type>::copyToArguments(ExecState* exec, VirtualRegister firstElementDest, unsigned offset, unsigned length)
{
    VM& vm = exec->vm();
    auto scope = DECLARE_THROW_SCOPE(vm);

    Type* thisObject = static_cast<Type*>(this);
    for (unsigned i = 0; i < length; ++i) {
        if (thisObject->isMappedArgument(i + offset))
            exec->r(firstElementDest + i) = thisObject->getIndexQuickly(i + offset);
        else {
            exec->r(firstElementDest + i) = get(exec, i + offset);
            RETURN_IF_EXCEPTION(scope, void());
        }
    }
}

} // namespace JSC
