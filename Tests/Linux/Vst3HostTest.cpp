#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#include <limits>
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include <atomic>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <poll.h>
#include <thread>
#include <vector>
#include "X11Test.h"

using namespace Steinberg;
using namespace Steinberg::Vst;
using Clock = std::chrono::steady_clock;
static std::atomic<unsigned> sFailures {0};

static void Check(bool condition, const char* message)
{
  if (!condition)
  {
    fprintf(stderr, "FAIL: %s\n", message);
    ++sFailures;
  }
}

#include "Vst3AudioTest.h"

class Host final : public IPlugFrame, public Linux::IRunLoop, public IComponentHandler
{
public:
  tresult PLUGIN_API queryInterface(const TUID id, void** object) override
  {
    QUERY_INTERFACE(id, object, FUnknown::iid, IPlugFrame)
    QUERY_INTERFACE(id, object, IPlugFrame::iid, IPlugFrame)
    QUERY_INTERFACE(id, object, IComponentHandler::iid, IComponentHandler)
    if (offerLoop)
    {
      QUERY_INTERFACE(id, object, Linux::IRunLoop::iid, Linux::IRunLoop)
    }
    *object = nullptr;
    return kNoInterface;
  }
  uint32 PLUGIN_API addRef() override { return ++references; }
  uint32 PLUGIN_API release() override { return --references; }
  tresult PLUGIN_API resizeView(IPlugView* view, ViewRect* size) override
  {
    CheckMainThread();
    return view->onSize(size);
  }
  tresult PLUGIN_API registerTimer(Linux::ITimerHandler* handler, Linux::TimerInterval interval) override
  {
    CheckMainThread();
    Check(interval > 0 && std::find(timers.begin(), timers.end(), handler) == timers.end(),
          "valid distinct VST3 timer registration");
    if (rejectTimer)
      return kResultFalse;
    timers.push_back(handler);
    handler->addRef();
    ++registeredTimers;
    return kResultOk;
  }
  tresult PLUGIN_API unregisterTimer(Linux::ITimerHandler* handler) override
  {
    CheckMainThread();
    const auto found = std::find(timers.begin(), timers.end(), handler);
    Check(found != timers.end(), "unregister an active VST3 timer");
    if (found == timers.end())
      return kResultFalse;
    timers.erase(found);
    handler->release();
    ++unregisteredTimers;
    return kResultOk;
  }
  tresult PLUGIN_API registerEventHandler(Linux::IEventHandler* handler, int descriptor) override
  {
    CheckMainThread();
    Check(descriptor >= 0 && !events, "valid single VST3 fd registration");
    if (rejectFd)
      return kResultFalse;
    events = handler;
    events->addRef();
    fd = descriptor;
    ++registeredFds;
    return kResultOk;
  }
  tresult PLUGIN_API unregisterEventHandler(Linux::IEventHandler* handler) override
  {
    CheckMainThread();
    Check(events == handler, "unregister the active VST3 fd");
    events = nullptr;
    fd = -1;
    handler->release();
    ++unregisteredFds;
    return kResultOk;
  }
  tresult PLUGIN_API beginEdit(ParamID) override
  {
    CheckMainThread();
    ++begins;
    return kResultOk;
  }
  tresult PLUGIN_API performEdit(ParamID, ParamValue) override
  {
    CheckMainThread();
    ++edits;
    return kResultOk;
  }
  tresult PLUGIN_API endEdit(ParamID) override
  {
    CheckMainThread();
    ++ends;
    return kResultOk;
  }
  tresult PLUGIN_API restartComponent(int32) override
  {
    CheckMainThread();
    return kResultOk;
  }
  void CheckMainThread()
  {
    Check(std::this_thread::get_id() == mainThread, "VST3 host callback runs on the main thread");
  }
  void Pump()
  {
    if (events)
    {
      pollfd descriptor {fd, POLLIN, 0};
      if (poll(&descriptor, 1, 0) > 0 && (descriptor.revents & POLLIN))
        events->onFDIsSet(fd);
    }
    const auto snapshot = timers;
    for (auto* timer : snapshot)
      timer->addRef();
    for (auto* timer : snapshot)
    {
      if (std::find(timers.begin(), timers.end(), timer) != timers.end())
        timer->onTimer();
      timer->release();
    }
  }
  void CheckStopped(unsigned idleTimers = 0)
  {
    Check(timers.size() == idleTimers && !events, "only lifetime idle timers remain registered");
    Check(registeredTimers == unregisteredTimers + idleTimers, "all GUI VST3 timers released");
    Check(registeredFds == unregisteredFds, "all VST3 descriptors released");
  }

