/*
    Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
    Copyright (C) 2008 Trolltech ASA
    Copyright (C) 2008 Collabora Ltd. All rights reserved.
    Copyright (C) 2008 INdT - Instituto Nokia de Tecnologia
    Copyright (C) 2009-2010 ProFUSION embedded systems
    Copyright (C) 2009-2011 Samsung Electronics
    Copyright (C) 2012 Intel Corporation

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
#include "PlatformStrategiesHaiku.h"

#include "BlobRegistryImpl.h"
#include "NetworkStorageSession.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PageGroup.h"
#include "PlatformCookieJar.h"
#include "WebResourceLoadScheduler.h"


using namespace WebCore;


void PlatformStrategiesHaiku::initialize()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(PlatformStrategiesHaiku, platformStrategies, ());
    setPlatformStrategies(&platformStrategies);
}

PlatformStrategiesHaiku::PlatformStrategiesHaiku()
{
}

CookiesStrategy* PlatformStrategiesHaiku::createCookiesStrategy()
{
    return this;
}

LoaderStrategy* PlatformStrategiesHaiku::createLoaderStrategy()
{
    return new WebResourceLoadScheduler();
}

PasteboardStrategy* PlatformStrategiesHaiku::createPasteboardStrategy()
{
    notImplemented();
    return 0;
}

WebCore::BlobRegistry* PlatformStrategiesHaiku::createBlobRegistry()
{
    return new BlobRegistryImpl();
}

// CookiesStrategy
std::pair<WTF::String, bool> PlatformStrategiesHaiku::cookiesForDOM(const NetworkStorageSession& session, const URL& firstParty, const URL& url, IncludeSecureCookies includeSecure)
{
    return WebCore::cookiesForDOM(session, firstParty, url, includeSecure);
}

void PlatformStrategiesHaiku::setCookiesFromDOM(const NetworkStorageSession& session, const URL& firstParty, const URL& url, const String& cookieString)
{
    WebCore::setCookiesFromDOM(session, firstParty, url, cookieString);
}

bool PlatformStrategiesHaiku::cookiesEnabled(const NetworkStorageSession& session)
{
    return WebCore::cookiesEnabled(session);
}

std::pair<WTF::String, bool> PlatformStrategiesHaiku::cookieRequestHeaderFieldValue(const NetworkStorageSession& session, const URL& firstParty, const URL& url, IncludeSecureCookies includeSecure)
{
    return WebCore::cookieRequestHeaderFieldValue(session, firstParty, url, includeSecure);
}

std::pair<WTF::String, bool> PlatformStrategiesHaiku::cookieRequestHeaderFieldValue(PAL::SessionID sessionID, const URL& firstParty, const URL& url, IncludeSecureCookies includeSecure)
{
    return WebCore::cookieRequestHeaderFieldValue(NetworkStorageSession::defaultStorageSession(), firstParty, url, includeSecure);
}

bool PlatformStrategiesHaiku::getRawCookies(const NetworkStorageSession& session, const URL& firstParty, const URL& url, Vector<Cookie>& rawCookies)
{
    return WebCore::getRawCookies(session, firstParty, url, rawCookies);
}

void PlatformStrategiesHaiku::deleteCookie(const NetworkStorageSession& session, const URL& url, const String& cookieName)
{
    WebCore::deleteCookie(session, url, cookieName);
}
