#include "swell.h"
#include "reaper_plugin.h"
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <thread>

static const auto sMainThread = std::this_thread::get_id();
static unsigned sErrors = 0, sDockAdds = 0;
static HWND sDocker = nullptr, sDockedWindow = nullptr;
static const char* sFailureMode = "";

extern "C" HWND __wrap__Z25SWELL_CreateXBridgeWindowP6HWND__PPvPK4RECT(
  HWND parent, void** window, const RECT* bounds)
{
  if (strcmp(sFailureMode, "bridge-fail") == 0)
  {
    *window = nullptr;
    return nullptr;
  }
  HWND bridge = SWELL_CreateXBridgeWindow(parent, window, bounds);
  if (strcmp(sFailureMode, "bridge-empty") == 0)
    *window = nullptr;
  return bridge;
}

static void Check(bool condition, const char* message)
{
  if (!condition)
  {
    fprintf(stderr, "FAIL: %s\n", message);
    ++sErrors;
  }
}

static void MainThread() { Check(std::this_thread::get_id() == sMainThread, "REAPER API on main thread"); }
static void NoArgs() { MainThread(); }
static void WindowArg(HWND) { MainThread(); }
static void MainCommand(int, int) { MainThread(); }
static const char* ResourcePath() { MainThread(); return "/tmp"; }
static const char* IniFile() { MainThread(); return nullptr; }
static void Console(const char*) { MainThread(); }
static void DockAdd(HWND, const char*, int, bool) { MainThread(); }
static void DockAddEx(HWND window, const char*, const char*, bool)
{
  MainThread();
  ++sDockAdds;
  sDockedWindow = window;
  SetParent(window, sDocker);
  SetWindowPos(window, nullptr, 25, 35, 600, 600, SWP_NOZORDER | SWP_NOACTIVATE);
  ShowWindow(window, SW_SHOW);
}
static int DockChild(HWND window, bool*) { MainThread(); return window == sDockedWindow ? 0 : -1; }
static void RectArg(RECT*) { MainThread(); }

static void* GetFunction(const char* name)
{
  MainThread();
  // Keep the mock's function signatures identical to the imported REAPER APIs.
#define API(name, function) if (strcmp(requested, #name) == 0) return reinterpret_cast<void*>(function)
  const char* requested = name;
  API(Main_OnCommand, MainCommand);
  API(GetResourcePath, ResourcePath);
  API(AddExtensionsMainMenu, NoArgs);
  API(AttachWindowTopmostButton, WindowArg);
  API(ShowConsoleMsg, Console);
  API(DockWindowAdd, DockAdd);
  API(DockWindowAddEx, DockAddEx);
  API(DockWindowRemove, WindowArg);
  API(DockWindowActivate, WindowArg);
  API(DockIsChildOfDock, DockChild);
  API(get_ini_file, IniFile);
  API(EnsureNotCompletelyOffscreen, RectArg);
#undef API
  fprintf(stderr, "Unexpected REAPER API import: %s\n", name);
  ++sErrors;
  return nullptr;
}

static int Register(const char*, void*) { MainThread(); return 1; }
INT_PTR SWELLAppMain(int, INT_PTR, INT_PTR) { return 0; }

