/*
 * Copyright (C) 2011, 2017 Igalia S.L.
 * Portions Copyright (c) 2011 Motorola Mobility, Inc.  All rights reserved.
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
#include "WebViewTest.h"

#include <JavaScriptCore/JSRetainPtr.h>

bool WebViewTest::shouldInitializeWebViewInConstructor = true;

WebViewTest::WebViewTest()
    : m_userContentManager(adoptGRef(webkit_user_content_manager_new()))
    , m_mainLoop(g_main_loop_new(nullptr, TRUE))
{
    if (shouldInitializeWebViewInConstructor)
        initializeWebView();
    assertObjectIsDeletedWhenTestFinishes(G_OBJECT(m_userContentManager.get()));
}

WebViewTest::~WebViewTest()
{
    platformDestroy();
    if (m_javascriptResult)
        webkit_javascript_result_unref(m_javascriptResult);
    if (m_surface)
        cairo_surface_destroy(m_surface);
    g_object_unref(m_webView);
    g_main_loop_unref(m_mainLoop);
}

void WebViewTest::initializeWebView()
{
    g_assert(!m_webView);
    m_webView = WEBKIT_WEB_VIEW(g_object_new(WEBKIT_TYPE_WEB_VIEW,
#if PLATFORM(WPE)
        "backend", Test::createWebViewBackend(),
#endif
        "web-context", m_webContext.get(),
        "user-content-manager", m_userContentManager.get(),
        nullptr));
    platformInitializeWebView();
    assertObjectIsDeletedWhenTestFinishes(G_OBJECT(m_webView));

    g_signal_connect(m_webView, "web-process-crashed", G_CALLBACK(WebViewTest::webProcessCrashed), this);
}

gboolean WebViewTest::webProcessCrashed(WebKitWebView*, WebViewTest* test)
{
    if (test->m_expectedWebProcessCrash) {
        test->m_expectedWebProcessCrash = false;
        return FALSE;
    }
    g_assert_not_reached();
    return TRUE;
}

void WebViewTest::loadURI(const char* uri)
{
    m_activeURI = uri;
    webkit_web_view_load_uri(m_webView, uri);
    g_assert(webkit_web_view_is_loading(m_webView));
    g_assert_cmpstr(webkit_web_view_get_uri(m_webView), ==, m_activeURI.data());
}

void WebViewTest::loadHtml(const char* html, const char* baseURI)
{
    if (!baseURI)
        m_activeURI = "about:blank";
    else
        m_activeURI = baseURI;
    webkit_web_view_load_html(m_webView, html, baseURI);
    g_assert(webkit_web_view_is_loading(m_webView));
    g_assert_cmpstr(webkit_web_view_get_uri(m_webView), ==, m_activeURI.data());
}

void WebViewTest::loadPlainText(const char* plainText)
{
    m_activeURI = "about:blank";
    webkit_web_view_load_plain_text(m_webView, plainText);
    g_assert(webkit_web_view_is_loading(m_webView));
    g_assert_cmpstr(webkit_web_view_get_uri(m_webView), ==, m_activeURI.data());
}

void WebViewTest::loadBytes(GBytes* bytes, const char* mimeType, const char* encoding, const char* baseURI)
{
    if (!baseURI)
        m_activeURI = "about:blank";
    else
        m_activeURI = baseURI;
    webkit_web_view_load_bytes(m_webView, bytes, mimeType, encoding, baseURI);
    g_assert(webkit_web_view_is_loading(m_webView));
    g_assert_cmpstr(webkit_web_view_get_uri(m_webView), ==, m_activeURI.data());
}

void WebViewTest::loadRequest(WebKitURIRequest* request)
{
    m_activeURI = webkit_uri_request_get_uri(request);
    webkit_web_view_load_request(m_webView, request);
    g_assert(webkit_web_view_is_loading(m_webView));
    g_assert_cmpstr(webkit_web_view_get_uri(m_webView), ==, m_activeURI.data());
}

void WebViewTest::loadAlternateHTML(const char* html, const char* contentURI, const char* baseURI)
{
    m_activeURI = contentURI;
    webkit_web_view_load_alternate_html(m_webView, html, contentURI, baseURI);
    g_assert(webkit_web_view_is_loading(m_webView));
    g_assert_cmpstr(webkit_web_view_get_uri(m_webView), ==, m_activeURI.data());
}

void WebViewTest::goBack()
{
    bool canGoBack = webkit_web_view_can_go_back(m_webView);
    if (canGoBack) {
        WebKitBackForwardList* list = webkit_web_view_get_back_forward_list(m_webView);
        WebKitBackForwardListItem* item = webkit_back_forward_list_get_nth_item(list, -1);
        m_activeURI = webkit_back_forward_list_item_get_original_uri(item);
    }

    // Call go_back even when can_go_back returns FALSE to check nothing happens.
    webkit_web_view_go_back(m_webView);
    if (canGoBack) {
        g_assert(webkit_web_view_is_loading(m_webView));
        g_assert_cmpstr(webkit_web_view_get_uri(m_webView), ==, m_activeURI.data());
    }
}

void WebViewTest::goForward()
{
    bool canGoForward = webkit_web_view_can_go_forward(m_webView);
    if (canGoForward) {
        WebKitBackForwardList* list = webkit_web_view_get_back_forward_list(m_webView);
        WebKitBackForwardListItem* item = webkit_back_forward_list_get_nth_item(list, 1);
        m_activeURI = webkit_back_forward_list_item_get_original_uri(item);
    }

    // Call go_forward even when can_go_forward returns FALSE to check nothing happens.
    webkit_web_view_go_forward(m_webView);
    if (canGoForward) {
        g_assert(webkit_web_view_is_loading(m_webView));
        g_assert_cmpstr(webkit_web_view_get_uri(m_webView), ==, m_activeURI.data());
    }
}

void WebViewTest::goToBackForwardListItem(WebKitBackForwardListItem* item)
{
    m_activeURI = webkit_back_forward_list_item_get_original_uri(item);
    webkit_web_view_go_to_back_forward_list_item(m_webView, item);
    g_assert(webkit_web_view_is_loading(m_webView));
    g_assert_cmpstr(webkit_web_view_get_uri(m_webView), ==, m_activeURI.data());
}

void WebViewTest::quitMainLoop()
{
    g_main_loop_quit(m_mainLoop);
}

void WebViewTest::wait(double seconds)
{
    g_timeout_add(seconds * 1000, [](gpointer userData) -> gboolean {
        static_cast<WebViewTest*>(userData)->quitMainLoop();
        return G_SOURCE_REMOVE;
    }, this);
    g_main_loop_run(m_mainLoop);
}

static void loadChanged(WebKitWebView* webView, WebKitLoadEvent loadEvent, WebViewTest* test)
{
    if (loadEvent != WEBKIT_LOAD_FINISHED)
        return;
    g_signal_handlers_disconnect_by_func(webView, reinterpret_cast<void*>(loadChanged), test);
    g_main_loop_quit(test->m_mainLoop);
}

void WebViewTest::waitUntilLoadFinished()
{
    g_signal_connect(m_webView, "load-changed", G_CALLBACK(loadChanged), this);
    g_main_loop_run(m_mainLoop);
}

static void titleChanged(WebKitWebView* webView, GParamSpec*, WebViewTest* test)
{
    if (!test->m_expectedTitle.isNull() && test->m_expectedTitle != webkit_web_view_get_title(webView))
        return;

    g_signal_handlers_disconnect_by_func(webView, reinterpret_cast<void*>(titleChanged), test);
    g_main_loop_quit(test->m_mainLoop);
}

void WebViewTest::waitUntilTitleChangedTo(const char* expectedTitle)
{
    m_expectedTitle = expectedTitle;
    g_signal_connect(m_webView, "notify::title", G_CALLBACK(titleChanged), this);
    g_main_loop_run(m_mainLoop);
    m_expectedTitle = CString();
}

void WebViewTest::waitUntilTitleChanged()
{
    waitUntilTitleChangedTo(0);
}

static void directoryChangedCallback(GFileMonitor*, GFile* file, GFile*, GFileMonitorEvent event, WebViewTest* test)
{
    if (event == G_FILE_MONITOR_EVENT_CREATED && g_file_equal(file, test->m_monitoredFile.get()))
        test->quitMainLoop();
}

void WebViewTest::waitUntilFileExists(const char* filename)
{
    m_monitoredFile = adoptGRef(g_file_new_for_path(filename));
    GRefPtr<GFile> directory = adoptGRef(g_file_get_parent(m_monitoredFile.get()));
    GRefPtr<GFileMonitor> monitor = adoptGRef(g_file_monitor_directory(directory.get(), G_FILE_MONITOR_NONE, nullptr, nullptr));
    g_assert(monitor.get());
    g_signal_connect(monitor.get(), "changed", G_CALLBACK(directoryChangedCallback), this);
    if (!g_file_query_exists(m_monitoredFile.get(), nullptr))
        g_main_loop_run(m_mainLoop);
}

void WebViewTest::selectAll()
{
    webkit_web_view_execute_editing_command(m_webView, "SelectAll");
}

bool WebViewTest::isEditable()
{
    return webkit_web_view_is_editable(m_webView);
}

void WebViewTest::setEditable(bool editable)
{
    webkit_web_view_set_editable(m_webView, editable);
}

static void resourceGetDataCallback(GObject* object, GAsyncResult* result, gpointer userData)
{
    size_t dataSize;
    GUniqueOutPtr<GError> error;
    unsigned char* data = webkit_web_resource_get_data_finish(WEBKIT_WEB_RESOURCE(object), result, &dataSize, &error.outPtr());
    g_assert(data);

    WebViewTest* test = static_cast<WebViewTest*>(userData);
    test->m_resourceData.reset(reinterpret_cast<char*>(data));
    test->m_resourceDataSize = dataSize;
    g_main_loop_quit(test->m_mainLoop);
}

const char* WebViewTest::mainResourceData(size_t& mainResourceDataSize)
{
    m_resourceDataSize = 0;
    m_resourceData.reset();
    WebKitWebResource* resource = webkit_web_view_get_main_resource(m_webView);
    g_assert(resource);

    webkit_web_resource_get_data(resource, 0, resourceGetDataCallback, this);
    g_main_loop_run(m_mainLoop);

    mainResourceDataSize = m_resourceDataSize;
    return m_resourceData.get();
}

static void runJavaScriptReadyCallback(GObject*, GAsyncResult* result, WebViewTest* test)
{
    test->m_javascriptResult = webkit_web_view_run_javascript_finish(test->m_webView, result, test->m_javascriptError);
    g_main_loop_quit(test->m_mainLoop);
}

static void runJavaScriptFromGResourceReadyCallback(GObject*, GAsyncResult* result, WebViewTest* test)
{
    test->m_javascriptResult = webkit_web_view_run_javascript_from_gresource_finish(test->m_webView, result, test->m_javascriptError);
    g_main_loop_quit(test->m_mainLoop);
}

WebKitJavascriptResult* WebViewTest::runJavaScriptAndWaitUntilFinished(const char* javascript, GError** error)
{
    if (m_javascriptResult)
        webkit_javascript_result_unref(m_javascriptResult);
    m_javascriptResult = 0;
    m_javascriptError = error;
    webkit_web_view_run_javascript(m_webView, javascript, 0, reinterpret_cast<GAsyncReadyCallback>(runJavaScriptReadyCallback), this);
    g_main_loop_run(m_mainLoop);

    return m_javascriptResult;
}

WebKitJavascriptResult* WebViewTest::runJavaScriptFromGResourceAndWaitUntilFinished(const char* resource, GError** error)
{
    if (m_javascriptResult)
        webkit_javascript_result_unref(m_javascriptResult);
    m_javascriptResult = 0;
    m_javascriptError = error;
    webkit_web_view_run_javascript_from_gresource(m_webView, resource, 0, reinterpret_cast<GAsyncReadyCallback>(runJavaScriptFromGResourceReadyCallback), this);
    g_main_loop_run(m_mainLoop);

    return m_javascriptResult;
}

static char* jsValueToCString(JSGlobalContextRef context, JSValueRef value)
{
    g_assert(value);
    g_assert(JSValueIsString(context, value));

    JSRetainPtr<JSStringRef> stringValue(Adopt, JSValueToStringCopy(context, value, 0));
    g_assert(stringValue);

    size_t cStringLength = JSStringGetMaximumUTF8CStringSize(stringValue.get());
    char* cString = static_cast<char*>(g_malloc(cStringLength));
    JSStringGetUTF8CString(stringValue.get(), cString, cStringLength);
    return cString;
}

char* WebViewTest::javascriptResultToCString(WebKitJavascriptResult* javascriptResult)
{
    JSGlobalContextRef context = webkit_javascript_result_get_global_context(javascriptResult);
    g_assert(context);
    return jsValueToCString(context, webkit_javascript_result_get_value(javascriptResult));
}

double WebViewTest::javascriptResultToNumber(WebKitJavascriptResult* javascriptResult)
{
    JSGlobalContextRef context = webkit_javascript_result_get_global_context(javascriptResult);
    g_assert(context);
    JSValueRef value = webkit_javascript_result_get_value(javascriptResult);
    g_assert(value);
    g_assert(JSValueIsNumber(context, value));

    return JSValueToNumber(context, value, 0);
}

bool WebViewTest::javascriptResultToBoolean(WebKitJavascriptResult* javascriptResult)
{
    JSGlobalContextRef context = webkit_javascript_result_get_global_context(javascriptResult);
    g_assert(context);
    JSValueRef value = webkit_javascript_result_get_value(javascriptResult);
    g_assert(value);
    g_assert(JSValueIsBoolean(context, value));

    return JSValueToBoolean(context, value);
}

bool WebViewTest::javascriptResultIsNull(WebKitJavascriptResult* javascriptResult)
{
    JSGlobalContextRef context = webkit_javascript_result_get_global_context(javascriptResult);
    g_assert(context);
    JSValueRef value = webkit_javascript_result_get_value(javascriptResult);
    g_assert(value);

    return JSValueIsNull(context, value);
}

bool WebViewTest::javascriptResultIsUndefined(WebKitJavascriptResult* javascriptResult)
{
    JSGlobalContextRef context = webkit_javascript_result_get_global_context(javascriptResult);
    g_assert(context);
    JSValueRef value = webkit_javascript_result_get_value(javascriptResult);
    g_assert(value);

    return JSValueIsUndefined(context, value);
}

#if PLATFORM(GTK)
static void onSnapshotReady(WebKitWebView* web_view, GAsyncResult* res, WebViewTest* test)
{
    GUniqueOutPtr<GError> error;
    test->m_surface = webkit_web_view_get_snapshot_finish(web_view, res, &error.outPtr());
    g_assert(!test->m_surface || !error.get());
    if (error)
        g_assert_error(error.get(), WEBKIT_SNAPSHOT_ERROR, WEBKIT_SNAPSHOT_ERROR_FAILED_TO_CREATE);
    test->quitMainLoop();
}

cairo_surface_t* WebViewTest::getSnapshotAndWaitUntilReady(WebKitSnapshotRegion region, WebKitSnapshotOptions options)
{
    if (m_surface)
        cairo_surface_destroy(m_surface);
    m_surface = 0;
    webkit_web_view_get_snapshot(m_webView, region, options, 0, reinterpret_cast<GAsyncReadyCallback>(onSnapshotReady), this);
    g_main_loop_run(m_mainLoop);
    return m_surface;
}
#endif

bool WebViewTest::runWebProcessTest(const char* suiteName, const char* testName)
{
    GUniquePtr<char> script(g_strdup_printf("WebProcessTestRunner.runTest('%s/%s');", suiteName, testName));
    GUniqueOutPtr<GError> error;
    WebKitJavascriptResult* javascriptResult = runJavaScriptAndWaitUntilFinished(script.get(), &error.outPtr());
    g_assert(!error);
    return javascriptResultToBoolean(javascriptResult);
}
