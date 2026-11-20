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
#include <cstdlib>

// GTK is process-global. Host callbacks must run on the GUI/main thread.
extern "C" void IPlugLinuxWebViewPump()
{
  for (int i = 0; i < 32 && g_main_context_pending(nullptr); ++i)
    g_main_context_iteration(nullptr, FALSE);
}

// dladdr needs an address inside this module to resolve the module's path
static void IPlugLinuxWebViewModuleAnchor() {}

BEGIN_IPLUG_NAMESPACE

class IWebViewImpl
{
public:
  explicit IWebViewImpl(IWebView* pOwner)
    : mOwner(pOwner)
  {
  }

  ~IWebViewImpl() { CloseWebView(); }

  void* OpenWebView(void* pParent, float x, float y, float w, float h, float scale)
  {
    CloseWebView();

    if (!pParent)
      return nullptr;

    // GtkPlug is X11-only. In a Wayland session GDK would otherwise prefer the
    // Wayland backend even though the host window is an (XWayland) X11 window.
    // No effect if the process has already opened a GDK display.
    gdk_set_allowed_backends("x11");

    // gtk_init_check() would otherwise call setlocale(LC_ALL, ""), changing
    // e.g. the decimal separator for the host and every other plugin.
    gtk_disable_setlocale();

    if (!gtk_init_check(nullptr, nullptr))
      return nullptr;

    // Reject stale/invalid XIDs before gtk_plug_new issues X requests. GDK's
    // own connection may otherwise turn a BadWindow into a process abort.
    GdkDisplay* pDisplay = gdk_display_get_default();

    if (!GDK_IS_X11_DISPLAY(pDisplay))
      return nullptr;

    Window parentXID = static_cast<Window>(reinterpret_cast<uintptr_t>(pParent));
    GdkWindow* pOwnWindow = gdk_x11_window_lookup_for_display(pDisplay, parentXID);

    // GDK also caches foreign wrappers for host windows from earlier opens.
    if (pOwnWindow && gdk_window_get_window_type(pOwnWindow) != GDK_WINDOW_FOREIGN)
    {
      // The parent belongs to this process's GDK (standalone app). GtkPlug can
      // only embed into a foreign window, so interpose a plain X11 child that
      // GDK does not track, placed below the container's menu bar.
      const char* menuOffset = getenv("IPLUG2_MENU_OFFSET");
      mHostScale = std::max(1, gdk_window_get_scale_factor(pOwnWindow));
      Display* pXDisplay = gdk_x11_display_get_xdisplay(pDisplay);
      mHostChild = XCreateSimpleWindow(pXDisplay, parentXID, 0, menuOffset ? std::clamp(atoi(menuOffset), 0, 100) : 0, std::max(1, static_cast<int>(std::ceil(w * scale))) * mHostScale,
                                       std::max(1, static_cast<int>(std::ceil(h * scale))) * mHostScale, 0, 0, 0);
      XMapWindow(pXDisplay, mHostChild);
      XSync(pXDisplay, False);
      parentXID = mHostChild;
    }

    GdkWindow* pHostWindow = gdk_x11_window_foreign_new_for_display(pDisplay, parentXID);

    if (!pHostWindow)
    {
      CloseWebView();
      return nullptr;
    }

    g_object_unref(pHostWindow);
    mPlug = gtk_plug_new(parentXID);

    if (!mPlug)
    {
      CloseWebView();
      return nullptr;
    }

    // GTK destroys the plug itself if the host destroys the parent window first
    g_signal_connect(mPlug, "destroy", G_CALLBACK(OnPlugDestroyed), this);

    mView = webkit_web_view_new();
    auto* pManager = webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(mView));
    webkit_user_content_manager_register_script_message_handler(pManager, "iplug");
    g_signal_connect(pManager, "script-message-received::iplug", G_CALLBACK(OnScriptMessage), this);

    auto* pBridge = webkit_user_script_new("window.IPlugSendMsg = function(msg) { window.webkit.messageHandlers.iplug.postMessage(JSON.stringify(msg)); };", WEBKIT_USER_CONTENT_INJECT_TOP_FRAME,
                                           WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START, nullptr, nullptr);
    webkit_user_content_manager_add_script(pManager, pBridge);
    webkit_user_script_unref(pBridge);

    g_signal_connect(mView, "decide-policy", G_CALLBACK(OnDecidePolicy), this);
    g_signal_connect(mView, "load-changed", G_CALLBACK(OnLoadChanged), this);
    g_signal_connect(mView, "load-failed", G_CALLBACK(OnLoadFailed), this);

