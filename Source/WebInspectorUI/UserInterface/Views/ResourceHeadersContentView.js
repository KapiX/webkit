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

WI.ResourceHeadersContentView = class ResourceHeadersContentView extends WI.ContentView
{
    constructor(resource, delegate)
    {
        super(null);

        console.assert(resource instanceof WI.Resource);
        console.assert(delegate);

        this._resource = resource;
        this._resource.addEventListener(WI.Resource.Event.MetricsDidChange, this._resourceMetricsDidChange, this);
        this._resource.addEventListener(WI.Resource.Event.RequestHeadersDidChange, this._resourceRequestHeadersDidChange, this);
        this._resource.addEventListener(WI.Resource.Event.ResponseReceived, this._resourceResponseReceived, this);

        this._delegate = delegate;

        this._searchQuery = null;
        this._searchResults = null;
        this._searchDOMChanges = [];
        this._searchIndex = -1;
        this._automaticallyRevealFirstSearchResult = false;
        this._bouncyHighlightElement = null;

        this.element.classList.add("resource-details", "resource-headers");
        this.element.tabIndex = 0;

        this._needsSummaryRefresh = false;
        this._needsRequestHeadersRefresh = false;
        this._needsResponseHeadersRefresh = false;
    }

    // Protected

    initialLayout()
    {
        super.initialLayout();

        this._summarySection = new WI.ResourceDetailsSection(WI.UIString("Summary"), "summary");
        this.element.appendChild(this._summarySection.element);
        this._refreshSummarySection();

        this._requestHeadersSection = new WI.ResourceDetailsSection(WI.UIString("Request"), "headers");
        this.element.appendChild(this._requestHeadersSection.element);
        this._refreshRequestHeadersSection();

        // FIXME: <https://webkit.org/b/150005> Web Inspector: Redirect requests are not shown in either Network or Timeline tabs

        this._responseHeadersSection = new WI.ResourceDetailsSection(WI.UIString("Response"), "headers");
        this.element.appendChild(this._responseHeadersSection.element);
        this._refreshResponseHeadersSection();

        if (this._resource.urlComponents.queryString) {
            this._queryStringSection = new WI.ResourceDetailsSection(WI.UIString("Query String"));
            this.element.appendChild(this._queryStringSection.element);
            this._refreshQueryStringSection();
        }

        if (this._resource.requestData) {
            this._requestDataSection = new WI.ResourceDetailsSection(WI.UIString("Request Data"));
            this.element.appendChild(this._requestDataSection.element);
            this._refreshRequestDataSection();
        }

        this._needsSummaryRefresh = false;
        this._needsRequestHeadersRefresh = false;
        this._needsResponseHeadersRefresh = false;
    }

    layout()
    {
        super.layout();

        if (this._needsSummaryRefresh) {
            this._refreshSummarySection();
            this._needsSummaryRefresh = false;
        }

        if (this._needsRequestHeadersRefresh) {
            this._refreshRequestHeadersSection();
            this._needsRequestHeadersRefresh = false;
        }

        if (this._needsResponseHeadersRefresh) {
            this._refreshResponseHeadersSection();
            this._needsResponseHeadersRefresh = false;
        }
    }

    closed()
    {
        this._resource.removeEventListener(null, null, this);

        super.closed();
    }

    get supportsSearch()
    {
        return true;
    }

    get numberOfSearchResults()
    {
        return this._searchResults ? this._searchResults.length : null;
    }

    get hasPerformedSearch()
    {
        return this._searchResults !== null;
    }

    set automaticallyRevealFirstSearchResult(reveal)
    {
        this._automaticallyRevealFirstSearchResult = reveal;

        // If we haven't shown a search result yet, reveal one now.
        if (this._automaticallyRevealFirstSearchResult && this.numberOfSearchResults > 0) {
            if (this._searchIndex === -1)
                this.revealNextSearchResult();
        }
    }

    performSearch(query)
    {
        if (query === this._searchQuery)
            return;

        WI.revertDOMChanges(this._searchDOMChanges);

        this._searchQuery = query;
        this._searchResults = [];
        this._searchDOMChanges = [];
        this._searchIndex = -1;

        this._perfomSearchOnKeyValuePairs();

        this.dispatchEventToListeners(WI.ContentView.Event.NumberOfSearchResultsDidChange);

        if (this._automaticallyRevealFirstSearchResult && this._searchResults.length > 0)
            this.revealNextSearchResult();
    }

    searchCleared()
    {
        WI.revertDOMChanges(this._searchDOMChanges);

        this._searchQuery = null;
        this._searchResults = null;
        this._searchDOMChanges = [];
        this._searchIndex = -1;
    }

    revealPreviousSearchResult(changeFocus)
    {
        if (!this.numberOfSearchResults)
            return;

        if (this._searchIndex > 0)
            --this._searchIndex;
        else
            this._searchIndex = this._searchResults.length - 1;

        this._revealSearchResult(this._searchIndex, changeFocus);
    }

    revealNextSearchResult(changeFocus)
    {
        if (!this.numberOfSearchResults)
            return;

        if (this._searchIndex + 1 < this._searchResults.length)
            ++this._searchIndex;
        else
            this._searchIndex = 0;

        this._revealSearchResult(this._searchIndex, changeFocus);
    }

    // Private

    _markIncompleteSectionWithMessage(section, message)
    {
        section.toggleIncomplete(true);

        let p = section.detailsElement.appendChild(document.createElement("p"));
        p.textContent = message;
    }

    _markIncompleteSectionWithLoadingIndicator(section)
    {
        section.toggleIncomplete(true);

        let p = section.detailsElement.appendChild(document.createElement("p"));
        let spinner = new WI.IndeterminateProgressSpinner;
        p.appendChild(spinner.element);
    }

    _appendKeyValuePair(parentElement, key, value, className)
    {
        let p = parentElement.appendChild(document.createElement("p"));
        p.className = "pair";
        if (className)
            p.classList.add(className);

        // Don't include a colon if no value.
        console.assert(typeof key === "string");
        let displayKey = key + (value ? ": " : "");

        let keyElement = p.appendChild(document.createElement("span"));
        keyElement.className = "key";
        keyElement.textContent = displayKey;

        let valueElement = p.appendChild(document.createElement("span"));
        valueElement.className = "value";
        if (value instanceof Node)
            valueElement.appendChild(value);
        else
            valueElement.textContent = value;
    }

    _responseSourceDisplayString(responseSource)
    {
        switch (responseSource) {
        case WI.Resource.ResponseSource.Network:
            return WI.UIString("Network");
        case WI.Resource.ResponseSource.MemoryCache:
            return WI.UIString("Memory Cache");
        case WI.Resource.ResponseSource.DiskCache:
            return WI.UIString("Disk Cache");
        case WI.Resource.ResponseSource.ServiceWorker:
            return WI.UIString("Service Worker");
        case WI.Resource.ResponseSource.Unknown:
        default:
            return null;
        }
    }

    _refreshSummarySection()
    {
        let detailsElement = this._summarySection.detailsElement;
        detailsElement.removeChildren();

        this._summarySection.toggleError(this._resource.hadLoadingError());

        this._appendKeyValuePair(detailsElement, WI.UIString("URL"), this._resource.url.insertWordBreakCharacters());

        let status = emDash;
        if (!isNaN(this._resource.statusCode))
            status = this._resource.statusCode + (this._resource.statusText ? " " + this._resource.statusText : "");
        this._appendKeyValuePair(detailsElement, WI.UIString("Status"), status);

        // FIXME: <https://webkit.org/b/178827> Web Inspector: Should be able to link directly to the ServiceWorker that handled a particular load

        let source = this._responseSourceDisplayString(this._resource.responseSource) || emDash;
        this._appendKeyValuePair(detailsElement, WI.UIString("Source"), source);

        if (this._resource.remoteAddress)
            this._appendKeyValuePair(detailsElement, WI.UIString("Address"), this._resource.remoteAddress);
    }

    _refreshRequestHeadersSection()
    {
        let detailsElement = this._requestHeadersSection.detailsElement;
        detailsElement.removeChildren();

        // A revalidation request still sends a request even though we served from cache, so show the request.
        if (this._resource.statusCode !== 304) {
            if (this._resource.responseSource === WI.Resource.ResponseSource.MemoryCache) {
                this._markIncompleteSectionWithMessage(this._requestHeadersSection, WI.UIString("No request, served from the memory cache."));
                return;
            }
            if (this._resource.responseSource === WI.Resource.ResponseSource.DiskCache) {
                this._markIncompleteSectionWithMessage(this._requestHeadersSection, WI.UIString("No request, served from the disk cache."));
                return;
            }
        }

        let protocol = this._resource.protocol || "";
        let urlComponents = this._resource.urlComponents;
        if (protocol.startsWith("http/1")) {
            // HTTP/1.1 request line:
            // https://www.w3.org/Protocols/rfc2616/rfc2616-sec5.html#sec5.1
            let requestLine = `${this._resource.requestMethod} ${urlComponents.path} ${protocol.toUpperCase()}`;
            this._appendKeyValuePair(detailsElement, requestLine, null, "h1-status");
        } else if (protocol === "h2") {
            // HTTP/2 Request pseudo headers:
            // https://tools.ietf.org/html/rfc7540#section-8.1.2.3
            this._appendKeyValuePair(detailsElement, ":method", this._resource.requestMethod, "h2-pseudo-header");
            this._appendKeyValuePair(detailsElement, ":scheme", urlComponents.scheme, "h2-pseudo-header");
            this._appendKeyValuePair(detailsElement, ":authority", WI.h2Authority(urlComponents), "h2-pseudo-header");
            this._appendKeyValuePair(detailsElement, ":path", WI.h2Path(urlComponents), "h2-pseudo-header");
        }

        let requestHeaders = this._resource.requestHeaders;
        for (let key in requestHeaders)
            this._appendKeyValuePair(detailsElement, key, requestHeaders[key], "header");

        if (!detailsElement.firstChild)
            this._markIncompleteSectionWithMessage(this._requestHeadersSection, WI.UIString("No request headers"));
    }

    _refreshResponseHeadersSection()
    {
        let detailsElement = this._responseHeadersSection.detailsElement;
        detailsElement.removeChildren();

        if (!this._resource.hasResponse()) {
            this._markIncompleteSectionWithLoadingIndicator(this._responseHeadersSection);
            return;
        }

        this._responseHeadersSection.toggleIncomplete(false);

        let protocol = this._resource.protocol || "";
        if (protocol.startsWith("http/1")) {
            // HTTP/1.1 response status line:
            // https://www.w3.org/Protocols/rfc2616/rfc2616-sec6.html#sec6.1
            let responseLine = `${protocol.toUpperCase()} ${this._resource.statusCode} ${this._resource.statusText}`;
            this._appendKeyValuePair(detailsElement, responseLine, null, "h1-status");
        } else if (protocol === "h2") {
            // HTTP/2 Response pseudo headers:
            // https://tools.ietf.org/html/rfc7540#section-8.1.2.4
            this._appendKeyValuePair(detailsElement, ":status", this._resource.statusCode, "h2-pseudo-header");
        }

        let responseHeaders = this._resource.responseHeaders;
        for (let key in responseHeaders) {
            // Split multiple Set-Cookie response headers out into their multiple headers instead of as a combined value.
            if (key.toLowerCase() === "set-cookie") {
                let responseCookies = this._resource.responseCookies;
                console.assert(responseCookies.length > 0);
                for (let cookie of responseCookies)
                    this._appendKeyValuePair(detailsElement, key, cookie.rawHeader, "header");
                continue;
            }

            this._appendKeyValuePair(detailsElement, key, responseHeaders[key], "header");
        }

        if (!detailsElement.firstChild)
            this._markIncompleteSectionWithMessage(this._responseHeadersSection, WI.UIString("No response headers"));
    }

    _refreshQueryStringSection()
    {
        if (!this._queryStringSection)
            return;

        let detailsElement = this._queryStringSection.detailsElement;
        detailsElement.removeChildren();

        let queryString = this._resource.urlComponents.queryString;
        let queryStringPairs = parseQueryString(queryString, true);
        for (let {name, value} of queryStringPairs)
            this._appendKeyValuePair(detailsElement, name, value);
    }

    _refreshRequestDataSection()
    {
        if (!this._requestDataSection)
            return;

        let detailsElement = this._requestDataSection.detailsElement;
        detailsElement.removeChildren();

        let requestData = this._resource.requestData;
        let requestDataContentType = this._resource.requestDataContentType || "";

        if (requestDataContentType && requestDataContentType.match(/^application\/x-www-form-urlencoded\s*(;.*)?$/i)) {
            // Simple form data that should be parsable like a query string.
            this._appendKeyValuePair(detailsElement, WI.UIString("MIME Type"), requestDataContentType);
            let queryStringPairs = parseQueryString(requestData, true);
            for (let {name, value} of queryStringPairs)
                this._appendKeyValuePair(detailsElement, name, value);
            return;
        }

        let mimeTypeComponents = parseMIMEType(requestDataContentType);
        let mimeType = mimeTypeComponents.type;
        let boundary = mimeTypeComponents.boundary;
        let encoding = mimeTypeComponents.encoding;

        this._appendKeyValuePair(detailsElement, WI.UIString("MIME Type"), mimeType);
        if (boundary)
            this._appendKeyValuePair(detailsElement, WI.UIString("Boundary"), boundary);
        if (encoding)
            this._appendKeyValuePair(detailsElement, WI.UIString("Encoding"), encoding);

        let goToButton = detailsElement.appendChild(WI.createGoToArrowButton());
        goToButton.addEventListener("click", () => { this._delegate.headersContentViewGoToRequestData(this); });
        this._appendKeyValuePair(detailsElement, WI.UIString("Request Data"), goToButton);
    }

    _perfomSearchOnKeyValuePairs()
    {
        let searchRegex = new RegExp(this._searchQuery.escapeForRegExp(), "gi");

        let elements = this.element.querySelectorAll(".key, .value");
        for (let element of elements) {
            let matchRanges = [];
            let text = element.textContent;
            let match;
            while (match = searchRegex.exec(text))
                matchRanges.push({offset: match.index, length: match[0].length});

            if (matchRanges.length) {
                let highlightedNodes = WI.highlightRangesWithStyleClass(element, matchRanges, "search-highlight", this._searchDOMChanges);
                this._searchResults = this._searchResults.concat(highlightedNodes);
            }
        }
    }

    _revealSearchResult(index, changeFocus)
    {
        let highlightElement = this._searchResults[index];
        if (!highlightElement)
            return;

        highlightElement.scrollIntoViewIfNeeded();

        if (!this._bouncyHighlightElement) {
            this._bouncyHighlightElement = document.createElement("div");
            this._bouncyHighlightElement.className = "bouncy-highlight";
            this._bouncyHighlightElement.addEventListener("animationend", (event) => {
                this._bouncyHighlightElement.remove();
            });
        }

        this._bouncyHighlightElement.remove();

        let computedStyles = window.getComputedStyle(highlightElement);
        let highlightElementRect = highlightElement.getBoundingClientRect();
        let contentViewRect = this.element.getBoundingClientRect();
        let contentViewScrollTop = this.element.scrollTop;
        let contentViewScrollLeft = this.element.scrollLeft;

        this._bouncyHighlightElement.textContent = highlightElement.textContent;
        this._bouncyHighlightElement.style.top = (highlightElementRect.top - contentViewRect.top + contentViewScrollTop) + "px";
        this._bouncyHighlightElement.style.left = (highlightElementRect.left - contentViewRect.left + contentViewScrollLeft) + "px";
        this._bouncyHighlightElement.style.fontWeight = computedStyles.fontWeight;

        this.element.appendChild(this._bouncyHighlightElement);
    }

    _resourceMetricsDidChange(event)
    {
        this._needsSummaryRefresh = true;
        this._needsRequestHeadersRefresh = true;
        this._needsResponseHeadersRefresh = true;
        this.needsLayout();
    }

    _resourceRequestHeadersDidChange(event)
    {
        this._needsRequestHeadersRefresh = true;
        this.needsLayout();
    }

    _resourceResponseReceived(event)
    {
        this._needsSummaryRefresh = true;
        this._needsResponseHeadersRefresh = true;
        this.needsLayout();
    }
};