  const std::thread::id mainThread = std::this_thread::get_id();
  std::atomic<uint32> references {1};
  std::atomic<unsigned> begins {0}, edits {0}, ends {0};
  bool offerLoop = true;
  bool rejectTimer = false;
  bool rejectFd = false;
  std::vector<Linux::ITimerHandler*> timers;
  Linux::IEventHandler* events = nullptr;
  int fd = -1;
  unsigned registeredTimers = 0, unregisteredTimers = 0;
  unsigned registeredFds = 0, unregisteredFds = 0;
};

static void SendGesture(Display* display, Window editor)
{
  XEvent event {};
  event.xbutton.display = display;
  event.xbutton.window = editor;
  event.xbutton.root = DefaultRootWindow(display);
  event.xbutton.time = 1000;
  event.xbutton.x = 300;
  event.xbutton.y = 200;
  event.xbutton.button = Button1;
  event.xbutton.same_screen = True;
  event.xbutton.type = ButtonPress;
  XSendEvent(display, editor, False, ButtonPressMask, &event);
  event.xmotion.type = MotionNotify;
  event.xmotion.state = Button1Mask;
  event.xmotion.y = 180;
  XSendEvent(display, editor, False, PointerMotionMask, &event);
  event.xbutton.type = ButtonRelease;
  event.xbutton.button = Button1;
  event.xbutton.state = Button1Mask;
  XSendEvent(display, editor, False, ButtonReleaseMask, &event);
  XSync(display, False);
}