    if (mHostChild)
    {
      // The standalone exports its effective UI scale (which may be fractional,
      // e.g. 1.5); GDK only renders at integer factors, so zoom the difference.
      const char* str = getenv("IPLUG2_SCREEN_SCALE");
      char* pEnd = nullptr;
      const double screenScale = str ? strtod(str, &pEnd) : 0.;

      if (str && pEnd != str && *pEnd == '\0' && screenScale >= 1. && screenScale <= 8.)
        webkit_web_view_set_zoom_level(WEBKIT_WEB_VIEW(mView), screenScale / mHostScale);
    }

    gtk_container_add(GTK_CONTAINER(mPlug), mView);
    SetWebViewBounds(x, y, w, h, scale);
    gtk_widget_show_all(mPlug);

    if (!gtk_plug_get_embedded(GTK_PLUG(mPlug)))
    {
      CloseWebView();
      return nullptr;
    }

    MapPlug();
    SetWebViewBounds(x, y, w, h, scale);
    mOwner->OnWebViewReady();

    return reinterpret_cast<void*>(static_cast<uintptr_t>(gtk_plug_get_id(GTK_PLUG(mPlug))));
  }

  void CloseWebView()
  {
    if (mPlug)
    {
      g_signal_handlers_disconnect_by_data(mPlug, this);

      if (mView)
      {
        g_signal_handlers_disconnect_by_data(mView, this);
        auto* pManager = webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(mView));
        g_signal_handlers_disconnect_by_data(pManager, this);
      }

      gtk_widget_destroy(mPlug);
      mPlug = nullptr;
      mView = nullptr;
    }

    if (mHostChild)
    {
      XDestroyWindow(gdk_x11_get_default_xdisplay(), mHostChild);
      mHostChild = 0;
    }
  }

  void HideWebView(bool hide)
  {
    if (!mPlug)
      return;

    gtk_widget_set_visible(mPlug, !hide);

    if (!hide)
      MapPlug();
  }

  void LoadHTML(const char* html)
  {
    if (mView)
      webkit_web_view_load_html(WEBKIT_WEB_VIEW(mView), html, nullptr);
  }

  void LoadURL(const char* url)
  {
    if (mView)
      webkit_web_view_load_uri(WEBKIT_WEB_VIEW(mView), url);
  }

  void LoadFile(const char* fileName, const char*)
  {
    if (!mView || !fileName)
      return;

    if (PathHasDotDotComponent(fileName))
      return;

    namespace fs = std::filesystem;
    fs::path path(fileName);

    if (!path.is_absolute())
    {
      Dl_info info{};

      if (!dladdr(reinterpret_cast<void*>(&IPlugLinuxWebViewModuleAnchor), &info))
        return;

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

    // dladdr reports the path as given to dlopen, which may be relative;
    // g_filename_to_uri only accepts absolute paths.
    std::error_code ec;

    if (fs::path absolute = fs::absolute(path, ec); !ec)
      path = absolute;

    mWebRoot.Set(path.parent_path().string().c_str());
    GError* pError = nullptr;
    char* uri = g_filename_to_uri(path.string().c_str(), nullptr, &pError);

    if (uri && fs::exists(path, ec))
      LoadURL(uri);
    else
      g_warning("iPlug2 WebView: missing file %s", path.string().c_str());

    g_free(uri);

    if (pError)
      g_error_free(pError);
  }

  void ReloadPageContent()
  {
    if (mView)
      webkit_web_view_reload(WEBKIT_WEB_VIEW(mView));
  }

  void EvaluateJavaScript(const char* script, IWebView::completionHandlerFunc func)
  {
    if (!mView)
      return;

    auto* pCallback = new IWebView::completionHandlerFunc(std::move(func));
    webkit_web_view_run_javascript(WEBKIT_WEB_VIEW(mView), script, nullptr, OnJavaScriptFinished, pCallback);
  }

  void EnableScroll(bool) {}

  void EnableInteraction(bool enable)
  {
    if (mView)
      gtk_widget_set_sensitive(mView, enable);
  }

  void SetWebViewBounds(float, float, float w, float h, float scale)
  {
    if (!mPlug)
      return;

    // Plugin hosts size the editor in X11 (physical) pixels. The standalone
    // container is sized in GDK units, which GDK multiplies by its scale.
    const int unit = mHostChild ? mHostScale : 1;
    const int width = std::max(1, static_cast<int>(std::ceil(w * scale))) * unit;
    const int height = std::max(1, static_cast<int>(std::ceil(h * scale))) * unit;

    // GTK sizes are in GDK units; keep the X11 window at the exact pixel size.
    const int gdkScale = std::max(1, gtk_widget_get_scale_factor(mPlug));
    const int gtkWidth = std::max(1, (width + gdkScale - 1) / gdkScale);
    const int gtkHeight = std::max(1, (height + gdkScale - 1) / gdkScale);

    if (mHostChild)
      XResizeWindow(gdk_x11_get_default_xdisplay(), mHostChild, width, height);

    gtk_widget_set_size_request(mPlug, gtkWidth, gtkHeight);

    if (gtk_widget_get_realized(mPlug))
    {
      gtk_window_resize(GTK_WINDOW(mPlug), gtkWidth, gtkHeight);
      XResizeWindow(gdk_x11_get_default_xdisplay(), gtk_plug_get_id(GTK_PLUG(mPlug)), width, height);
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
  static void OnPlugDestroyed(GtkWidget*, gpointer pData)
  {
    auto* pSelf = static_cast<IWebViewImpl*>(pData);
    pSelf->mPlug = nullptr;
    pSelf->mView = nullptr;
  }

  static void OnScriptMessage(WebKitUserContentManager*, WebKitJavascriptResult* pResult, gpointer pData)
  {
    auto* pSelf = static_cast<IWebViewImpl*>(pData);
    char* text = jsc_value_to_string(webkit_javascript_result_get_js_value(pResult));

    if (text)
    {
      pSelf->mOwner->OnMessageFromWebView(text);
      g_free(text);
    }
  }

  static gboolean OnDecidePolicy(WebKitWebView*, WebKitPolicyDecision* pDecision, WebKitPolicyDecisionType type, gpointer pData)
  {
    if (type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION)
      return FALSE;

    const char* uri = webkit_uri_request_get_uri(webkit_navigation_policy_decision_get_request(WEBKIT_NAVIGATION_POLICY_DECISION(pDecision)));

    if (static_cast<IWebViewImpl*>(pData)->mOwner->OnCanNavigateToURL(uri))
      return FALSE;

    webkit_policy_decision_ignore(pDecision);
    return TRUE;
  }

  static void OnLoadChanged(WebKitWebView*, WebKitLoadEvent event, gpointer pData)
  {
    auto* pSelf = static_cast<IWebViewImpl*>(pData);

    if (event == WEBKIT_LOAD_STARTED)
      pSelf->mLoadFailed = false;

    if (event == WEBKIT_LOAD_FINISHED && !pSelf->mLoadFailed)
      pSelf->mOwner->OnWebContentLoaded();
  }

  static gboolean OnLoadFailed(WebKitWebView*, WebKitLoadEvent, const char*, GError*, gpointer pData)
  {
    static_cast<IWebViewImpl*>(pData)->mLoadFailed = true;
    return FALSE;
  }

  static void OnJavaScriptFinished(GObject* pSource, GAsyncResult* pResult, gpointer pData)
  {
    std::unique_ptr<IWebView::completionHandlerFunc> callback(static_cast<IWebView::completionHandlerFunc*>(pData));
    WebKitJavascriptResult* pValue = webkit_web_view_run_javascript_finish(WEBKIT_WEB_VIEW(pSource), pResult, nullptr);

    if (pValue)
    {
      if (*callback)
      {
        char* text = jsc_value_to_string(webkit_javascript_result_get_js_value(pValue));
        (*callback)(text);
        g_free(text);
      }

      webkit_javascript_result_unref(pValue);
    }
  }

  // XEMBED leaves mapping the client to the embedder (normally a GtkSocket).
  // Plugin hosts hand over a plain X11 window, so the plug must map itself.
  void MapPlug()
  {
    if (GdkWindow* pWindow = gtk_widget_get_window(mPlug))
      gdk_window_show(pWindow);
  }

  IWebView* mOwner;
  GtkWidget* mPlug = nullptr;
  GtkWidget* mView = nullptr;
  Window mHostChild = 0; // only when the parent is this process's own GDK window
  int mHostScale = 1;
  bool mLoadFailed = false;
  WDL_String mWebRoot;
};

END_IPLUG_NAMESPACE

#include "IPlugWebView.cpp"
