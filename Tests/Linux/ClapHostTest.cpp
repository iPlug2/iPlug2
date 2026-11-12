#include <clap/clap.h>
#include <dlfcn.h>
#include <poll.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <limits>
#include <thread>

#include "X11Test.h"

using Clock = std::chrono::steady_clock;
static int sFailures = 0;

static void Check(bool condition, const char* message)
{
  if (!condition)
  {
    fprintf(stderr, "FAIL: %s\n", message);
    ++sFailures;
  }
}

#include "ClapAudioTest.h"

struct Host
{
  clap_host_t api {};
  const std::thread::id mainThread = std::this_thread::get_id();
  bool offerTimer = true;
  bool offerFd = true;
  bool rejectTimer = false;
  bool rejectFd = false;
  bool timerActive = false;
  int fd = -1;
  unsigned registeredTimers = 0;
  unsigned registeredFds = 0;
  unsigned unregisteredTimers = 0;
  unsigned unregisteredFds = 0;
  std::atomic<bool> pending {false};
  std::atomic<unsigned> requests {0};

  Host()
  {
    api.clap_version = CLAP_VERSION;
    api.host_data = this;
    api.name = "Linux regression host";
    api.vendor = "iPlug2";
    api.url = "https://iplug2.github.io";
    api.version = "1.0.0";
    api.get_extension = GetExtension;
    api.request_restart = [](const clap_host_t*) {};
    api.request_process = [](const clap_host_t*) {};
    api.request_callback = [](const clap_host_t* host) {
      auto& self = Get(host);
      ++self.requests;
      self.pending = true;
    };
  }

  static Host& Get(const clap_host_t* host)
  {
    return *static_cast<Host*>(host->host_data);
  }

  static const void* GetExtension(const clap_host_t* host, const char* name)
  {
    auto& self = Get(host);
    static const clap_host_timer_support_t timer {
      [](const clap_host_t* host, uint32_t period, clap_id* id) {
        auto& self = Get(host);
        Check(std::this_thread::get_id() == self.mainThread, "register timer on main thread");
        Check(period >= 1, "positive timer interval");
        if (self.rejectTimer)
          return false;
        Check(!self.timerActive, "timer registered only once");
        self.timerActive = true;
        ++self.registeredTimers;
        *id = 101;
        return true;
      },
      [](const clap_host_t* host, clap_id id) {
        auto& self = Get(host);
        Check(std::this_thread::get_id() == self.mainThread, "unregister timer on main thread");
        Check(id == 101 && self.timerActive, "unregister only an active timer");
        self.timerActive = false;
        ++self.unregisteredTimers;
        return true;
      }
    };
    static const clap_host_posix_fd_support_t fd {
      [](const clap_host_t* host, int fd, clap_posix_fd_flags_t flags) {
        auto& self = Get(host);
        Check(std::this_thread::get_id() == self.mainThread, "register fd on main thread");
        Check(fd >= 0 && flags == CLAP_POSIX_FD_READ, "valid X11 read descriptor");
        if (self.rejectFd)
          return false;
        Check(self.fd == -1, "fd registered only once");
        self.fd = fd;
        ++self.registeredFds;
        return true;
      },
      [](const clap_host_t*, int, clap_posix_fd_flags_t) { return true; },
      [](const clap_host_t* host, int fd) {
        auto& self = Get(host);
        Check(std::this_thread::get_id() == self.mainThread, "unregister fd on main thread");
        Check(fd == self.fd && self.fd >= 0, "unregister only an active fd");
        self.fd = -1;
        ++self.unregisteredFds;
        return true;
      }
    };
    static const clap_host_thread_check_t threads {
      [](const clap_host_t* host) { return Get(host).mainThread == std::this_thread::get_id(); },
      [](const clap_host_t*) { return sAudioThread; }
    };
    static const clap_host_params_t params {
      [](const clap_host_t*, clap_param_rescan_flags) {},
      [](const clap_host_t*, clap_id, clap_param_clear_flags) {},
      [](const clap_host_t*) {}
    };
    if (strcmp(name, CLAP_EXT_TIMER_SUPPORT) == 0)
      return self.offerTimer ? &timer : nullptr;
    if (strcmp(name, CLAP_EXT_POSIX_FD_SUPPORT) == 0)
      return self.offerFd ? &fd : nullptr;
    if (strcmp(name, CLAP_EXT_THREAD_CHECK) == 0)
      return &threads;
    if (strcmp(name, CLAP_EXT_PARAMS) == 0)
      return &params;
    return nullptr;
  }

