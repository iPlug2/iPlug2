// Standalone feasibility check: can WebKitGTK's GtkPlug live inside a raw X11
// host window (the kind of parent supplied to CLAP/VST3 editors)?
#include <X11/Xlib.h>
#include <gtk/gtkx.h>
#include <webkit2/webkit2.h>
#include <chrono>
#include <cstdio>
#include <cstring>

struct Probe
{
  bool loaded = false;
  bool message = false;
};

int main(int argc, char** argv)
{
  if (!gtk_init_check(&argc, &argv)) return 1;
  Display* display = XOpenDisplay(nullptr);
  if (!display) return 1;
  Window parent = XCreateSimpleWindow(display, DefaultRootWindow(display), 10, 10, 320, 240, 0, 0, 0);
  XMapWindow(display, parent);
  XSync(display, False);

  GtkWidget* plug = gtk_plug_new(parent);
  GtkWidget* view = webkit_web_view_new();
  Probe state;
  auto* manager = webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(view));
  g_signal_connect(manager, "script-message-received::probe", G_CALLBACK(+[](
    WebKitUserContentManager*, WebKitJavascriptResult* result, gpointer context) {
      auto* probe = static_cast<Probe*>(context);
      JSCValue* value = webkit_javascript_result_get_js_value(result);
      char* text = jsc_value_to_string(value);
      probe->message = text && strcmp(text, "ok") == 0;
      g_free(text);
    }), &state);
  webkit_user_content_manager_register_script_message_handler(manager, "probe");
  g_signal_connect(view, "load-changed", G_CALLBACK(+[](
    WebKitWebView*, WebKitLoadEvent event, gpointer context) {
      if (event == WEBKIT_LOAD_FINISHED) static_cast<Probe*>(context)->loaded = true;
    }), &state);
  gtk_container_add(GTK_CONTAINER(plug), view);
  gtk_widget_show_all(plug);
  webkit_web_view_load_html(WEBKIT_WEB_VIEW(view),
    "<html><body><script>window.webkit.messageHandlers.probe.postMessage('ok')</script>probe</body></html>",
    "file:///");

  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(8);
  while ((!state.loaded || !state.message) && std::chrono::steady_clock::now() < deadline)
  {
    // WebKit may keep sources permanently ready; drain one iteration per tick
    // so the timeout still applies if a page fails to load.
    g_main_context_iteration(nullptr, FALSE);
    g_usleep(1000);
  }
  Window root = 0, actualParent = 0, *children = nullptr;
  unsigned count = 0;
  const Window plugID = gtk_plug_get_id(GTK_PLUG(plug));
  const bool hasParent = XQueryTree(display, plugID, &root, &actualParent, &children, &count);
  if (children) XFree(children);
  printf("WebKitGTK loaded=%d JS-message=%d plug-parent=%s\n", state.loaded,
         state.message, hasParent && actualParent == parent ? "host" : "not-host");
  const bool embedded = hasParent && actualParent == parent;
  gtk_widget_destroy(plug);
  XDestroyWindow(display, parent);
  XCloseDisplay(display);
  return state.loaded && state.message && embedded ? 0 : 1;
}