static void Pump(unsigned milliseconds)
{
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
  while (std::chrono::steady_clock::now() < deadline)
  {
    SWELL_RunMessageLoop();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
}

static void CheckEmbedding(Display* display, Window editor, HWND container, bool docked)
{
  auto* native = static_cast<GdkWindow*>(SWELL_GetOSWindow(docked ? sDocker : container, "GdkWindow"));
  Check(native != nullptr, "container has a native X11 ancestor");
  if (!native || !editor)
    return;
  if (docked)
    Check(SWELL_GetOSWindow(container, "GdkWindow") == nullptr,
          "docked SWELL child has no native window of its own");

  Window root = 0, bridge = 0, parent = 0, *children = nullptr;
  unsigned count = 0;
  Check(XQueryTree(display, editor, &root, &bridge, &children, &count), "query editor parent");
  if (children)
    XFree(children);
  Check(bridge != 0 && bridge != root, "editor is embedded, not a root child");
  if (!bridge || bridge == root)
    return;
  children = nullptr;
  Check(XQueryTree(display, bridge, &root, &parent, &children, &count), "query bridge parent");
  if (children)
    XFree(children);
  Check(parent == GDK_WINDOW_XID(native), "bridge belongs to the expected SWELL native ancestor");

  RECT client;
  XWindowAttributes attributes {};
  // GDK updates the bridge on its own X11 connection. Wait for the requested
  // geometry rather than assuming a fixed Pump() duration also orders replies
  // on this independent test connection (especially under CPU emulation).
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
  bool queried = false;
  do
  {
    GetClientRect(container, &client);
    queried = XGetWindowAttributes(display, bridge, &attributes);
    if (queried && attributes.width == client.right - client.left &&
        attributes.height == client.bottom - client.top)
      break;
    Pump(10);
  } while (std::chrono::steady_clock::now() < deadline);
  Check(queried, "query bridge geometry");
  Check(attributes.width == client.right - client.left &&
        attributes.height == client.bottom - client.top, "bridge fills the container client area");
  Check(attributes.map_state == IsViewable, "bridge is visible");
  POINT origin {0, 0};
  ClientToScreen(container, &origin);
  int x = 0, y = 0;
  Window child = 0;
  Check(XTranslateCoordinates(display, bridge, root, 0, 0, &x, &y, &child),
        "translate bridge position to screen");
  Check(x == origin.x && y == origin.y, "bridge tracks the container client origin");
  Check(XGetWindowAttributes(display, editor, &attributes), "query editor geometry");
  Check(attributes.width == client.right - client.left &&
        attributes.height == client.bottom - client.top, "editor fills the container client area");
  Check(attributes.x == 0 && attributes.y == 0, "editor is positioned at the bridge origin");
}

template<typename T>
static T Symbol(void* module, const char* name)
{
  auto symbol = reinterpret_cast<T>(dlsym(module, name));
  if (!symbol)
  {
    fprintf(stderr, "Missing probe symbol %s: %s\n", name, dlerror());
    abort();
  }
  return symbol;
}

int main(int argc, char** argv)
{
  if (argc != 2 && argc != 3)
    return 1;
  if (argc == 3)
    sFailureMode = argv[2];
  SWELL_initargs(&argc, &argv);
  SWELL_Internal_PostMessage_Init();
  void* module = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
  if (!module)
  {
    fprintf(stderr, "Could not load REAPER probe: %s\n", dlerror());
    return 1;
  }
  auto entry = Symbol<int (*)(REAPER_PLUGIN_HINSTANCE, reaper_plugin_info_t*)>(module, "ReaperPluginEntry");
  auto show = Symbol<void (*)()>(module, "LinuxReaperProbeShow");
  auto stop = Symbol<void (*)()>(module, "LinuxReaperProbeStop");
  auto window = Symbol<void* (*)()>(module, "LinuxReaperProbeWindow");
  auto container = Symbol<HWND (*)()>(module, "LinuxReaperProbeContainer");
  auto count = Symbol<unsigned (*)(unsigned)>(module, "LinuxReaperProbeCount");
  sDocker = CreateDialog(nullptr, nullptr, nullptr,
    +[](HWND, UINT, WPARAM, LPARAM) -> INT_PTR { return 0; });
  Check(sDocker != nullptr, "create mock REAPER docker");
  SetWindowPos(sDocker, nullptr, 0, 0, 900, 800, SWP_NOZORDER | SWP_NOACTIVATE);
  ShowWindow(sDocker, SW_SHOW);
  reaper_plugin_info_t host {REAPER_PLUGIN_VERSION, nullptr, Register, GetFunction};
  if (!entry(nullptr, &host))
  {
    fprintf(stderr, "REAPER probe initialization failed\n");
    dlclose(module);
    return 1;
  }
  Pump(150);
  Check(count(0) > 0, "idle runs before opening an editor");
  if (sFailureMode[0])
  {
    show();
    Pump(150);
    Check(container() == nullptr && window() == nullptr, "failed attachment closes the container and editor");
    Check(count(1) == 0 && count(2) == 0, "failed attachment does not report an opened editor");
    const auto idle = count(0);
    Pump(100);
    Check(count(0) > idle, "idle continues after failed attachment");
    sFailureMode = "";
    unsetenv("IPLUG_LINUX_TEST_MODE");
  }
  show();
  Pump(200);
  Check(count(1) == 1 && count(4) > 0, "editor opens and draws");
  Display* display = XOpenDisplay(nullptr);
  Check(display != nullptr, "open test X11 connection");
  if (display)
  {
    for (unsigned i = 0; i < 2; ++i)
    {
      CheckEmbedding(display, reinterpret_cast<uintptr_t>(window()), container(), i == 1);
      SetWindowPos(container(), nullptr, i ? 25 : 0, i ? 35 : 0, 650, 620,
                   SWP_NOZORDER | SWP_NOACTIVATE);
      Pump(200);
      CheckEmbedding(display, reinterpret_cast<uintptr_t>(window()), container(), i == 1);
      XEvent event {};
      event.xbutton.type = ButtonPress;
      event.xbutton.display = display;
      event.xbutton.window = reinterpret_cast<uintptr_t>(window());
      event.xbutton.button = Button1;
      event.xbutton.x = 200;
      event.xbutton.y = 200;
      event.xbutton.time = 1000 + i * 1000;
      Check(event.xbutton.window != 0, "editor exists before clicking");
      if (!event.xbutton.window)
        break;
      XSendEvent(display, event.xbutton.window, 0, ButtonPressMask, &event);
      XSync(display, 0);
      Pump(200);
      Check(count(1) == 2 && count(2) == i + 1, "deferred dock/hide completes once after dispatch");
    }
    show();
    Pump(200);
    CheckEmbedding(display, reinterpret_cast<uintptr_t>(window()), container(), true);
    Check(count(1) == 3, "docked editor reopens after hiding");
    DestroyWindow(container());
    Pump(100);
    Check(container() == nullptr && window() == nullptr, "container destruction also closes the editor");
    Check(count(2) == 3, "container destruction closes the editor exactly once");
    XCloseDisplay(display);
  }
  Check(sDockAdds == 2, "dock callbacks delivered on main thread, including reopening");
  const auto idle = count(0);
  Pump(100);
  Check(count(0) > idle, "idle continues with editor closed");
  Check(count(3) == 0, "draw, input, idle and close retain main-thread/lifetime invariants");
  stop();
  DestroyWindow(sDocker);
  const auto stopped = count(0);
  Pump(100);
  Check(count(0) == stopped, "timers stop before extension destruction");
  dlclose(module);
  printf("Linux REAPER main-loop checks: %s\n", sErrors ? "FAILED" : "passed");
  return sErrors ? 1 : 0;
}
