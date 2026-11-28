// Exercise the plug-in's floating CLAP GUI from the host main thread.
#include <clap/clap.h>
#include <X11/Xlib.h>
#include <atomic>
#include <initializer_list>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>

static void Require(bool condition, const char* message)
{
  if (!condition)
  {
    std::fprintf(stderr, "CLAP X11 floating test: %s\n", message);
    std::exit(1);
  }
}

struct Host
{
  clap_id mTimer = CLAP_INVALID_ID;
  clap_id mNextTimer = 0;
  std::atomic<bool> mCallbackRequested{false};
  bool mGuiClosed = false;

  static Host& From(const clap_host_t* host)
  {
    return *static_cast<Host*>(host->host_data);
  }

  static const void* GetExtension(const clap_host_t*, const char* id)
  {
    static const clap_host_timer_support_t timers{
      [](const clap_host_t* host, uint32_t period, clap_id* timer) {
        auto& state = From(host);
        Require(period > 0 && state.mTimer == CLAP_INVALID_ID, "unexpected timer registration");
        state.mTimer = ++state.mNextTimer;
        *timer = state.mTimer;
        return true;
      },
      [](const clap_host_t* host, clap_id timer) {
        auto& state = From(host);
        Require(timer != CLAP_INVALID_ID && timer == state.mTimer, "unexpected timer removal");
        state.mTimer = CLAP_INVALID_ID;
        return true;
      }
    };
    static const clap_host_gui_t gui{
      [](const clap_host_t*) {},
      [](const clap_host_t*, uint32_t, uint32_t) {
        Require(false, "floating editor asked the host to resize it");
        return false;
      },
      [](const clap_host_t*) { return false; },
      [](const clap_host_t*) { return false; },
      [](const clap_host_t* host, bool wasDestroyed) {
        Require(wasDestroyed && !From(host).mGuiClosed, "unexpected notification of editor close");
        From(host).mGuiClosed = true;
      }
    };
    if (std::strcmp(id, CLAP_EXT_TIMER_SUPPORT) == 0)
      return &timers;
    if (std::strcmp(id, CLAP_EXT_GUI) == 0)
      return &gui;
    return nullptr;
  }
};

static Window FindWindow(Display* display)
{
  Window root, parent, *children = nullptr;
  unsigned count = 0;
  Window found = 0;
  Require(XQueryTree(display, DefaultRootWindow(display), &root, &parent, &children, &count), "could not query root window");
  for (unsigned i = 0; i < count; ++i)
  {
    char* name = nullptr;
    if (XFetchName(display, children[i], &name) && name)
    {
      if (!std::strcmp(name, "iPlug2 X11 floating test"))
        found = children[i];
      XFree(name);
    }
  }
  if (children)
    XFree(children);
  return found;
}

int main(int argc, char** argv)
{
  Require(argc == 2, "expected a .clap path");
  Host state;
  Display* display = XOpenDisplay(nullptr);
  Require(display != nullptr, "could not open X11 display");
  const clap_host_t host{
    CLAP_VERSION, &state, "X11 floating test host", "iPlug2", "", "1",
    Host::GetExtension, [](const clap_host_t*) {}, [](const clap_host_t*) {},
    [](const clap_host_t* host) { Host::From(host).mCallbackRequested = true; }
  };
  auto* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
  Require(library != nullptr, library ? "" : dlerror());
  auto* entry = static_cast<const clap_plugin_entry_t*>(dlsym(library, "clap_entry"));
  Require(entry && entry->init(argv[1]), "could not initialize module");
  auto* factory = static_cast<const clap_plugin_factory_t*>(entry->get_factory(CLAP_PLUGIN_FACTORY_ID));
  Require(factory && factory->get_plugin_count(factory) > 0, "missing plug-in factory");
  auto* descriptor = factory->get_plugin_descriptor(factory, 0);
  auto* plugin = factory->create_plugin(factory, &host, descriptor->id);
  Require(plugin && plugin->init(plugin), "could not initialize plug-in");
  auto* gui = static_cast<const clap_plugin_gui_t*>(plugin->get_extension(plugin, CLAP_EXT_GUI));
  auto* timers = static_cast<const clap_plugin_timer_support_t*>(plugin->get_extension(plugin, CLAP_EXT_TIMER_SUPPORT));
  Require(gui && timers, "missing GUI or timer extension");
  Require(gui->is_api_supported(plugin, CLAP_WINDOW_API_X11, false) &&
          gui->is_api_supported(plugin, CLAP_WINDOW_API_X11, true), "missing X11 API");
  for (const char* api : {CLAP_WINDOW_API_X11, ""})
  {
    state.mGuiClosed = false;
    Require(gui->create(plugin, api, true), "create failed");
    gui->suggest_title(plugin, "iPlug2 X11 floating test");
    Require(gui->show(plugin), "show failed");
    const Window window = FindWindow(display);
    Require(window != 0, "floating X11 window not found");
    auto checkVisibility = [&](int expected) {
      XWindowAttributes attrs{};
      Require(FindWindow(display) == window && XGetWindowAttributes(display, window, &attrs) &&
              attrs.map_state == expected, "window identity or map state changed unexpectedly");
    };
    checkVisibility(IsViewable);
    const clap_id timer = state.mTimer;
    Require(gui->hide(plugin), "hide failed");
    checkVisibility(IsUnmapped);
    Require(gui->show(plugin), "reshow failed");
    checkVisibility(IsViewable);
    Require(state.mTimer == timer && timer != CLAP_INVALID_ID, "timer changed across hide/show");

    XEvent event{};
    event.xclient.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = XInternAtom(display, "WM_PROTOCOLS", False);
    event.xclient.format = 32;
    event.xclient.data.l[0] = XInternAtom(display, "WM_DELETE_WINDOW", False);
    Require(XSendEvent(display, window, False, NoEventMask, &event), "could not send close event");
    XSync(display, False);
    timers->on_timer(plugin, timer);
    timers->on_timer(plugin, timer);
    Require(state.mGuiClosed && state.mTimer == CLAP_INVALID_ID && !FindWindow(display), "native close failed");
    Require(!gui->show(plugin), "show succeeded before destroy acknowledgement");
    gui->destroy(plugin);
  }
  plugin->destroy(plugin);
  entry->deinit();
  dlclose(library);
  XCloseDisplay(display);
  std::puts("CLAP X11 floating lifecycle passed");
}
