/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include "IPlugWebView.h"
#include "IPlugPaths.h"
#include <gtk/gtkx.h>
#include <gdk/gdkx.h>
#include <webkit2/webkit2.h>
#include <dlfcn.h>
#include <cmath>
#include <filesystem>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#include <cstring>

// GTK is process-global. Host callbacks must run on the GUI/main thread.
extern "C" void IPlugLinuxWebViewPump()
{
  for (int i = 0; i < 32 && g_main_context_pending(nullptr); ++i)
    g_main_context_iteration(nullptr, FALSE);
}

static void IPlugLinuxWebViewModuleAnchor() {}

BEGIN_IPLUG_NAMESPACE

class IWebViewImpl
{
public:
  explicit IWebViewImpl(IWebView* owner) : mOwner(owner) {}
  ~IWebViewImpl() { CloseWebView(); }

  void* OpenWebView(void* parent, float x, float y, float w, float h, float scale)
  {
    CloseWebView();
    if (!parent || !gtk_init_check(nullptr, nullptr)) return nullptr;
    // Reject stale/invalid XIDs before gtk_plug_new issues X requests. GDK's
    // own connection may otherwise turn a BadWindow into a process abort.
    GdkDisplay* display = gdk_display_get_default();
    if (!GDK_IS_X11_DISPLAY(display)) return nullptr;
    GdkWindow* hostWindow = gdk_x11_window_foreign_new_for_display(
      display, static_cast<Window>(reinterpret_cast<uintptr_t>(parent)));
    if (!hostWindow) return nullptr;
    g_object_unref(hostWindow);
    mPlug = gtk_plug_new(static_cast<Window>(reinterpret_cast<uintptr_t>(parent)));
    if (!mPlug) return nullptr;
    mView = webkit_web_view_new();
    auto* manager = webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(mView));
    webkit_user_content_manager_register_script_message_handler(manager, "iplug");
    g_signal_connect(manager, "script-message-received::iplug", G_CALLBACK(+[](
      WebKitUserContentManager*, WebKitJavascriptResult* result, gpointer data) {
      auto* self = static_cast<IWebViewImpl*>(data);
      char* text = jsc_value_to_string(webkit_javascript_result_get_js_value(result));
      if (text) { self->mOwner->OnMessageFromWebView(text); g_free(text); }
    }), this);
    auto* bridge = webkit_user_script_new(
      "window.IPlugSendMsg = function(msg) { window.webkit.messageHandlers.iplug.postMessage(JSON.stringify(msg)); };",
      WEBKIT_USER_CONTENT_INJECT_TOP_FRAME, WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START, nullptr, nullptr);
    webkit_user_content_manager_add_script(manager, bridge);
    webkit_user_script_unref(bridge);
    g_signal_connect(mView, "decide-policy", G_CALLBACK(+[](
      WebKitWebView*, WebKitPolicyDecision* decision, WebKitPolicyDecisionType type, gpointer data) -> gboolean {
      if (type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION) return FALSE;
      const char* uri = webkit_uri_request_get_uri(webkit_navigation_policy_decision_get_request(WEBKIT_NAVIGATION_POLICY_DECISION(decision)));
      if (static_cast<IWebViewImpl*>(data)->mOwner->OnCanNavigateToURL(uri)) return FALSE;
      webkit_policy_decision_ignore(decision);
      return TRUE;
    }), this);
    g_signal_connect(mView, "load-changed", G_CALLBACK(+[](
      WebKitWebView*, WebKitLoadEvent event, gpointer data) {
      auto* self = static_cast<IWebViewImpl*>(data);
      if (event == WEBKIT_LOAD_STARTED) self->mLoadFailed = false;
      if (event == WEBKIT_LOAD_FINISHED && !self->mLoadFailed)
        self->mOwner->OnWebContentLoaded();
    }), this);
    g_signal_connect(mView, "load-failed", G_CALLBACK(+[](
      WebKitWebView*, WebKitLoadEvent, const char*, GError*, gpointer data) -> gboolean {
      static_cast<IWebViewImpl*>(data)->mLoadFailed = true;
      return FALSE;
    }), this);
    gtk_container_add(GTK_CONTAINER(mPlug), mView);
    SetWebViewBounds(x, y, w, h, scale);
    gtk_widget_show_all(mPlug);
    if (!gtk_plug_get_embedded(GTK_PLUG(mPlug))) { CloseWebView(); return nullptr; }
    mOwner->OnWebViewReady();
    return reinterpret_cast<void*>(static_cast<uintptr_t>(gtk_plug_get_id(GTK_PLUG(mPlug))));
  }

  void CloseWebView()
  {
    if (mPlug)
    {
      if (mView)
      {
        g_signal_handlers_disconnect_by_data(mView, this);
        auto* manager = webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(mView));
        g_signal_handlers_disconnect_by_data(manager, this);
      }
      gtk_widget_destroy(mPlug);
      mPlug = nullptr;
      mView = nullptr;
    }
  }
  void HideWebView(bool hide) { if (mPlug) gtk_widget_set_visible(mPlug, !hide); }
  void LoadHTML(const char* html) { if (mView) webkit_web_view_load_html(WEBKIT_WEB_VIEW(mView), html, nullptr); }
  void LoadURL(const char* url) { if (mView) webkit_web_view_load_uri(WEBKIT_WEB_VIEW(mView), url); }
  void LoadFile(const char* fileName, const char*)
  {
    if (!mView || !fileName) return;
    if (PathHasDotDotComponent(fileName)) return;
    namespace fs = std::filesystem;
    fs::path path(fileName);
    if (!path.is_absolute())
    {
      Dl_info info {};
      if (!dladdr(reinterpret_cast<void*>(&IPlugLinuxWebViewModuleAnchor), &info)) return;
      fs::path directory = fs::path(info.dli_fname).parent_path();
      // VST3 binaries live in <bundle>/Contents/<arch>/; CLAP uses a
      // per-plugin .clap.resources sidecar so multiple CLAPs never collide.
      if (directory.parent_path().filename() == "Contents")
        directory = directory.parent_path().parent_path();
      if (fs::path(info.dli_fname).extension() == ".clap")
        path = fs::path(info.dli_fname).string() + ".resources/web/" + path.string();
      else
        path = directory / "Resources" / "web" / path;
    }
    mWebRoot.Set(path.parent_path().string().c_str());
    GError* error = nullptr;
    char* uri = g_filename_to_uri(path.string().c_str(), nullptr, &error);
    if (uri && fs::exists(path)) { LoadURL(uri); }
    else g_warning("iPlug2 WebView: missing file %s", path.string().c_str());
    g_free(uri);
    if (error) g_error_free(error);
  }
  void ReloadPageContent() { if (mView) webkit_web_view_reload(WEBKIT_WEB_VIEW(mView)); }
  void EvaluateJavaScript(const char* script, IWebView::completionHandlerFunc func)
  {
    if (!mView) return;
    auto* callback = new IWebView::completionHandlerFunc(std::move(func));
    webkit_web_view_run_javascript(WEBKIT_WEB_VIEW(mView), script, nullptr, +[](
      GObject* source, GAsyncResult* result, gpointer data) {
      std::unique_ptr<IWebView::completionHandlerFunc> callback(static_cast<IWebView::completionHandlerFunc*>(data));
      WebKitJavascriptResult* value = webkit_web_view_run_javascript_finish(WEBKIT_WEB_VIEW(source), result, nullptr);
      if (value)
      {
        if (*callback)
        {
          char* text = jsc_value_to_string(webkit_javascript_result_get_js_value(value));
          (*callback)(text);
          g_free(text);
        }
        webkit_javascript_result_unref(value);
      }
    }, callback);
  }
  void EnableScroll(bool) {}
  void EnableInteraction(bool enable) { if (mView) gtk_widget_set_sensitive(mView, enable); }
  void SetWebViewBounds(float, float, float w, float h, float scale)
  {
    if (mPlug)
    {
      const int width = std::max(1, static_cast<int>(std::ceil(w * scale)));
      const int height = std::max(1, static_cast<int>(std::ceil(h * scale)));
      gtk_widget_set_size_request(mPlug, width, height);
      if (gtk_widget_get_realized(mPlug))
      {
        gtk_window_resize(GTK_WINDOW(mPlug), width, height);
        XResizeWindow(gdk_x11_get_default_xdisplay(), gtk_plug_get_id(GTK_PLUG(mPlug)), width, height);
      }
    }
  }
  void GetWebRoot(WDL_String& path) const { path.Set(mWebRoot.Get()); }
  void GetLocalDownloadPathForFile(const char* fileName, WDL_String& path)
  {
    DesktopPath(path);
    path.Append("/");
    path.Append(fileName);
  }
private:
  IWebView* mOwner;
  GtkWidget* mPlug = nullptr;
  GtkWidget* mView = nullptr;
  bool mLoadFailed = false;
  WDL_String mWebRoot;
};

END_IPLUG_NAMESPACE
#include "IPlugWebView.cpp"