  void Pump(const clap_plugin_t* plugin)
  {
    if (pending.exchange(false))
      plugin->on_main_thread(plugin);
    if (timerActive)
    {
      const auto* timer = static_cast<const clap_plugin_timer_support_t*>(
        plugin->get_extension(plugin, CLAP_EXT_TIMER_SUPPORT));
      Check(timer != nullptr, "plugin timer extension");
      if (timer)
        timer->on_timer(plugin, 101);
    }
    if (fd >= 0)
    {
      pollfd descriptor {fd, POLLIN, 0};
      if (poll(&descriptor, 1, 0) > 0 && (descriptor.revents & POLLIN))
      {
        const auto* events = static_cast<const clap_plugin_posix_fd_support_t*>(
          plugin->get_extension(plugin, CLAP_EXT_POSIX_FD_SUPPORT));
        Check(events != nullptr, "plugin fd extension");
        if (events)
          events->on_fd(plugin, fd, CLAP_POSIX_FD_READ);
      }
    }
  }

  bool WaitForRender(const clap_plugin_t* plugin, Display* display, Window parent)
  {
    const auto deadline = Clock::now() + std::chrono::seconds(3);
    while (Clock::now() < deadline)
    {
      Pump(plugin);
      const Window editor = FindEditor(display, parent);
      if (editor && IsRendered(display, editor))
        return true;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return false;
  }

  void CheckUnregistered()
  {
    Check(!timerActive && fd == -1, "GUI callbacks inactive after hide or destroy");
    Check(registeredTimers == unregisteredTimers, "all successful timer registrations released");
    Check(registeredFds == unregisteredFds, "all successful fd registrations released");
  }
};

int main(int argc, char** argv)
{
  if (argc != 3)
    return 1;
  const std::string mode(argv[2]);
  Host host;
  host.offerTimer = mode != "none" && mode != "fd-only" && mode != "fallback-fail" &&
    mode != "native-close-fallback" && mode != "idle-none" && mode != "close-order-fallback";
  host.offerFd = mode != "none" && mode != "timer-only" && mode != "native-close-fallback" &&
    mode != "idle-none" && mode != "close-order-fallback" && mode != "close-order-poll";
  host.rejectTimer = mode == "timer-fail" || mode == "both-fail";
  host.rejectFd = mode == "fd-fail" || mode == "both-fail";
  void* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
  if (!library)
  {
    fprintf(stderr, "Could not load CLAP test plugin: %s\n", dlerror());
    return 1;
  }
  const auto* entry = static_cast<const clap_plugin_entry_t*>(dlsym(library, "clap_entry"));
  if (!entry || !entry->init(argv[1]))
  {
    fprintf(stderr, "Could not initialize CLAP entry\n");
    dlclose(library);
    return 1;
  }
  const auto* factory = static_cast<const clap_plugin_factory_t*>(
    entry->get_factory(CLAP_PLUGIN_FACTORY_ID));
  if (!factory || factory->get_plugin_count(factory) == 0)
  {
    fprintf(stderr, "Missing CLAP plugin factory\n");
    entry->deinit();
    dlclose(library);
    return 1;
  }
  const auto* descriptor = factory->get_plugin_descriptor(factory, 0);
  const bool closeProbe = mode.compare(0, 11, "close-order") == 0;
  const bool idleProbe = mode.compare(0, 4, "idle") == 0 || closeProbe;
  const auto idleCount = reinterpret_cast<unsigned (*)(unsigned)>(dlsym(library, "LinuxIdleProbeCount"));
  const auto queue = reinterpret_cast<void (*)()>(dlsym(library, "LinuxIdleProbeQueue"));
  if (idleProbe)
    Check(idleCount && queue, "load CLAP idle probe hooks");
  if (mode == "idle-fail")
    Check(setenv("IPLUG_LINUX_TEST_MODE", "core-timer-fail", 1) == 0, "inject idle timer failure");
  const auto* plugin = factory->create_plugin(factory, &host.api, descriptor->id);
  const bool initialized = plugin && plugin->init(plugin);
  if (mode == "idle-fail")
  {
    Check(!initialized, "CLAP initialization reports idle timer failure");
    if (plugin)
      plugin->destroy(plugin);
    unsetenv("IPLUG_LINUX_TEST_MODE");
    entry->deinit();
    dlclose(library);
    return sFailures ? 1 : 0;
  }
  if (!initialized)
  {
    fprintf(stderr, "Could not initialize CLAP plugin\n");
    if (plugin)
      plugin->destroy(plugin);
    entry->deinit();
    dlclose(library);
    return 1;
  }
  if (mode == "audio-state")
  {
    TestAudio(plugin);
    plugin->destroy(plugin);
    entry->deinit();
    dlclose(library);
    return sFailures ? 1 : 0;
  }
  if (idleProbe)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    Check(idleCount(0) == 0 && idleCount(2) == 0, "idle and queued UI messages wait for host dispatch");
    Check(host.requests == 1, "headless idle callbacks are coalesced");
    host.Pump(plugin);
    Check(idleCount(0) > 0 && idleCount(2) > 0, "idle and UI messages run before an editor exists");
    Check(idleCount(1) == 0, "headless idle and messages run on the main thread");
  }
  if (mode == "idle-headless")
  {
    Check(!plugin->get_extension(plugin, CLAP_EXT_GUI), "headless CLAP offers no editor");
    queue();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    host.Pump(plugin);
    Check(idleCount(2) >= 2 && idleCount(1) == 0, "headless CLAP keeps delivering UI messages on main");
    plugin->destroy(plugin);
    const unsigned stopped = host.requests;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    Check(host.requests == stopped, "headless CLAP teardown stops callback requests");
    entry->deinit();
    dlclose(library);
    return sFailures ? 1 : 0;
  }
  const auto* gui = static_cast<const clap_plugin_gui_t*>(
    plugin->get_extension(plugin, CLAP_EXT_GUI));
  if (!gui || !gui->create(plugin, CLAP_WINDOW_API_X11, false))
  {
    fprintf(stderr, "Could not create CLAP GUI\n");
    plugin->destroy(plugin);
    entry->deinit();
    dlclose(library);
    return 1;
  }
  if (mode == "editor-fail")
  {
    clap_window_t parent {};
    parent.api = CLAP_WINDOW_API_X11;
    parent.x11 = 1;
    Check(!gui->set_parent(plugin, &parent), "non-IGraphics editor attach reports failure");
    Check(!gui->show(plugin), "non-IGraphics editor show reports failure");
    Check(!gui->set_parent(plugin, &parent), "failed editor attach can be retried safely");
    gui->destroy(plugin);
    host.CheckUnregistered();
    plugin->destroy(plugin);
    entry->deinit();
    dlclose(library);
    return sFailures ? 1 : 0;
  }
  if (mode == "scale")
  {
    uint32_t width = 0, height = 0;
    Check(gui->set_scale(plugin, 2.0), "accept scale before attachment");
    Check(gui->get_size(plugin, &width, &height) && width == 1200 && height == 1200,
          "initial size reflects host scale before attachment");
    Check(!gui->set_scale(plugin, 0.) && !gui->set_scale(plugin, -1.) &&
          !gui->set_scale(plugin, std::numeric_limits<double>::infinity()), "reject invalid scales");
    Check(setenv("GDK_SCALE", "3", 1) == 0, "test explicit host scale priority");
  }
  XInitThreads();
  Display* display = XOpenDisplay(nullptr);
  if (!display)
  {
    fprintf(stderr, "No X11 display for CLAP test\n");
    gui->destroy(plugin);
    plugin->destroy(plugin);
    entry->deinit();
    dlclose(library);
    return 1;
  }
  const Window window = XCreateSimpleWindow(display, DefaultRootWindow(display),
                                             0, 0, mode == "scale" ? 1200 : 600, mode == "scale" ? 1200 : 600, 0, 0, 0);
  XMapWindow(display, window);
  XSync(display, False);
  clap_window_t parent {};
  parent.api = CLAP_WINDOW_API_X11;
  parent.x11 = window;
  if (mode == "window-fail" || mode == "fallback-fail")
    Check(setenv("IPLUG_LINUX_TEST_MODE",
                 mode == "window-fail" ? "window-fail" : "core-timer-fail", 1) == 0,
          "configure fault injection");