int main(int argc, char** argv)
{
  if (argc != 3)
    return 1;
  const char* mode = argv[2];
  const bool closeProbe = strncmp(mode, "close-order", 11) == 0;
  const bool idleProbe = strncmp(mode, "idle", 4) == 0 || closeProbe;
  Host host, frame;
  host.offerLoop = strcmp(mode, "no-loop") != 0 && strcmp(mode, "idle-frame") != 0 &&
    strcmp(mode, "idle-missing") != 0;
  frame.offerLoop = strcmp(mode, "no-loop") != 0 && strcmp(mode, "context-loop") != 0 &&
    strcmp(mode, "idle-missing") != 0;
  frame.rejectTimer = strcmp(mode, "timer-fail") == 0 || strcmp(mode, "idle-reject") == 0;
  host.rejectTimer = strcmp(mode, "idle-reject") == 0;
  frame.rejectFd = strcmp(mode, "fd-fail") == 0 || strcmp(mode, "close-order-poll") == 0;
  void* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
  if (!library)
  {
    fprintf(stderr, "Could not load VST3 module: %s\n", dlerror());
    return 1;
  }
  const auto entry = reinterpret_cast<bool (*)(void*)>(dlsym(library, "ModuleEntry"));
  const auto exit = reinterpret_cast<bool (*)()>(dlsym(library, "ModuleExit"));
  const auto getFactory = reinterpret_cast<IPluginFactory* (*)()>(dlsym(library, "GetPluginFactory"));
  const auto idleCount = reinterpret_cast<unsigned (*)(unsigned)>(dlsym(library, "LinuxIdleProbeCount"));
  const auto webProbe = reinterpret_cast<unsigned (*)(int)>(dlsym(library, "LinuxWebEditorProbe"));
  if (idleProbe && !idleCount)
  {
    fprintf(stderr, "Missing idle probe counters\n");
    return 1;
  }
  if (!entry || !exit || !getFactory || !entry(library))
  {
    fprintf(stderr, "Missing or failed VST3 entry point\n");
    dlclose(library);
    return 1;
  }
  IPluginFactory* factory = getFactory();
  PClassInfo info {};
  IComponent* component = nullptr;
  if (!factory || factory->getClassInfo(0, &info) != kResultOk ||
      factory->createInstance(info.cid, IComponent::iid, reinterpret_cast<void**>(&component)) != kResultOk ||
      component->initialize(static_cast<IPlugFrame*>(&host)) != kResultOk)
  {
    fprintf(stderr, "Could not initialize VST3 component\n");
    return 1;
  }
  if (strcmp(mode, "audio-state") == 0)
  {
    TestAudio(component);
    component->terminate();
    component->release();
    host.CheckStopped();
    factory->release();
    exit();
    dlclose(library);
    return sFailures ? 1 : 0;
  }
  if (strcmp(mode, "idle-headless") == 0)
  {
    FUnknownPtr<IEditController> controller(component);
    Check(controller && !controller->createView("editor"), "headless VST3 offers no editor");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    Check(idleCount(0) == 0, "headless VST3 idle waits for host dispatch");
    host.Pump();
    Check(idleCount(0) > 0 && idleCount(1) == 0, "headless VST3 idle runs on the main thread");
    controller = nullptr;
    component->terminate();
    component->release();
    host.CheckStopped();
    Check(host.references == 1, "headless VST3 releases the host run loop");
    factory->release();
    exit();
    dlclose(library);
    return sFailures ? 1 : 0;
  }
  {
    FUnknownPtr<IEditController> controller(component);
    Check(controller != nullptr, "single-component VST3 controller");
    if (!controller)
      return 1;
    Check(controller->setComponentHandler(&host) == kResultOk, "set VST3 component handler");
    const unsigned hostIdleTimers = host.offerLoop && !host.rejectTimer ? 1 : 0;
    if (idleProbe)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      Check(idleCount(0) == 0, "idle never runs on a private worker before host dispatch");
      host.Pump();
      Check((idleCount(0) > 0) == (hostIdleTimers > 0), "idle before any editor uses the context loop");
    }
    XInitThreads();
    Display* display = XOpenDisplay(nullptr);
    if (!display)
      return 1;
    Window parent = XCreateSimpleWindow(display, DefaultRootWindow(display),
                                         0, 0, 600, 600, 0, 0, 0);
    XMapWindow(display, parent);
    XSync(display, False);
    const bool windowFailure = strcmp(mode, "window-fail") == 0;
    if (windowFailure)
      Check(setenv("IPLUG_LINUX_TEST_MODE", "window-fail", 1) == 0, "inject VST3 window failure");
    const bool expectFailure = (!host.offerLoop && !frame.offerLoop) || frame.rejectTimer || windowFailure;
    for (unsigned cycle = 0; cycle < 3; ++cycle)
    {
      IPlugView* view = controller->createView("editor");
      Check(view != nullptr, "create VST3 view");
      if (!view)
        break;
      if (strcmp(mode, "scale") == 0)
      {
        FUnknownPtr<IPlugViewContentScaleSupport> scale(view);
        ViewRect size;
        Check(scale && scale->setContentScaleFactor(1.25f) == kResultOk,
              "accept scale before frame and attachment");
        Check(view->getSize(&size) == kResultOk && size.getWidth() == 750 && size.getHeight() == 750,
              "pre-frame getSize reflects fractional scale");
        Check(scale->setContentScaleFactor(0.f) != kResultOk &&
              scale->setContentScaleFactor(std::numeric_limits<float>::quiet_NaN()) != kResultOk,
              "reject invalid VST3 content scales");
        Check(setenv("GDK_SCALE", "3", 1) == 0, "test VST3 host scale priority");
        XResizeWindow(display, parent, 1200, 1200);
        XSync(display, False);
      }
      Check(view->setFrame(&frame) == kResultOk, "set VST3 frame");
      const auto result = view->attached(reinterpret_cast<void*>(static_cast<uintptr_t>(parent)),
                                         kPlatformTypeX11EmbedWindowID);
      Check((result == kResultOk) == !expectFailure, "VST3 attach reports actual success or failure");
      if (result == kResultOk)
      {
        if (strcmp(mode, "webview") == 0)
        {
          Check(webProbe != nullptr, "VST3 WebView exports test counters");
          const auto deadline = Clock::now() + std::chrono::seconds(10);
          while (webProbe && Clock::now() < deadline &&
                 (webProbe(0) <= cycle || webProbe(1) <= cycle || webProbe(2) <= cycle))
          {
            frame.Pump();
            host.Pump();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
          }
          Check(webProbe && webProbe(0) > cycle && webProbe(1) > cycle && webProbe(2) > cycle,
                "VST3 WebView loads page and exchanges JS messages");
          Window root = 0, owner = 0, *children = nullptr;
          unsigned count = 0;
          const bool queried = XQueryTree(display, parent, &root, &owner, &children, &count);
          Check(queried && count > 0, "VST3 WebView embedded under host X11 parent");
          if (children) XFree(children);
          ViewRect resized(0, 0, 410 + static_cast<int>(cycle) * 10, 310);
          Check(view->onSize(&resized) == kResultTrue, "resize VST3 WebView");
          XResizeWindow(display, parent, resized.getWidth(), resized.getHeight());
          const auto resizeDeadline = Clock::now() + std::chrono::seconds(2);
          bool sizeMatched = false;
          while (Clock::now() < resizeDeadline && !sizeMatched)
          {
            frame.Pump();
            host.Pump();
            XSync(display, False);
            children = nullptr;
            count = 0;
            if (XQueryTree(display, parent, &root, &owner, &children, &count) && count)
            {
              XWindowAttributes size {};
              sizeMatched = XGetWindowAttributes(display, children[0], &size) &&
                            size.width == resized.getWidth() && size.height == resized.getHeight();
            }
            if (children) XFree(children);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
          }
          Check(sizeMatched, "VST3 WebView X11 plug follows host resize");
        }
        else
        {
        if (strcmp(mode, "scale") == 0)
        {
          FUnknownPtr<IPlugViewContentScaleSupport> scale(view);
          Check(scale->setContentScaleFactor(2.f) == kResultOk, "accept attached scale change");
          ViewRect size;
          Check(view->getSize(&size) == kResultOk && size.getWidth() == 1200 && size.getHeight() == 1200,
                "attached size reflects scale change for a fixed-size editor");
          frame.Pump();
          host.Pump();
          const Window editor = FindEditor(display, parent);
          XWindowAttributes attributes {};
          Check(editor && XGetWindowAttributes(display, editor, &attributes) &&
                attributes.width == 1200 && attributes.height == 1200, "VST3 native size matches scale");
          unsetenv("GDK_SCALE");
        }
        if (strcmp(mode, "frame-switch") == 0)
        {
          Check(view->setFrame(&host) == kResultOk, "replace frame while editor is attached");
          frame.CheckStopped();
        }
        bool rendered = false;
        Window editor = 0;
        const auto deadline = Clock::now() + std::chrono::seconds(3);
        while (!rendered && Clock::now() < deadline)
        {
          frame.Pump();
          host.Pump();
          editor = FindEditor(display, parent);
          rendered = editor && IsRendered(display, editor);
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        Check(rendered, "VST3 editor renders through the host run loop");
        if (editor && !idleProbe && strcmp(mode, "scale") != 0)
        {
          const unsigned before = host.begins;
          SendGesture(display, editor);
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          Check(host.begins == before, "VST3 input waits for the host UI event loop");
          frame.Pump();
          host.Pump();
          Check(host.begins > before && host.edits > 0 && host.ends > 0,
                "real knob gesture reaches beginEdit, performEdit, and endEdit");
        }
        if (editor && (strcmp(mode, "native-close") == 0 || closeProbe))
        {
          const unsigned closes = closeProbe ? idleCount(4) : 0;
          SendNativeClose(display, editor);
          if (closeProbe)
            Check(idleCount(4) == closes, "native close waits for owner main-thread dispatch");
          frame.Pump();
          host.Pump();
          frame.CheckStopped();
          host.CheckStopped(hostIdleTimers);
          if (closeProbe)
          {
            Check(idleCount(4) == closes + 1, "VST3 native close calls OnUIClose exactly once");
            Check(idleCount(5) == 0, "VST3 OnUIClose runs on main with live controls");
          }
        }
        }
      }
      view->removed();
      const unsigned frameIdleTimers = idleProbe && strcmp(mode, "idle-frame") == 0 ? 1 : 0;
      host.CheckStopped(hostIdleTimers);
      frame.CheckStopped(frameIdleTimers);
      if (idleProbe)
      {
        Check(idleCount(3) == idleCount(4) && idleCount(5) == 0,
              "VST3 host removal preserves paired UI callbacks and live controls");
        const unsigned before = idleCount(0);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        Check(idleCount(0) == before, "hidden editor idle waits for main-thread dispatch");
        host.Pump();
        frame.Pump();
        Check((idleCount(0) > before) == !expectFailure, "idle continues after editor removal");
        Check(idleCount(1) == 0, "all VST3 idle callbacks run on the main thread");
      }
      view->setFrame(nullptr);
      view->release();
    }
    if (windowFailure)
      unsetenv("IPLUG_LINUX_TEST_MODE");
    controller->setComponentHandler(nullptr);
    XDestroyWindow(display, parent);
    XCloseDisplay(display);
  }
  component->terminate();
  host.CheckStopped();
  frame.CheckStopped();
  component->release();
  Check(host.references == 1 && frame.references == 1, "VST3 teardown releases host run loops");
  factory->release();
  exit();
  dlclose(library);
  printf("VST3 %s: %s\n", mode, sFailures ? "FAILED" : "passed");
  return sFailures ? 1 : 0;
}
