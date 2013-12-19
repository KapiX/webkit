/*
    Copyright (C) 2013 Haiku, Inc.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1.  Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
    2.  Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
    ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "config.h"
#include "DumpRenderTreeClient.h"

#include "WebFrame.h"
#include "WebPage.h"
#include "WebView.h"

#include <bindings/js/DOMWrapperWorld.h>
#include <Bitmap.h>
#include <DocumentLoader.h>
#include <Frame.h>
#include <FrameLoader.h>
#include <FrameView.h>
#include "NotImplemented.h"
#include <Page.h>
#include <PageGroup.h>
#include <UserContentTypes.h>

namespace WebCore {

bool DumpRenderTreeClient::s_drtRun = false;

void DumpRenderTreeClient::setDumpRenderTreeModeEnabled(bool enabled)
{
    s_drtRun = enabled;
}

bool DumpRenderTreeClient::dumpRenderTreeModeEnabled()
{
    return s_drtRun;
}

void DumpRenderTreeClient::Register(BWebPage* page)
{
    page->fDumpRenderTree = this;
}

void DumpRenderTreeClient::addUserScript(const BWebView* view,
    const String& sourceCode, bool runAtStart, bool allFrames)
{
    view->WebPage()->page()->group().addUserScriptToWorld(
        WebCore::mainThreadNormalWorld(), sourceCode, WebCore::URL(),
        Vector<String>(), Vector<String>(),
        runAtStart ? WebCore::InjectAtDocumentStart : WebCore::InjectAtDocumentEnd,
        allFrames ? WebCore::InjectInAllFrames : WebCore::InjectInTopFrameOnly);
}

void DumpRenderTreeClient::clearUserScripts(const BWebView* view)
{
    view->WebPage()->page()->group().removeUserScriptsFromWorld(
        WebCore::mainThreadNormalWorld());
}


void
DumpRenderTreeClient::setShouldTrackVisitedLinks(bool)
{
    notImplemented();
}

String DumpRenderTreeClient::responseMimeType(const BWebFrame* frame)
{
    WebCore::DocumentLoader *documentLoader = frame->Frame()->loader().documentLoader();

    if (!documentLoader)
        return String();

    return documentLoader->responseMIMEType();
}

BBitmap* DumpRenderTreeClient::getOffscreen(BWebView* view)
{
    view->OffscreenView()->LockLooper();
    view->OffscreenView()->Sync();
    view->OffscreenView()->UnlockLooper();
    return view->OffscreenBitmap();
}

BSize DumpRenderTreeClient::getOffscreenSize(BWebView* view)
{
    view->OffscreenView()->LockLooper();
    BRect bounds = view->OffscreenView()->Bounds();
    view->OffscreenView()->UnlockLooper();
    return BSize(bounds.Width(), bounds.Height());
}

void
DumpRenderTreeClient::setValueForUser(OpaqueJSContext const*, OpaqueJSValue const*, WTF::String const&)
{
    notImplemented();
}

void
DumpRenderTreeClient::setDomainRelaxationForbiddenForURLScheme(bool, WTF::String const&)
{
    notImplemented();
}

void
DumpRenderTreeClient::setSerializeHTTPLoads(bool)
{
    notImplemented();
}

}