  const bool expectFailure = mode == "window-fail" || mode == "fallback-fail";
  const bool attached = gui->set_parent(plugin, &parent);
  Check(attached == !expectFailure, "GUI attach reports actual success or failure");
  if (attached)
  {
    const bool fallback = !host.offerTimer || host.rejectTimer;
    if (fallback)
    {
      const unsigned beforeRequests = host.requests;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      Check(host.requests > 0, "fallback requests main-thread callbacks");
      Check(host.requests - beforeRequests <= 2, "GUI and idle each coalesce outstanding callbacks");
    }
    for (unsigned i = 0; i < 3; ++i)
    {
      Check(gui->show(plugin), "show attached or recreated GUI");
      if (mode == "scale")
      {
        uint32_t width = 0, height = 0;
        Check(gui->get_size(plugin, &width, &height) && width == 1200 && height == 1200,
              "host scale persists across attach and hide/show");
        const Window editor = FindEditor(display, window);
        XWindowAttributes attributes {};
        Check(editor && XGetWindowAttributes(display, editor, &attributes) &&
              attributes.width == 1200 && attributes.height == 1200, "native size matches accepted scale");
        Check(gui->set_scale(plugin, 1.25), "change scale while visible");
        Check(gui->get_size(plugin, &width, &height) && width == 750 && height == 750,
              "fractional host scale updates visible editor size");
        Check(gui->set_scale(plugin, 2.), "restore scale before hide");
      }
      Check(host.WaitForRender(plugin, display, window), "GUI renders with this host's extensions");
      Check(gui->hide(plugin), "hide GUI");
      host.CheckUnregistered();
      host.pending = false;
      const unsigned before = idleProbe ? idleCount(0) : 0;
      const unsigned messages = idleProbe ? idleCount(2) : 0;
      if (idleProbe)
        queue();
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      plugin->on_main_thread(plugin);
      if (idleProbe)
      {
        Check(idleCount(0) > before && idleCount(2) > messages,
              "hidden editor still receives idle and queued UI messages");
        Check(idleCount(1) == 0, "hidden idle and messages stay on the main thread");
      }
    }
    Check(gui->show(plugin), "reopen GUI before destroy");
    Check(host.WaitForRender(plugin, display, window), "reopened GUI renders");
    if (mode == "native-close" || mode == "native-close-fallback" || closeProbe)
    {
      const Window editor = FindEditor(display, window);
      Check(editor != 0, "find editor for native close");
      if (editor)
      {
        const unsigned closes = closeProbe ? idleCount(4) : 0;
        SendNativeClose(display, editor);
        if (closeProbe)
          Check(idleCount(4) == closes, "CLAP native close waits for main-thread dispatch");
        const auto deadline = Clock::now() + std::chrono::seconds(2);
        while (Clock::now() < deadline)
        {
          host.Pump(plugin);
          if (!host.timerActive && host.fd < 0 && !FindEditor(display, window))
            break;
          std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        host.CheckUnregistered();
        if (closeProbe)
        {
          Check(idleCount(4) == closes + 1, "CLAP native close calls OnUIClose exactly once");
          Check(idleCount(5) == 0, "CLAP OnUIClose runs on main with live controls");
          gui->hide(plugin);
          Check(idleCount(4) == closes + 1, "host hide after native close does not repeat OnUIClose");
        }
        Check(gui->show(plugin), "show editor again after native close");
        Check(host.WaitForRender(plugin, display, window), "native-closed editor can reopen");
      }
    }
  }
  if (mode == "scale")
    unsetenv("GDK_SCALE");
  gui->destroy(plugin);
  host.CheckUnregistered();
  host.pending = false;
  const unsigned before = idleProbe ? idleCount(0) : 0;
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  plugin->on_main_thread(plugin);
  if (idleProbe)
  {
    Check(idleCount(3) == idleCount(4) && idleCount(5) == 0,
          "CLAP destroy preserves paired UI callbacks and live controls");
    Check(idleCount(0) > before, "idle continues after GUI destruction");
    Check(idleCount(1) == 0, "all CLAP idle and message callbacks run on the main thread");
  }
  if (expectFailure)
    Check(unsetenv("IPLUG_LINUX_TEST_MODE") == 0, "clear fault injection");
  plugin->destroy(plugin);
  const unsigned stopped = host.requests;
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  Check(host.requests == stopped, "plugin destruction stops all worker callback requests");
  entry->deinit();
  XDestroyWindow(display, window);
  XCloseDisplay(display);
  dlclose(library);
  printf("CLAP %s: %s\n", mode.c_str(), sFailures ? "FAILED" : "passed");
  return sFailures ? 1 : 0;
}
