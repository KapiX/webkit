/*
 * Copyright (C) 2007-2017 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "JSDOMWindowCustom.h"

#include "DOMWindowIndexedDatabase.h"
#include "Frame.h"
#include "HTMLCollection.h"
#include "HTMLDocument.h"
#include "HTMLFrameOwnerElement.h"
#include "JSDOMBindingSecurity.h"
#include "JSDOMConvertNullable.h"
#include "JSDOMConvertNumbers.h"
#include "JSDOMConvertStrings.h"
#include "JSDOMWindowProxy.h"
#include "JSEvent.h"
#include "JSEventListener.h"
#include "JSHTMLAudioElement.h"
#include "JSHTMLCollection.h"
#include "JSHTMLOptionElement.h"
#include "JSIDBFactory.h"
#include "JSWorker.h"
#include "Location.h"
#include "RuntimeEnabledFeatures.h"
#include "ScheduledAction.h"
#include "Settings.h"
#include "WebCoreJSClientData.h"
#include <runtime/JSCInlines.h>
#include <runtime/Lookup.h>

#if ENABLE(USER_MESSAGE_HANDLERS)
#include "JSWebKitNamespace.h"
#endif


namespace WebCore {
using namespace JSC;

EncodedJSValue JSC_HOST_CALL jsDOMWindowInstanceFunctionShowModalDialog(ExecState*);

void JSDOMWindow::visitAdditionalChildren(SlotVisitor& visitor)
{
    if (Frame* frame = wrapped().frame())
        visitor.addOpaqueRoot(frame);
    
    // Normally JSEventTargetCustom.cpp's JSEventTarget::visitAdditionalChildren() would call this. But
    // even though DOMWindow is an EventTarget, JSDOMWindow does not subclass JSEventTarget, so we need
    // to do this here.
    wrapped().visitJSEventListeners(visitor);
}

#if ENABLE(USER_MESSAGE_HANDLERS)
static EncodedJSValue jsDOMWindowWebKit(ExecState* exec, EncodedJSValue thisValue, PropertyName)
{
    VM& vm = exec->vm();
    JSDOMWindow* castedThis = toJSDOMWindow(vm, JSValue::decode(thisValue));
    if (!BindingSecurity::shouldAllowAccessToDOMWindow(exec, castedThis->wrapped()))
        return JSValue::encode(jsUndefined());
    return JSValue::encode(toJS(exec, castedThis->globalObject(), castedThis->wrapped().webkitNamespace()));
}
#endif

static bool jsDOMWindowGetOwnPropertySlotRestrictedAccess(JSDOMWindow* thisObject, Frame* frame, ExecState* exec, PropertyName propertyName, PropertySlot& slot, const String& errorMessage)
{
    VM& vm = exec->vm();
    auto scope = DECLARE_THROW_SCOPE(vm);

    auto& builtinNames = static_cast<JSVMClientData*>(vm.clientData)->builtinNames();

    // We don't want any properties other than "close" and "closed" on a frameless window
    // (i.e. one whose page got closed, or whose iframe got removed).
    // FIXME: This handling for frameless windows duplicates similar behaviour for cross-origin
    // access below; we should try to find a way to merge the two.
    if (!frame) {
        if (propertyName == builtinNames.closedPublicName()) {
            slot.setCustom(thisObject, JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontDelete | JSC::PropertyAttribute::DontEnum, jsDOMWindowClosed);
            return true;
        }
        if (propertyName == builtinNames.closePublicName()) {
            slot.setCustom(thisObject, JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontDelete | JSC::PropertyAttribute::DontEnum, nonCachingStaticFunctionGetter<jsDOMWindowInstanceFunctionClose, 0>);
            return true;
        }

        // FIXME: We should have a message here that explains why the property access/function call was
        // not allowed. 
        slot.setUndefined();
        return true;
    }

    // https://html.spec.whatwg.org/#crossorigingetownpropertyhelper-(-o,-p-)
    if (propertyName == vm.propertyNames->toStringTagSymbol || propertyName == vm.propertyNames->hasInstanceSymbol || propertyName == vm.propertyNames->isConcatSpreadableSymbol) {
        slot.setValue(thisObject, JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum, jsUndefined());
        return true;
    }

    // These are the functions we allow access to cross-origin (DoNotCheckSecurity in IDL).
    // Always provide the original function, on a fresh uncached function object.
    if (propertyName == builtinNames.blurPublicName()) {
        slot.setCustom(thisObject, static_cast<unsigned>(JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum), nonCachingStaticFunctionGetter<jsDOMWindowInstanceFunctionBlur, 0>);
        return true;
    }
    if (propertyName == builtinNames.closePublicName()) {
        slot.setCustom(thisObject, static_cast<unsigned>(JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum), nonCachingStaticFunctionGetter<jsDOMWindowInstanceFunctionClose, 0>);
        return true;
    }
    if (propertyName == builtinNames.focusPublicName()) {
        slot.setCustom(thisObject, static_cast<unsigned>(JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum), nonCachingStaticFunctionGetter<jsDOMWindowInstanceFunctionFocus, 0>);
        return true;
    }
    if (propertyName == builtinNames.postMessagePublicName()) {
        slot.setCustom(thisObject, static_cast<unsigned>(JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum), nonCachingStaticFunctionGetter<jsDOMWindowInstanceFunctionPostMessage, 2>);
        return true;
    }

    // When accessing cross-origin known Window properties, we always use the original property getter,
    // even if the property was removed / redefined. As of early 2016, this matches Firefox and Chrome's
    // behavior.
    if (auto* entry = JSDOMWindow::info()->staticPropHashTable->entry(propertyName)) {
        // Only allow access to these specific properties.
        if (propertyName == builtinNames.locationPublicName()
            || propertyName == builtinNames.closedPublicName()
            || propertyName == vm.propertyNames->length
            || propertyName == builtinNames.selfPublicName()
            || propertyName == builtinNames.windowPublicName()
            || propertyName == builtinNames.framesPublicName()
            || propertyName == builtinNames.openerPublicName()
            || propertyName == builtinNames.parentPublicName()
            || propertyName == builtinNames.topPublicName()) {
            bool shouldExposeSetter = propertyName == builtinNames.locationPublicName();
            CustomGetterSetter* customGetterSetter = CustomGetterSetter::create(vm, entry->propertyGetter(), shouldExposeSetter ? entry->propertyPutter() : nullptr);
            slot.setCustomGetterSetter(thisObject, static_cast<unsigned>(JSC::PropertyAttribute::CustomAccessor | JSC::PropertyAttribute::DontEnum), customGetterSetter);
            return true;
        }

        // For any other entries in the static property table, deny access. (Early return also prevents
        // named getter from returning frames with matching names - this seems a little questionable, see
        // FIXME comment on prototype search below.)
        throwSecurityError(*exec, scope, errorMessage);
        slot.setUndefined();
        return false;
    }

    // Check for child frames by name before built-in properties to match Mozilla. This does
    // not match IE, but some sites end up naming frames things that conflict with window
    // properties that are in Moz but not IE. Since we have some of these, we have to do it
    // the Moz way.
    if (auto* scopedChild = frame->tree().scopedChild(propertyNameToAtomicString(propertyName))) {
        slot.setValue(thisObject, JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontDelete | JSC::PropertyAttribute::DontEnum, toJS(exec, scopedChild->document()->domWindow()));
        return true;
    }

    throwSecurityError(*exec, scope, errorMessage);
    slot.setUndefined();
    return false;
}

// Property access sequence is:
// (1) indexed properties,
// (2) regular own properties,
// (3) named properties (in fact, these shouldn't be on the window, should be on the NPO).
bool JSDOMWindow::getOwnPropertySlot(JSObject* object, ExecState* state, PropertyName propertyName, PropertySlot& slot)
{
    // (1) First, indexed properties.
    // Hand off all indexed access to getOwnPropertySlotByIndex, which supports the indexed getter.
    if (std::optional<unsigned> index = parseIndex(propertyName))
        return getOwnPropertySlotByIndex(object, state, index.value(), slot);

    auto* thisObject = jsCast<JSDOMWindow*>(object);
    auto* frame = thisObject->wrapped().frame();

    // Hand off all cross-domain/frameless access to jsDOMWindowGetOwnPropertySlotRestrictedAccess.
    String errorMessage;
    if (!frame || !BindingSecurity::shouldAllowAccessToDOMWindow(*state, thisObject->wrapped(), errorMessage))
        return jsDOMWindowGetOwnPropertySlotRestrictedAccess(thisObject, frame, state, propertyName, slot, errorMessage);
    
    // FIXME: this need more explanation.
    // (Particularly, is it correct that this exists here but not in getOwnPropertySlotByIndex?)
    slot.setWatchpointSet(thisObject->m_windowCloseWatchpoints);

    // (2) Regular own properties.
    PropertySlot slotCopy = slot;
    if (Base::getOwnPropertySlot(thisObject, state, propertyName, slot)) {
        // Detect when we're getting the property 'showModalDialog', this is disabled, and has its original value.
        bool isShowModalDialogAndShouldHide = propertyName == static_cast<JSVMClientData*>(state->vm().clientData)->builtinNames().showModalDialogPublicName()
            && !DOMWindow::canShowModalDialog(*frame)
            && slot.isValue() && isHostFunction(slot.getValue(state, propertyName), jsDOMWindowInstanceFunctionShowModalDialog);
        // Unless we're in the showModalDialog special case, we're done.
        if (!isShowModalDialogAndShouldHide)
            return true;
        slot = slotCopy;
    }

#if ENABLE(USER_MESSAGE_HANDLERS)
    if (propertyName == static_cast<JSVMClientData*>(state->vm().clientData)->builtinNames().webkitPublicName() && thisObject->wrapped().shouldHaveWebKitNamespaceForWorld(thisObject->world())) {
        slot.setCacheableCustom(thisObject, JSC::PropertyAttribute::DontDelete | JSC::PropertyAttribute::ReadOnly, jsDOMWindowWebKit);
        return true;
    }
#endif

    return false;
}

// Property access sequence is:
// (1) indexed properties,
// (2) regular own properties,
// (3) named properties (in fact, these shouldn't be on the window, should be on the NPO).
bool JSDOMWindow::getOwnPropertySlotByIndex(JSObject* object, ExecState* state, unsigned index, PropertySlot& slot)
{
    auto* thisObject = jsCast<JSDOMWindow*>(object);
    auto* frame = thisObject->wrapped().frame();

    // Indexed getters take precendence over regular properties, so caching would be invalid.
    slot.disableCaching();

    // (1) First, indexed properties.
    // These are also allowed cross-orgin, so come before the access check.
    if (frame && index < frame->tree().scopedChildCount()) {
        slot.setValue(thisObject, static_cast<unsigned>(JSC::PropertyAttribute::ReadOnly), toJS(state, frame->tree().scopedChild(index)->document()->domWindow()));
        return true;
    }

    // Hand off all cross-domain/frameless access to jsDOMWindowGetOwnPropertySlotRestrictedAccess.
    String errorMessage;
    if (!frame || !BindingSecurity::shouldAllowAccessToDOMWindow(*state, thisObject->wrapped(), errorMessage))
        return jsDOMWindowGetOwnPropertySlotRestrictedAccess(thisObject, frame, state, Identifier::from(state, index), slot, errorMessage);

    // (2) Regular own properties.
    return Base::getOwnPropertySlotByIndex(thisObject, state, index, slot);
}

bool JSDOMWindow::put(JSCell* cell, ExecState* state, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    VM& vm = state->vm();
    auto scope = DECLARE_THROW_SCOPE(vm);

    auto* thisObject = jsCast<JSDOMWindow*>(cell);
    if (!thisObject->wrapped().frame())
        return false;

    String errorMessage;
    if (!BindingSecurity::shouldAllowAccessToDOMWindow(*state, thisObject->wrapped(), errorMessage)) {
        // We only allow setting "location" attribute cross-origin.
        if (propertyName == static_cast<JSVMClientData*>(vm.clientData)->builtinNames().locationPublicName()) {
            bool putResult = false;
            if (lookupPut(state, propertyName, thisObject, value, *s_info.staticPropHashTable, slot, putResult))
                return putResult;
            return false;
        }
        throwSecurityError(*state, scope, errorMessage);
        return false;
    }

    return Base::put(thisObject, state, propertyName, value, slot);
}

bool JSDOMWindow::putByIndex(JSCell* cell, ExecState* exec, unsigned index, JSValue value, bool shouldThrow)
{
    auto* thisObject = jsCast<JSDOMWindow*>(cell);
    if (!thisObject->wrapped().frame() || !BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->wrapped()))
        return false;
    
    return Base::putByIndex(thisObject, exec, index, value, shouldThrow);
}

bool JSDOMWindow::deleteProperty(JSCell* cell, ExecState* exec, PropertyName propertyName)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(cell);
    // Only allow deleting properties by frames in the same origin.
    if (!BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->wrapped(), ThrowSecurityError))
        return false;
    return Base::deleteProperty(thisObject, exec, propertyName);
}

bool JSDOMWindow::deletePropertyByIndex(JSCell* cell, ExecState* exec, unsigned propertyName)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(cell);
    // Only allow deleting properties by frames in the same origin.
    if (!BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->wrapped(), ThrowSecurityError))
        return false;
    return Base::deletePropertyByIndex(thisObject, exec, propertyName);
}

// https://html.spec.whatwg.org/#crossoriginproperties-(-o-)
static void addCrossOriginWindowPropertyNames(VM& vm, PropertyNameArray& propertyNames)
{
    static const Identifier* const properties[] = {
        &static_cast<JSVMClientData*>(vm.clientData)->builtinNames().blurPublicName(),
        &static_cast<JSVMClientData*>(vm.clientData)->builtinNames().closePublicName(),
        &static_cast<JSVMClientData*>(vm.clientData)->builtinNames().closedPublicName(),
        &static_cast<JSVMClientData*>(vm.clientData)->builtinNames().focusPublicName(),
        &static_cast<JSVMClientData*>(vm.clientData)->builtinNames().framesPublicName(),
        &vm.propertyNames->length,
        &static_cast<JSVMClientData*>(vm.clientData)->builtinNames().locationPublicName(),
        &static_cast<JSVMClientData*>(vm.clientData)->builtinNames().openerPublicName(),
        &static_cast<JSVMClientData*>(vm.clientData)->builtinNames().parentPublicName(),
        &static_cast<JSVMClientData*>(vm.clientData)->builtinNames().postMessagePublicName(),
        &static_cast<JSVMClientData*>(vm.clientData)->builtinNames().selfPublicName(),
        &static_cast<JSVMClientData*>(vm.clientData)->builtinNames().topPublicName(),
        &static_cast<JSVMClientData*>(vm.clientData)->builtinNames().windowPublicName()
    };
    for (auto* property : properties)
        propertyNames.add(*property);
}

static void addScopedChildrenIndexes(ExecState& state, DOMWindow& window, PropertyNameArray& propertyNames)
{
    auto* document = window.document();
    if (!document)
        return;

    auto* frame = document->frame();
    if (!frame)
        return;

    unsigned scopedChildCount = frame->tree().scopedChildCount();
    for (unsigned i = 0; i < scopedChildCount; ++i)
        propertyNames.add(Identifier::from(&state, i));
}

// https://html.spec.whatwg.org/#crossoriginownpropertykeys-(-o-)
static void addCrossOriginWindowOwnPropertyNames(ExecState& state, PropertyNameArray& propertyNames)
{
    VM& vm = state.vm();
    addCrossOriginWindowPropertyNames(vm, propertyNames);

    propertyNames.add(vm.propertyNames->toStringTagSymbol);
    propertyNames.add(vm.propertyNames->hasInstanceSymbol);
    propertyNames.add(vm.propertyNames->isConcatSpreadableSymbol);
}

// https://html.spec.whatwg.org/#windowproxy-ownpropertykeys
void JSDOMWindow::getOwnPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(object);

    addScopedChildrenIndexes(*exec, thisObject->wrapped(), propertyNames);

    if (!BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->wrapped(), DoNotReportSecurityError)) {
        if (mode.includeDontEnumProperties())
            addCrossOriginWindowOwnPropertyNames(*exec, propertyNames);
        return;
    }
    Base::getOwnPropertyNames(thisObject, exec, propertyNames, mode);
}

bool JSDOMWindow::defineOwnProperty(JSC::JSObject* object, JSC::ExecState* exec, JSC::PropertyName propertyName, const JSC::PropertyDescriptor& descriptor, bool shouldThrow)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(object);
    // Only allow defining properties in this way by frames in the same origin, as it allows setters to be introduced.
    if (!BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->wrapped(), ThrowSecurityError))
        return false;

    // Don't allow shadowing location using accessor properties.
    if (descriptor.isAccessorDescriptor() && propertyName == Identifier::fromString(exec, "location"))
        return false;

    return Base::defineOwnProperty(thisObject, exec, propertyName, descriptor, shouldThrow);
}

JSValue JSDOMWindow::getPrototype(JSObject* object, ExecState* exec)
{
    JSDOMWindow* thisObject = jsCast<JSDOMWindow*>(object);
    if (!BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->wrapped(), DoNotReportSecurityError))
        return jsNull();

    return Base::getPrototype(object, exec);
}

bool JSDOMWindow::preventExtensions(JSObject*, ExecState* exec)
{
    auto scope = DECLARE_THROW_SCOPE(exec->vm());

    throwTypeError(exec, scope, ASCIILiteral("Cannot prevent extensions on this object"));
    return false;
}

String JSDOMWindow::toStringName(const JSObject* object, ExecState* exec)
{
    auto* thisObject = jsCast<const JSDOMWindow*>(object);
    if (!BindingSecurity::shouldAllowAccessToDOMWindow(exec, thisObject->wrapped(), DoNotReportSecurityError))
        return ASCIILiteral("Object");
    return ASCIILiteral("Window");
}

// Custom Attributes

JSValue JSDOMWindow::event(ExecState& state) const
{
    Event* event = currentEvent();
    if (!event)
        return jsUndefined();
    return toJS(&state, const_cast<JSDOMWindow*>(this), event);
}

// Custom functions

class DialogHandler {
public:
    explicit DialogHandler(ExecState& exec)
        : m_exec(exec)
    {
    }

    void dialogCreated(DOMWindow&);
    JSValue returnValue() const;

private:
    ExecState& m_exec;
    RefPtr<Frame> m_frame;
};

inline void DialogHandler::dialogCreated(DOMWindow& dialog)
{
    m_frame = dialog.frame();
    
    // FIXME: This looks like a leak between the normal world and an isolated
    //        world if dialogArguments comes from an isolated world.
    JSDOMWindow* globalObject = toJSDOMWindow(m_frame.get(), normalWorld(m_exec.vm()));
    if (JSValue dialogArguments = m_exec.argument(1))
        globalObject->putDirect(m_exec.vm(), Identifier::fromString(&m_exec, "dialogArguments"), dialogArguments);
}

inline JSValue DialogHandler::returnValue() const
{
    JSDOMWindow* globalObject = toJSDOMWindow(m_frame.get(), normalWorld(m_exec.vm()));
    if (!globalObject)
        return jsUndefined();
    Identifier identifier = Identifier::fromString(&m_exec, "returnValue");
    PropertySlot slot(globalObject, PropertySlot::InternalMethodType::Get);
    if (!JSGlobalObject::getOwnPropertySlot(globalObject, &m_exec, identifier, slot))
        return jsUndefined();
    return slot.getValue(&m_exec, identifier);
}

JSValue JSDOMWindow::showModalDialog(ExecState& state)
{
    VM& vm = state.vm();
    auto scope = DECLARE_THROW_SCOPE(vm);

    if (UNLIKELY(state.argumentCount() < 1))
        return throwException(&state, scope, createNotEnoughArgumentsError(&state));

    String urlString = convert<IDLNullable<IDLDOMString>>(state, state.argument(0));
    RETURN_IF_EXCEPTION(scope, JSValue());
    String dialogFeaturesString = convert<IDLNullable<IDLDOMString>>(state, state.argument(2));
    RETURN_IF_EXCEPTION(scope, JSValue());

    DialogHandler handler(state);

    wrapped().showModalDialog(urlString, dialogFeaturesString, activeDOMWindow(state), firstDOMWindow(state), [&handler](DOMWindow& dialog) {
        handler.dialogCreated(dialog);
    });

    return handler.returnValue();
}

DOMWindow* JSDOMWindow::toWrapped(VM& vm, JSValue value)
{
    if (!value.isObject())
        return nullptr;
    JSObject* object = asObject(value);
    if (object->inherits(vm, JSDOMWindow::info()))
        return &jsCast<JSDOMWindow*>(object)->wrapped();
    if (object->inherits(vm, JSDOMWindowProxy::info()))
        return &jsCast<JSDOMWindowProxy*>(object)->wrapped();
    return nullptr;
}

} // namespace WebCore
