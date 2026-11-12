#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/Xresource.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <climits>
#include <clocale>
#include <cstdio>
#include <cstring>
#include <future>
#include <string>
#include <thread>
#include <sys/wait.h>
#include <unistd.h>
#include <dlfcn.h>
#include <glad/glad.h>
#include <GL/glx.h>

#include "IGraphicsLinux.h"
#include "IControls.h"
#include "IPlugTimer.h"

using namespace iplug;
using namespace iplug::igraphics;
using Clock = std::chrono::steady_clock;

static int sFailures = 0;
static bool sFailGLXActivation = false;
static bool sDisableInputMethod = false;
static unsigned sInputMethodAttempts = 0;
static unsigned sInputContextsDestroyed = 0;
static bool sWatchErrorHandler = false;
static unsigned sErrorHandlerChanges = 0;
static const char* sInputCommit = nullptr;
static int sInputCommitStatus = XLookupChars;
static unsigned sInputLookups = 0;
static bool sSynchronousResize = false;

extern "C" int XResizeWindow(Display* display, Window window, unsigned width, unsigned height)
{
  static auto resize = reinterpret_cast<decltype(&XResizeWindow)>(dlsym(RTLD_NEXT, "XResizeWindow"));
  if (!resize)
    abort();
  const int result = resize(display, window, width, height);
  // Keep the two independent test connections in order, so a host resize cannot
  // overtake an older editor request that is still buffered by Xlib.
  if (sSynchronousResize)
    XSync(display, 0);
  return result;
}

extern "C" XIM XOpenIM(Display* display, XrmDatabase database, char* name, char* className)
{
  ++sInputMethodAttempts;
  if (sDisableInputMethod)
    return nullptr;
  static auto open = reinterpret_cast<decltype(&XOpenIM)>(dlsym(RTLD_NEXT, "XOpenIM"));
  if (!open)
    abort();
  return open(display, database, name, className);
}

extern "C" void XDestroyIC(XIC context)
{
  ++sInputContextsDestroyed;
  static auto destroy = reinterpret_cast<decltype(&XDestroyIC)>(dlsym(RTLD_NEXT, "XDestroyIC"));
  if (!destroy)
    abort();
  destroy(context);
}

extern "C" int Xutf8LookupString(XIC context, XKeyPressedEvent* event, char* buffer,
                                 int capacity, KeySym* symbol, int* status)
{
  ++sInputLookups;
  if (sInputCommit)
  {
    const int length = static_cast<int>(strlen(sInputCommit));
    *symbol = sInputCommitStatus == XLookupChars ? XK_a : XK_Return;
    *status = length > capacity ? XBufferOverflow : sInputCommitStatus;
    if (length <= capacity)
      memcpy(buffer, sInputCommit, length);
    return length;
  }
  static auto lookup = reinterpret_cast<decltype(&Xutf8LookupString)>(
    dlsym(RTLD_NEXT, "Xutf8LookupString"));
  if (!lookup)
    abort();
  return lookup(context, event, buffer, capacity, symbol, status);
}

extern "C" XErrorHandler XSetErrorHandler(XErrorHandler handler)
{
  if (sWatchErrorHandler)
    ++sErrorHandlerChanges;
  static auto set = reinterpret_cast<decltype(&XSetErrorHandler)>(
    dlsym(RTLD_NEXT, "XSetErrorHandler"));
  if (!set)
    abort();
  return set(handler);
}

extern "C" int glXMakeCurrent(Display* display, GLXDrawable drawable, GLXContext context)
{
  if (sFailGLXActivation)
  {
    sFailGLXActivation = false;
    return 0;
  }
  static auto makeCurrent = reinterpret_cast<int (*)(Display*, GLXDrawable, GLXContext)>(
    dlsym(RTLD_NEXT, "glXMakeCurrent"));
  if (!makeCurrent)
  {
    fprintf(stderr, "Could not resolve glXMakeCurrent\n");
    abort();
  }
  return makeCurrent(display, drawable, context);
}

static void Check(bool condition, const char* message)
{
  if (!condition)
  {
    fprintf(stderr, "FAIL: %s\n", message);
    ++sFailures;
  }
}

class TestDelegate final : public IGEditorDelegate
{
public:
  explicit TestDelegate(bool hostDriven) : IGEditorDelegate(0)
  {
    mMakeGraphicsFunc = [this, hostDriven]() {
      auto* graphics = new IGraphicsLinux(*this, 320, 240, 60, 1.f);
      graphics->SetHostDriven(hostDriven);
      return graphics;
    };
    mLayoutFunc = [](IGraphics* graphics) {
      graphics->AttachPanelBackground(COLOR_GRAY);
    };
  }

  void BeginInformHostOfParamChangeFromUI(int) override
  {
    Check(false, "unexpected parameter gesture");
  }
  void EndInformHostOfParamChangeFromUI(int) override
  {
    Check(false, "unexpected parameter gesture");
  }
  bool EditorResizeFromUI(int width, int height, bool) override
  {
    SetEditorSize(width, height);
    return false; // The test has no parent resize callback.
  }
  void OnUIClose() override
  {
    Check(GetUI() != nullptr, "graphics remain available during OnUIClose");
    Check(GetUI() && GetUI()->GetBackgroundControl(), "controls remain alive during OnUIClose");
    Check(std::this_thread::get_id() == mMainThread, "OnUIClose runs on the owning main thread");
    if (mReentrantClose)
      CloseWindow();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ++mCloses;
  }

  unsigned mCloses = 0;
  bool mReentrantClose = false;

private:
  const std::thread::id mMainThread = std::this_thread::get_id();
};

struct GLXState
{
  Display* display = glXGetCurrentDisplay();
  GLXContext context = glXGetCurrentContext();
  GLXDrawable draw = glXGetCurrentDrawable();
  GLXDrawable read = glXGetCurrentReadDrawable();

  void CheckCurrent(const char* message) const
  {
    Check(display == glXGetCurrentDisplay() && context == glXGetCurrentContext() &&
          draw == glXGetCurrentDrawable() && read == glXGetCurrentReadDrawable(), message);
  }
};

static void TestGLXContext()
{
  XInitThreads();
  Display* display = XOpenDisplay(nullptr);
  Check(display != nullptr, "open separate host GLX display");
  if (!display)
    return;
  const int attributes[] = {GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
                             GLX_RENDER_TYPE, GLX_RGBA_BIT, 0};
  int count = 0;
  GLXFBConfig* configs = glXChooseFBConfig(display, DefaultScreen(display), attributes, &count);
  Check(configs && count > 0, "choose host GLX framebuffer configuration");
  if (!configs || count == 0)
  {
    if (configs) XFree(configs);
    XCloseDisplay(display);
    return;
  }
  GLXContext context = glXCreateNewContext(display, configs[0], GLX_RGBA_TYPE, nullptr, 1);
  const int size[] = {GLX_PBUFFER_WIDTH, 16, GLX_PBUFFER_HEIGHT, 16, 0};
  const GLXPbuffer draw = glXCreatePbuffer(display, configs[0], size);
  const GLXPbuffer read = glXCreatePbuffer(display, configs[0], size);
  XFree(configs);
  Check(context && draw && read && draw != read, "create host context with distinct draw/read buffers");
  if (context && draw && read)
  {
    for (bool hostCurrent : {false, true})
    {
      Check(glXMakeContextCurrent(display, hostCurrent ? draw : 0, hostCurrent ? read : 0,
                                  hostCurrent ? context : nullptr), "set initial host GLX state");
      const GLXState host;
      TestDelegate first(true), second(true);
      Check(first.OpenWindow(nullptr) != nullptr, "open first editor with host GLX state");
      host.CheckCurrent("editor initialization restores host GLX state");
      Check(second.OpenWindow(nullptr) != nullptr, "open second editor with host GLX state");
      host.CheckCurrent("second editor initialization restores host GLX state");
      if (first.GetUI() && second.GetUI())
      {
        auto& graphics = *static_cast<IGraphicsLinux*>(first.GetUI());
        auto& other = *static_cast<IGraphicsLinux*>(second.GetUI());
        graphics.Resize(360, 260, 1.f);
        host.CheckCurrent("resize restores host GLX state");
        graphics.OnDisplayTimer();
        host.CheckCurrent("render restores host GLX state");
        {
          IGraphics::ScopedGLContext outer {&graphics};
          const GLXState firstState;
          Check(firstState.context && firstState.context != context, "activate first editor context");
          graphics.DrawResize();
          firstState.CheckCurrent("nested framebuffer resize preserves editor context");
          {
            IGraphics::ScopedGLContext middle {&other};
            const GLXState secondState;
            Check(secondState.context && secondState.context != firstState.context,
                  "activate different editor context");
            {
              IGraphics::ScopedGLContext inner {&graphics};
              firstState.CheckCurrent("reenter first editor context");
            }
            secondState.CheckCurrent("nested editor scope restores second editor");
          }
          firstState.CheckCurrent("second editor scope restores first editor");
        }
        host.CheckCurrent("nested GLX scopes restore host state");
        first.CloseWindow();
        host.CheckCurrent("editor close restores host GLX state");
        Check(first.OpenWindow(nullptr) != nullptr, "reopen first editor");
        host.CheckCurrent("editor reopening restores host GLX state");
      }
      second.CloseWindow();
      first.CloseWindow();
      host.CheckCurrent("all editor teardown preserves host GLX state");

      TestDelegate failed(true);
      sFailGLXActivation = true;
      Check(failed.OpenWindow(nullptr) == nullptr, "GLX activation failure rejects editor");
      Check(!sFailGLXActivation, "GLX activation failure hook was exercised");
      Check(!failed.GetUI(), "failed editor is cleaned up");
      host.CheckCurrent("failed editor initialization preserves host GLX state");
    }
  }
  glXMakeContextCurrent(display, 0, 0, nullptr);
  if (context) glXDestroyContext(display, context);
  if (draw) glXDestroyPbuffer(display, draw);
  if (read) glXDestroyPbuffer(display, read);
  XCloseDisplay(display);
}

static void TestClose()
{
  XInitThreads();
  Display* hostDisplay = XOpenDisplay(nullptr);
  if (!hostDisplay)
  {
    Check(false, "open host display for close regression");
    return;
  }
  const Window parent = XCreateSimpleWindow(hostDisplay, DefaultRootWindow(hostDisplay),
                                             0, 0, 320, 240, 0, 0, 0);
  XMapWindow(hostDisplay, parent);
  XSync(hostDisplay, 0);
  TestDelegate delegate(false);
  for (unsigned i = 0; i < 10; ++i)
  {
    if (!delegate.OpenWindow(reinterpret_cast<void*>(static_cast<uintptr_t>(parent))))
    {
      Check(false, "open editor for close regression");
      break;
    }
    delegate.CloseWindow();
    Check(delegate.GetUI() == nullptr, "editor destroyed after close");
    Check(delegate.mCloses == i + 1, "close callback called once per editor");
  }
  XDestroyWindow(hostDisplay, parent);
  XCloseDisplay(hostDisplay);
}

static void TestExternalClipboard(IGraphics& graphics, const char* text)
{
  std::atomic<bool> stop {false};
  std::promise<bool> ready;
  auto future = ready.get_future();
  std::thread owner([&]() {
    Display* display = XOpenDisplay(nullptr);
    if (!display)
    {
      ready.set_value(false);
      return;
    }
    const Window window = XCreateSimpleWindow(display, DefaultRootWindow(display),
                                               0, 0, 1, 1, 0, 0, 0);
    const Atom clipboard = XInternAtom(display, "CLIPBOARD", 0);
    const Atom utf8 = XInternAtom(display, "UTF8_STRING", 0);
    XSetSelectionOwner(display, clipboard, window, 0);
    ready.set_value(XGetSelectionOwner(display, clipboard) == window);
    while (!stop)
    {
      while (XPending(display))
      {
        XEvent event;
        XNextEvent(display, &event);
        if (event.type != SelectionRequest)
          continue;
        const auto& request = event.xselectionrequest;
        XEvent response {};
        response.xselection.type = SelectionNotify;
        response.xselection.requestor = request.requestor;
        response.xselection.selection = request.selection;
        response.xselection.target = request.target;
        response.xselection.time = request.time;
        if (request.target == utf8)
        {
          response.xselection.property = request.property ? request.property : request.target;
          XChangeProperty(display, request.requestor, response.xselection.property, utf8, 8,
                          PropModeReplace, reinterpret_cast<const unsigned char*>(text),
                          static_cast<int>(strlen(text)));
        }
        XSendEvent(display, request.requestor, 0, 0, &response);
        XFlush(display);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    XDestroyWindow(display, window);
    XCloseDisplay(display);
  });

  const bool ownsSelection = future.get();
  Check(ownsSelection, "external clipboard owner initialized");
  if (ownsSelection)
  {
    WDL_String result("old contents");
    Check(graphics.GetTextFromClipboard(result), "read external clipboard");
    Check(strcmp(result.Get(), text) == 0, "external clipboard text is complete");
  }
  stop = true;
  owner.join();
}

static void TestClipboard()
{
  TestDelegate delegate(true);
  if (!delegate.OpenWindow(nullptr))
  {
    Check(false, "open host-driven clipboard editor");
    return;
  }
  auto& graphics = *static_cast<IGraphicsLinux*>(delegate.GetUI());
  for (const char* text : {"clipboard text", "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e", ""})
  {
    Check(graphics.SetTextInClipboard(text), "own clipboard selection");
    WDL_String result("old contents");
    const auto start = Clock::now();
    Check(graphics.GetTextFromClipboard(result), "read self-owned clipboard");
    Check(Clock::now() - start < std::chrono::milliseconds(100),
          "self-owned clipboard does not wait for event dispatch");
    Check(strcmp(result.Get(), text) == 0, "self-owned clipboard preserves UTF-8 and empty text");
  }
  TestExternalClipboard(graphics, "external UTF-8 \xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e");
  TestExternalClipboard(graphics, "");
  Check(graphics.SetTextInClipboard("regained"), "regain clipboard ownership");
  graphics.ProcessX11Events();
  WDL_String result;
  Check(graphics.GetTextFromClipboard(result) && strcmp(result.Get(), "regained") == 0,
        "stale SelectionClear does not erase newly owned text");
  delegate.CloseWindow();
}

static void TestNativeClose(bool hostDriven)
{
  TestDelegate delegate(hostDriven);
  delegate.mReentrantClose = true;
  if (!delegate.OpenWindow(nullptr))
  {
    Check(false, "open native close editor");
    return;
  }
  auto* graphics = static_cast<IGraphicsLinux*>(delegate.GetUI());
  Display* sender = XOpenDisplay(nullptr);
  if (!sender)
  {
    Check(false, "open native close sender");
    return;
  }
  XEvent event {};
  event.xclient.type = ClientMessage;
  event.xclient.window = static_cast<Window>(reinterpret_cast<uintptr_t>(graphics->GetWindow()));
  event.xclient.message_type = XInternAtom(sender, "WM_PROTOCOLS", 0);
  event.xclient.format = 32;
  event.xclient.data.l[0] = XInternAtom(sender, "WM_DELETE_WINDOW", 0);
  XSendEvent(sender, event.xclient.window, 0, 0, &event);
  XSync(sender, 0);
  bool requested = false;
  const auto deadline = Clock::now() + std::chrono::seconds(2);
  while (!requested && Clock::now() < deadline)
  {
    if (hostDriven)
      graphics->OnDisplayTimer();
    {
      std::lock_guard<std::recursive_mutex> lock(delegate.GfxMutex());
      requested = graphics->WindowCloseRequested();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  Check(requested, "WM_DELETE_WINDOW records a deferred close request");
  {
    std::lock_guard<std::recursive_mutex> lock(delegate.GfxMutex());
    Check(delegate.mCloses == 0, "graphics dispatch does not run OnUIClose");
    Check(graphics->WindowIsOpen() && graphics->GetBackgroundControl(),
          "native window and controls survive until owner dispatch");
  }
  Check(delegate.CloseWindowIfRequested(), "owner completes native close outside graphics dispatch");
  Check(delegate.mCloses == 1, "native close invokes OnUIClose once");
  Check(delegate.GetUI() == nullptr, "owner joins the graphics worker and destroys the editor");
  delegate.CloseWindow();
  Check(delegate.mCloses == 1, "later host teardown does not repeat OnUIClose");
  Check(!delegate.CloseWindowIfRequested(), "native close request cannot be processed twice");
  if (delegate.OpenWindow(nullptr))
  {
    delegate.CloseWindow();
    Check(delegate.mCloses == 2, "reopening starts a fresh close lifecycle");
  }
  else
    Check(false, "reopen after deferred native close");
  XCloseDisplay(sender);
}

static void TestClipboardReceiver(const char* expected)
{
  TestDelegate receiver(true);
  if (!receiver.OpenWindow(nullptr))
  {
    Check(false, "open external clipboard receiver");
    return;
  }
  WDL_String text;
  Check(receiver.GetUI()->GetTextFromClipboard(text), "serve clipboard while owner UI thread waits");
  Check(strcmp(text.Get(), expected) == 0, "external receiver gets complete clipboard text");
}

static void TestClipboardInstances()
{
  TestDelegate owner(true), receiver(true);
  if (!owner.OpenWindow(nullptr) || !receiver.OpenWindow(nullptr))
  {
    Check(false, "open two host-driven clipboard editors");
    return;
  }
  for (const char* text : {"between instances", "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e", ""})
  {
    Check(owner.GetUI()->SetTextInClipboard(text), "set cross-instance clipboard");
    WDL_String result;
    Check(receiver.GetUI()->GetTextFromClipboard(result), "paste without pumping the host event loop");
    Check(strcmp(result.Get(), text) == 0, "cross-instance clipboard preserves UTF-8 and empty text");

    const pid_t child = fork();
    if (child == 0)
    {
      execl("/proc/self/exe", "LinuxRuntimeTest", "clipboard-receiver", text, nullptr);
      _exit(127);
    }
    Check(child > 0, "start independent clipboard client");
    if (child > 0)
    {
      int status = 0;
      Check(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0,
            "independent client can paste while the owner main thread is blocked");
    }
  }
  Check(owner.GetUI()->SetTextInClipboard("after close"), "set clipboard before closing owner editor");
  owner.CloseWindow();
  WDL_String text;
  Check(receiver.GetUI()->GetTextFromClipboard(text) && strcmp(text.Get(), "after close") == 0,
        "clipboard remains available while its module is loaded");
}

static void TestClipboardFailure()
{
  TestDelegate delegate(true);
  if (!delegate.OpenWindow(nullptr))
  {
    Check(false, "open clipboard failure editor");
    return;
  }
  for (const char* mode : {"clipboard-display-fail", "clipboard-connection-fail", "clipboard-thread-fail"})
  {
    Check(setenv("IPLUG_LINUX_TEST_MODE", mode, 1) == 0, "configure clipboard fault injection");
    Check(!delegate.GetUI()->SetTextInClipboard("not owned"), "clipboard startup failure is reported");
    Check(unsetenv("IPLUG_LINUX_TEST_MODE") == 0, "clear clipboard fault injection");
  }
  Check(delegate.GetUI()->SetTextInClipboard("retry"), "clipboard startup can retry after failures");
  WDL_String text;
  Check(delegate.GetUI()->GetTextFromClipboard(text) && strcmp(text.Get(), "retry") == 0,
        "retried clipboard service owns the expected text");
}

static std::atomic<unsigned> sHostXErrors {0};

static int HostXErrorHandler(Display*, XErrorEvent*)
{
  ++sHostXErrors;
  return 0;
}

class KeyProbe final : public IControl
{
public:
  KeyProbe() : IControl(IRECT(50, 40, 110, 90)) {}
  void Draw(IGraphics&) override {}
  bool OnKeyDown(float x, float y, const IKeyPress& key) override
  {
    mText += key.utf8;
    mKeys.push_back(key.VK);
    mX = x;
    mY = y;
    return true;
  }
  bool OnKeyUp(float, float, const IKeyPress& key) override
  {
    mReleasedText += key.utf8;
    ++mReleases;
    return true;
  }
  std::string mText;
  std::string mReleasedText;
  std::vector<int> mKeys;
  unsigned mReleases = 0;
  float mX = 0.f, mY = 0.f;
};

class DropProbe final : public IControl
{
public:
  DropProbe() : IControl(IRECT(50, 40, 110, 90)) { SetTooltip("Drop files here"); }
  void Draw(IGraphics&) override {}
  void OnDrop(const char* path) override { mPaths = {path}; }
  void OnDropMultiple(const std::vector<const char*>& paths) override
  {
    mPaths.clear();
    for (const char* path : paths) mPaths.emplace_back(path);
  }
  std::vector<std::string> mPaths;
};

static std::vector<Window> RootChildren(Display* display)
{
  Window root = 0, parent = 0, *children = nullptr;
  unsigned count = 0;
  std::vector<Window> result;
  XSync(display, 0);
  if (XQueryTree(display, DefaultRootWindow(display), &root, &parent, &children, &count) && count)
    result.assign(children, children + count);
  if (children) XFree(children);
  return result;
}

static void SendClient(Display* display, Window target, Atom type, long a, long b = 0,
                       long c = 0, long d = 0, long e = 0)
{
  XEvent event {};
  event.xclient.type = ClientMessage;
  event.xclient.display = display;
  event.xclient.window = target;
  event.xclient.message_type = type;
  event.xclient.format = 32;
  event.xclient.data.l[0] = a;
  event.xclient.data.l[1] = b;
  event.xclient.data.l[2] = c;
  event.xclient.data.l[3] = d;
  event.xclient.data.l[4] = e;
  XSendEvent(display, target, 0, NoEventMask, &event);
  XSync(display, 0);
}

static void TestDragAndTooltip(bool drag)
{
  TestDelegate delegate(true);
  Check(delegate.OpenWindow(nullptr) != nullptr, "open editor for X11 interaction test");
  if (!delegate.GetUI()) return;
  auto& graphics = *static_cast<IGraphicsLinux*>(delegate.GetUI());
  auto* probe = new DropProbe;
  graphics.AttachControl(probe);
  graphics.EnableMouseOver(true);
  graphics.EnableTooltips(true);
  Display* host = XOpenDisplay(nullptr);
  Check(host != nullptr, "open independent X11 source connection");
  if (!host) { delegate.CloseWindow(); return; }
  const Window editor = static_cast<Window>(reinterpret_cast<uintptr_t>(graphics.GetWindow()));
  const Window source = XCreateSimpleWindow(host, DefaultRootWindow(host), 0, 0, 1, 1, 0, 0, 0);
  XSelectInput(host, source, StructureNotifyMask | PropertyChangeMask);
  const auto atom = [host](const char* name) { return XInternAtom(host, name, 0); };
  if (drag)
  {
    const Atom selection = atom("XdndSelection"), uri = atom("text/uri-list");
    const Atom status = atom("XdndStatus"), finished = atom("XdndFinished");
    XSetSelectionOwner(host, selection, source, 0);
    XSync(host, 0);
    Check(XGetSelectionOwner(host, selection) == source, "own Xdnd selection");
    int rootX = 0, rootY = 0;
    Window child = 0;
    XTranslateCoordinates(host, editor, DefaultRootWindow(host), 70, 60, &rootX, &rootY, &child);
    for (const char* payload : {"file:///tmp/one%20file.wav\r\n",
                                "# comment\nfile://localhost/tmp/one%20file.wav\nfile:///tmp/two.wav\n"})
    {
      SendClient(host, editor, atom("XdndEnter"), source, 5L << 24, uri);
      SendClient(host, editor, atom("XdndPosition"), source, 0,
                 (rootX << 16) | (rootY & 0xffff), 0, atom("XdndActionCopy"));
      graphics.ProcessX11Events();
      XSync(host, 0);
      XEvent reply {};
      const bool gotStatus = XCheckTypedWindowEvent(host, source, ClientMessage, &reply);
      Check(gotStatus && reply.xclient.message_type == status && reply.xclient.data.l[1] == 1,
            "Xdnd target accepts URI list");
      SendClient(host, editor, atom("XdndDrop"), source, 0, 0);
      graphics.ProcessX11Events();
      XSync(host, 0);
      XEvent request {};
      const bool requested = XCheckTypedWindowEvent(host, source, SelectionRequest, &request);
      Check(requested && request.xselectionrequest.target == uri,
            "Xdnd requests URI-list selection data");
      if (!requested) break;
      XChangeProperty(host, request.xselectionrequest.requestor, request.xselectionrequest.property,
                      uri, 8, PropModeReplace, reinterpret_cast<const unsigned char*>(payload),
                      static_cast<int>(strlen(payload)));
      XEvent notify {};
      notify.xselection.type = SelectionNotify;
      notify.xselection.display = host;
      notify.xselection.requestor = request.xselectionrequest.requestor;
      notify.xselection.selection = selection;
      notify.xselection.target = uri;
      notify.xselection.property = request.xselectionrequest.property;
      notify.xselection.time = request.xselectionrequest.time;
      XSendEvent(host, notify.xselection.requestor, 0, NoEventMask, &notify);
      XSync(host, 0);
      graphics.ProcessX11Events();
      XSync(host, 0);
      Check(XCheckTypedWindowEvent(host, source, ClientMessage, &reply) &&
            reply.xclient.message_type == finished && reply.xclient.data.l[1] == 1,
            "Xdnd target reports a successful drop");
      const std::vector<std::string> expected = strstr(payload, "two.wav") ?
        std::vector<std::string>{"/tmp/one file.wav", "/tmp/two.wav"} :
        std::vector<std::string>{"/tmp/one file.wav"};
      Check(probe->mPaths == expected, "dropped paths reach the hovered control decoded and in order");
    }
  }
  else
  {
    const auto before = RootChildren(host);
    XEvent motion {};
    motion.xmotion.type = MotionNotify;
    motion.xmotion.display = host;
    motion.xmotion.window = editor;
    motion.xmotion.x = 70;
    motion.xmotion.y = 60;
    XSendEvent(host, editor, 0, NoEventMask, &motion);
    XSync(host, 0);
    graphics.ProcessX11Events();
    std::this_thread::sleep_for(std::chrono::milliseconds(750));
    graphics.OnDisplayTimer();
    const auto after = RootChildren(host);
    std::vector<Window> added;
    for (Window window : after)
      if (std::find(before.begin(), before.end(), window) == before.end()) added.push_back(window);
    Check(added.size() == 1, "hover creates one tooltip X11 window");
    if (!added.empty())
    {
      XWindowAttributes attrs {};
      Check(XGetWindowAttributes(host, added[0], &attrs) && attrs.map_state == IsViewable &&
            attrs.override_redirect, "tooltip window is mapped and override-redirect");
    }
    XEvent leave {};
    leave.xcrossing.type = LeaveNotify;
    leave.xcrossing.display = host;
    leave.xcrossing.window = editor;
    XSendEvent(host, editor, 0, NoEventMask, &leave);
    XSync(host, 0);
    graphics.ProcessX11Events();
    Check(RootChildren(host) == before, "leaving editor destroys tooltip window");
  }
  delegate.CloseWindow();
  XDestroyWindow(host, source);
  XCloseDisplay(host);
}

static void CheckText(const std::string& actual, const char* expected, const char* message)
{
  Check(actual == expected, message);
  if (actual != expected)
  {
    fprintf(stderr, "Actual UTF-8 bytes:");
    for (unsigned char byte : actual)
      fprintf(stderr, " %02x", byte);
    fprintf(stderr, "\n");
  }
}

static void TestFocus()
{
  TestDelegate delegate(true);
  Display* host = XOpenDisplay(nullptr);
  Check(host != nullptr, "open focus host connection");
  if (!host)
    return;
  const Window parent = XCreateSimpleWindow(host, DefaultRootWindow(host), 0, 0, 640, 480, 0, 0, 0);
  const Window sibling = XCreateSimpleWindow(host, parent, 400, 0, 100, 100, 0, 0, 0);
  XMapWindow(host, parent);
  XMapWindow(host, sibling);
  XSetInputFocus(host, sibling, RevertToParent, 0);
  XSync(host, 0);
  if (delegate.OpenWindow(reinterpret_cast<void*>(static_cast<uintptr_t>(parent))))
  {
    auto& graphics = *static_cast<IGraphicsLinux*>(delegate.GetUI());
    auto* probe = new KeyProbe();
    graphics.AttachControl(probe);
    const Window editor = reinterpret_cast<uintptr_t>(graphics.GetWindow());
    XSync(host, 0);
    Window focus = 0;
    int revert = 0;
    XGetInputFocus(host, &focus, &revert);
    Check(focus == sibling, "opening an embedded editor does not steal focus");
    XWarpPointer(host, 0, editor, 0, 0, 0, 0, 80, 64);
    XEvent event {};
    event.xbutton.type = ButtonPress;
    event.xbutton.display = host;
    event.xbutton.window = editor;
    event.xbutton.root = DefaultRootWindow(host);
    event.xbutton.button = Button1;
    event.xbutton.x = 80;
    event.xbutton.y = 64;
    event.xbutton.same_screen = 1;
    XSendEvent(host, editor, 0, ButtonPressMask, &event);
    event.xbutton.type = ButtonRelease;
    XSendEvent(host, editor, 0, ButtonReleaseMask, &event);
    XSync(host, 0);
    const auto focusDeadline = Clock::now() + std::chrono::seconds(2);
    do
    {
      graphics.ProcessX11Events();
      XGetInputFocus(host, &focus, &revert);
      if (focus == editor)
        break;
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    } while (Clock::now() < focusDeadline);
    Check(focus == editor, "click transfers keyboard focus from host sibling to editor");
    event = {};
    event.xkey.type = KeyPress;
    event.xkey.display = host;
    event.xkey.window = editor;
    event.xkey.root = DefaultRootWindow(host);
    event.xkey.keycode = XKeysymToKeycode(host, XK_a);
    event.xkey.x = 80;
    event.xkey.y = 64;
    event.xkey.same_screen = 1;
    // Destination 1 is X11 InputFocus: route through actual server focus.
    XSendEvent(host, 1, 0, KeyPressMask, &event);
    XSync(host, 0);
    graphics.ProcessX11Events();
    Check(probe->mText == "a", "keyboard input follows click focus into editor");
    XSetInputFocus(host, sibling, RevertToParent, 0);
    XSync(host, 0);
    graphics.ProcessX11Events();
    graphics.OnDisplayTimer();
    XGetInputFocus(host, &focus, &revert);
    Check(focus == sibling, "editor ticks preserve focus returned to host");
    delegate.CloseWindow();
  }
  else
    Check(false, "open editor for focus test");
  XDestroyWindow(host, parent);
  XCloseDisplay(host);
}

static void TestResize()
{
  struct SynchronousResize
  {
    SynchronousResize() { sSynchronousResize = true; }
    ~SynchronousResize() { sSynchronousResize = false; }
  } synchronousResize;
  TestDelegate delegate(true);
  if (!delegate.OpenWindow(nullptr))
  {
    Check(false, "open editor for host resizing");
    return;
  }
  auto& graphics = *static_cast<IGraphicsLinux*>(delegate.GetUI());
  Display* host = XOpenDisplay(nullptr);
  Check(host != nullptr, "open resizing host connection");
  if (!host)
    return;
  const Window window = reinterpret_cast<uintptr_t>(graphics.GetWindow());
  delegate.OnParentWindowResize(400, 300);
  Check(graphics.Width() == 400 && graphics.Height() == 300 && graphics.GetDrawScale() == 1.f,
        "editors without an explicit resize mode keep following the host layout");
  auto nativeResize = [&](int width, int height) {
    XResizeWindow(host, window, width, height);
    XSync(host, 0);
    graphics.ProcessX11Events();
  };
  for (float screenScale : {1.f, 2.f, 1.5f})
  {
    graphics.AttachCornerResizer(EUIResizerMode::Scale, false);
    graphics.Resize(320, 240, 1.f);
    graphics.SetScreenScale(screenScale);
    for (int iteration = 0; iteration < 3; ++iteration)
    {
      delegate.OnParentWindowResize(static_cast<int>(480 * screenScale),
                                    static_cast<int>(360 * screenScale));
      Check(graphics.Width() == 320 && graphics.Height() == 240 &&
            graphics.GetDrawScale() == 1.5f, "host resizing scales the fixed design bounds");
      graphics.ProcessX11Events();
      nativeResize(static_cast<int>(640 * screenScale), static_cast<int>(480 * screenScale));
      Check(graphics.Width() == 320 && graphics.Height() == 240 &&
            graphics.GetDrawScale() == 2.f, "X11 resizing shares the host's scale calculation without drift");
      nativeResize(static_cast<int>(240 * screenScale), static_cast<int>(180 * screenScale));
      Check(graphics.Width() == 320 && graphics.Height() == 240 &&
            graphics.GetDrawScale() == .75f, "X11 shrinking preserves design bounds");
    }
    graphics.SetScaleConstraints(.75f, 2.f);
    delegate.OnParentWindowResize(static_cast<int>(160 * screenScale), static_cast<int>(120 * screenScale));
    Check(graphics.GetDrawScale() == .75f, "host resize respects minimum drawing scale");
    delegate.OnParentWindowResize(static_cast<int>(960 * screenScale), static_cast<int>(720 * screenScale));
    Check(graphics.GetDrawScale() == 2.f, "host resize respects maximum drawing scale");
    delegate.SetSizeConstraints(240, 640, 180, 480);
    delegate.OnParentWindowResize(static_cast<int>(1000 * screenScale), static_cast<int>(750 * screenScale));
    Check(graphics.GetDrawScale() == 2.f, "host resize respects editor size constraints at HiDPI");
    delegate.SetSizeConstraints(10, 100000, 10, 100000);
    graphics.SetScaleConstraints(.5f, 4.f);

    graphics.AttachCornerResizer(EUIResizerMode::Size, false);
    graphics.Resize(320, 240, 1.25f);
    delegate.OnParentWindowResize(static_cast<int>(500 * screenScale), static_cast<int>(375 * screenScale));
    Check(graphics.Width() == 400 && graphics.Height() == 300 && graphics.GetDrawScale() == 1.25f,
          "Size mode changes logical bounds while preserving the drawing scale");
    graphics.ProcessX11Events();
    nativeResize(static_cast<int>(750 * screenScale), static_cast<int>(500 * screenScale));
    Check(graphics.Width() == 600 && graphics.Height() == 400 && graphics.GetDrawScale() == 1.25f,
          "X11 Size mode accounts for both screen and drawing scale");

    graphics.Resize(320, 240, 1.25f);
    delegate.SetSizeConstraints(200, 400, 160, 300);
    int proposedWidth = static_cast<int>(750 * screenScale);
    int proposedHeight = static_cast<int>(500 * screenScale);
    graphics.ConstrainHostResize(proposedWidth, proposedHeight);
    Check(proposedWidth == static_cast<int>(500 * screenScale) &&
          proposedHeight == static_cast<int>(375 * screenScale),
          "host Size constraints convert design maxima back to physical pixels");
    Check(graphics.Width() == 320 && graphics.Height() == 240,
          "querying host constraints does not resize the editor");
    delegate.OnParentWindowResize(proposedWidth, proposedHeight);
    Check(graphics.Width() == 400 && graphics.Height() == 300,
          "Size mode applies maximum constraints after removing drawing scale");
    graphics.ProcessX11Events();
    nativeResize(static_cast<int>(125 * screenScale), static_cast<int>(100 * screenScale));
    Check(graphics.Width() == 200 && graphics.Height() == 160 && graphics.GetDrawScale() == 1.25f,
          "native Size resizing respects design minima with zoom and HiDPI");
    nativeResize(static_cast<int>(500 * screenScale), static_cast<int>(375 * screenScale));
    Check(graphics.Width() == 400 && graphics.Height() == 300,
          "native Size resizing can reach the zoomed design maxima");

    graphics.AttachCornerResizer(EUIResizerMode::Scale, false);
    graphics.Resize(320, 240, 1.f);
    delegate.SetSizeConstraints(320, 320, 240, 240);
    proposedWidth = static_cast<int>(480 * screenScale);
    proposedHeight = static_cast<int>(360 * screenScale);
    graphics.ConstrainHostResize(proposedWidth, proposedHeight);
    Check(proposedWidth == static_cast<int>(480 * screenScale) &&
          proposedHeight == static_cast<int>(360 * screenScale),
          "fixed design constraints allow a scaled host window");
    delegate.OnParentWindowResize(proposedWidth, proposedHeight);
    Check(graphics.Width() == 320 && graphics.Height() == 240 && graphics.GetDrawScale() == 1.5f,
          "host Scale resizing preserves fixed design constraints");
    graphics.ProcessX11Events();
    nativeResize(static_cast<int>(640 * screenScale), static_cast<int>(480 * screenScale));
    Check(graphics.Width() == 320 && graphics.Height() == 240 && graphics.GetDrawScale() == 2.f,
          "native Scale resizing preserves fixed design constraints");
    graphics.SetScaleConstraints(.75f, 2.f);
    proposedWidth = static_cast<int>(960 * screenScale);
    proposedHeight = static_cast<int>(720 * screenScale);
    graphics.ConstrainHostResize(proposedWidth, proposedHeight);
    Check(proposedWidth == static_cast<int>(640 * screenScale) &&
          proposedHeight == static_cast<int>(480 * screenScale),
          "host size negotiation respects drawing-scale limits at HiDPI");
    delegate.SetSizeConstraints(10, 100000, 10, 100000);
    graphics.SetScaleConstraints(.5f, 4.f);
  }
  XCloseDisplay(host);
}

static void TestResizeRoundtrip()
{
  sSynchronousResize = true;
  TestDelegate delegate(true);
  if (!delegate.OpenWindow(nullptr))
  {
    Check(false, "open editor for resize round trips");
    return;
  }
  auto& graphics = *static_cast<IGraphicsLinux*>(delegate.GetUI());
  Display* host = XOpenDisplay(nullptr);
  Check(host != nullptr, "open host connection for resize round trips");
  if (!host)
    return;
  const Window window = reinterpret_cast<uintptr_t>(graphics.GetWindow());
  auto checkSize = [&](int width, int height) {
    Check(static_cast<int>(graphics.WindowWidth() * graphics.GetScreenScale()) == width &&
          static_cast<int>(graphics.WindowHeight() * graphics.GetScreenScale()) == height,
          "rendering honors the host-adjusted physical dimensions");
    Check(delegate.GetEditorWidth() == width && delegate.GetEditorHeight() == height,
          "the delegate reports the host-adjusted physical dimensions");
    XWindowAttributes attributes {};
    Check(XGetWindowAttributes(host, window, &attributes) &&
          attributes.width == width && attributes.height == height,
          "the X11 child matches the host-adjusted physical dimensions");
  };
  for (auto mode : {EUIResizerMode::Scale, EUIResizerMode::Size})
  for (auto design : {std::pair<int, int>{320, 240}, {317, 239}})
  for (float screenScale : {1.f, 1.25f, 1.5f, 2.f})
  for (float drawScale : {.625f, 1.f, 1.25f})
  {
    delegate.SetSizeConstraints(10, 640, 10, 480);
    graphics.SetScaleConstraints(.5f, 2.f);
    graphics.AttachCornerResizer(mode, false);
    graphics.Resize(design.first, design.second, drawScale);
    graphics.SetScreenScale(screenScale);
    if (mode == EUIResizerMode::Scale)
      delegate.SetSizeConstraints(design.first, design.first, design.second, design.second);
    else if (drawScale == 1.25f)
      delegate.SetSizeConstraints(10, INT_MAX, 10, INT_MAX);
    // Include non-binary scale limits as well as exact quarter/half scales.
    if (drawScale == 1.f)
      graphics.SetScaleConstraints(.73f, 1.61f);
    for (auto request : {std::pair<int, int>{483, 362}, {322, 241}, {511, 387},
                         {1200, 900}, {80, 60}, {713, 529}})
    {
      int width = request.first, height = request.second;
      graphics.ConstrainHostResize(width, height);
      for (int repeat = 0; repeat < 4; ++repeat)
      {
        int againWidth = width, againHeight = height;
        graphics.ConstrainHostResize(againWidth, againHeight);
        Check(width == againWidth && height == againHeight,
              "repeated host size negotiation does not change the adjusted size");
        delegate.OnParentWindowResize(width, height);
        graphics.ProcessX11Events();
        checkSize(width, height);
      }
      XResizeWindow(host, window, request.first, request.second);
      XSync(host, 0);
      graphics.ProcessX11Events();
      checkSize(width, height);
      graphics.ProcessX11Events();
      checkSize(width, height);
      if (mode == EUIResizerMode::Scale)
        Check(graphics.Width() == design.first && graphics.Height() == design.second,
              "round-trip resizing preserves the Scale-mode design dimensions");
    }
  }
  XCloseDisplay(host);
  sSynchronousResize = false;
}

class ClickProbe final : public IControl
{
public:
  explicit ClickProbe(const IRECT& bounds) : IControl(bounds) {}
  void Draw(IGraphics&) override {}
  void OnMouseDown(float, float, const IMouseMod&) override { ++mSingles; }
  void OnMouseDblClick(float, float, const IMouseMod&) override { ++mDoubles; }
  unsigned mSingles = 0;
  unsigned mDoubles = 0;
};

static void TestDoubleClick()
{
  XInitThreads();
  Display* host = XOpenDisplay(nullptr);
  Check(host != nullptr, "open clicking host connection");
  if (!host)
    return;
  for (const auto scales : {std::pair<float, float>{1.f, 1.f}, {2.f, 1.5f}, {1.f, .5f}})
  {
    TestDelegate delegate(true);
    if (!delegate.OpenWindow(nullptr))
    {
      Check(false, "open editor for double clicks");
      XCloseDisplay(host);
      return;
    }
    auto& graphics = *static_cast<IGraphicsLinux*>(delegate.GetUI());
    graphics.Resize(320, 240, scales.second);
    graphics.SetScreenScale(scales.first);
    auto* first = new ClickProbe(IRECT(20, 20, 100, 100));
    auto* second = new ClickProbe(IRECT(120, 20, 220, 100));
    graphics.AttachControl(first);
    graphics.AttachControl(second);
    const Window window = reinterpret_cast<uintptr_t>(graphics.GetWindow());
    auto click = [&](float x, float y, uint32_t time, unsigned button = Button1) {
      XEvent event {};
      event.xbutton.display = host;
      event.xbutton.window = window;
      event.xbutton.root = DefaultRootWindow(host);
      event.xbutton.time = time;
      event.xbutton.button = button;
      event.xbutton.x = static_cast<int>(x * graphics.GetTotalScale());
      event.xbutton.y = static_cast<int>(y * graphics.GetTotalScale());
      event.xbutton.same_screen = 1;
      for (int type : {ButtonPress, ButtonRelease})
      {
        event.type = type;
        XSendEvent(host, window, 0, type == ButtonPress ? ButtonPressMask : ButtonReleaseMask, &event);
      }
      XSync(host, 0);
      graphics.ProcessX11Events();
    };
    click(50, 50, 1000);
    click(150, 50, 1100);
    Check(first->mSingles == 1 && second->mSingles == 1 && second->mDoubles == 0,
          "quick clicks on distant controls remain single clicks");
    click(150, 50, 1200);
    Check(second->mDoubles == 1, "nearby consecutive clicks form a double click");
    click(150, 50, 1300);
    Check(second->mSingles == 2 && second->mDoubles == 1, "third click starts a new sequence");
    click(150, 50, 1800);
    Check(second->mSingles == 3, "click after timeout remains single");
    // A 6px movement on screen must exceed the tolerance even at low drawing scale.
    click(150 + 6.f / scales.second, 50, 1900);
    Check(second->mSingles == 4 && second->mDoubles == 1,
          "double-click distance is independent of drawing scale");
    click(150, 50, 2000, Button3);
    click(150, 50, 2100);
    Check(second->mSingles == 6 && second->mDoubles == 1, "another mouse button interrupts the left-click sequence");
    click(50, 50, UINT32_MAX - 50);
    click(50, 50, 49);
    Check(first->mDoubles == 1, "X11 timestamp wrap preserves double-click timing");
  }
  XCloseDisplay(host);
}

static void TestScaledKeys()
{
  TestDelegate delegate(true);
  if (!delegate.OpenWindow(nullptr))
  {
    Check(false, "open editor for scaled host keys");
    return;
  }
  auto& graphics = *static_cast<IGraphicsLinux*>(delegate.GetUI());
  auto* probe = new KeyProbe();
  graphics.AttachControl(probe);
  Display* host = XOpenDisplay(nullptr);
  Check(host != nullptr, "open pointer test connection");
  if (!host)
    return;
  for (const auto scales : {std::pair<float, float>{1.f, 1.f}, {2.f, 1.f},
                            {1.f, 1.5f}, {1.5f, 1.25f}})
  {
    graphics.Resize(320, 240, scales.second);
    graphics.SetScreenScale(scales.first);
    const float scale = graphics.GetTotalScale();
    const Window window = reinterpret_cast<uintptr_t>(graphics.GetWindow());
    XWarpPointer(host, 0, window, 0, 0, 0, 0,
                 static_cast<int>(80 * scale), static_cast<int>(64 * scale));
    XSync(host, 0);
    float x = 0.f, y = 0.f;
    graphics.GetMouseLocation(x, y);
    Check(std::fabs(x - 80.f) < 0.01f && std::fabs(y - 64.f) < 0.01f,
          "pointer coordinates include both screen and drawing scales");
    Check(delegate.OnKeyDown(IKeyPress("a", kVK_A)), "host key down reaches the hovered control");
    Check(delegate.OnKeyUp(IKeyPress("a", kVK_A)), "host key up reaches the hovered control");
    Check(std::fabs(probe->mX - 80.f) < 0.01f && std::fabs(probe->mY - 64.f) < 0.01f,
          "host key handler receives logical coordinates");
  }
  Check(probe->mText == "aaaa" && probe->mReleases == 4, "one key pair delivered at each scale");
  XCloseDisplay(host);
}

static void SendKey(Display* host, IGraphicsLinux& graphics, unsigned keycode, int type = KeyPress)
{
  XEvent event {};
  event.xkey.type = type;
  event.xkey.display = host;
  event.xkey.window = reinterpret_cast<uintptr_t>(graphics.GetWindow());
  event.xkey.root = DefaultRootWindow(host);
  event.xkey.keycode = keycode;
  event.xkey.same_screen = 1;
  Check(XSendEvent(host, event.xkey.window, 0,
                  type == KeyPress ? KeyPressMask : KeyReleaseMask, &event),
        "send keyboard event");
  XSync(host, 0);
  graphics.ProcessX11Events();
}

static void TestKeyboard(bool withoutIM, bool utf8Locale)
{
  Check(std::setlocale(LC_CTYPE, utf8Locale ? "C.UTF-8" : "C") != nullptr, "set the host locale");
  const std::string locale = std::setlocale(LC_CTYPE, nullptr);
  Check(XSetLocaleModifiers("@im=none") != nullptr, "use the local XIM Compose implementation");
  sDisableInputMethod = withoutIM;
  TestDelegate delegate(true);
  Display* host = XOpenDisplay(nullptr);
  Check(host != nullptr, "open keyboard test connection");
  if (!host)
    return;
  int minimum = 0, maximum = 0, symbolsPerKey = 0;
  XDisplayKeycodes(host, &minimum, &maximum);
  const int firstKey = maximum - 7;
  KeySym* previous = XGetKeyboardMapping(host, firstKey, 8, &symbolsPerKey);
  Check(previous != nullptr, "save keyboard mapping");
  if (!previous)
  {
    XCloseDisplay(host);
    return;
  }
  KeySym symbols[] = {XK_a, XK_eacute, XK_Multi_key, XK_apostrophe, XK_e,
                      0x01003042, XK_Return, XK_Left};
  XChangeKeyboardMapping(host, firstKey, 1, symbols, 8);
  XSync(host, 0);
  const unsigned destroyedBefore = sInputContextsDestroyed;
  for (unsigned iteration = 0; iteration < 2; ++iteration)
  {
    if (!delegate.OpenWindow(nullptr))
    {
      Check(false, "open keyboard editor");
      break;
    }
    Check(locale == std::setlocale(LC_CTYPE, nullptr), "opening an editor preserves the host locale");
    auto& graphics = *static_cast<IGraphicsLinux*>(delegate.GetUI());
    auto* probe = new KeyProbe();
    graphics.AttachControl(probe);
    const Window window = reinterpret_cast<uintptr_t>(graphics.GetWindow());
    XSetInputFocus(host, window, RevertToParent, 0);
    XEvent motion {};
    motion.xmotion.type = MotionNotify;
    motion.xmotion.display = host;
    motion.xmotion.window = window;
    motion.xmotion.x = 80;
    motion.xmotion.y = 64;
    motion.xmotion.same_screen = 1;
    Check(XSendEvent(host, window, 0, PointerMotionMask, &motion), "position keyboard target");
    XSync(host, 0);
    graphics.ProcessX11Events();
    SendKey(host, graphics, firstKey);
    SendKey(host, graphics, firstKey, KeyRelease);
    SendKey(host, graphics, firstKey + 1);
    SendKey(host, graphics, firstKey + 1, KeyRelease);
    SendKey(host, graphics, firstKey + 5);
    CheckText(probe->mText, "a\xc3\xa9\xe3\x81\x82", "ASCII, Latin-1 and Unicode keysyms produce UTF-8");
    CheckText(probe->mReleasedText, "a\xc3\xa9", "key releases also contain valid UTF-8");
    if (!withoutIM)
    {
      Check(sInputLookups > 0, "key presses use the input context");
      probe->mText.clear();
      SendKey(host, graphics, firstKey + 2);
      SendKey(host, graphics, firstKey + 3);
      Check(probe->mText.empty(), "Compose prefixes do not insert text");
      SendKey(host, graphics, firstKey + 4);
      Check(probe->mText == "\xc3\xa9", "Compose apostrophe plus e commits an accented character");

      std::string phrase;
      for (unsigned i = 0; i < 30; ++i)
        phrase += "\xe6\x97\xa5";
      phrase += "\xf0\x9f\x8e\xb5";
      probe->mText.clear();
      probe->mKeys.clear();
      const unsigned lookups = sInputLookups;
      sInputCommit = phrase.c_str();
      sInputCommitStatus = iteration ? XLookupBoth : XLookupChars;
      SendKey(host, graphics, firstKey);
      sInputCommit = nullptr;
      Check(sInputLookups == lookups + 2, "retry an IME commit after XBufferOverflow");
      Check(probe->mText == phrase && probe->mKeys.size() == 31,
            "deliver an entire Japanese phrase and a four-byte codepoint without truncation");
      for (int key : probe->mKeys)
        Check(key == kVK_NONE, "IME phrases do not carry unspecified or candidate-acceptance keys");
    }
    probe->mKeys.clear();
    SendKey(host, graphics, firstKey + 7);
    Check(probe->mKeys.size() == 1 && probe->mKeys[0] == kVK_LEFT,
          "non-text navigation keys retain virtual key codes");
    delegate.CloseWindow();
    Check(locale == std::setlocale(LC_CTYPE, nullptr), "closing an editor preserves the host locale");
    Check(sInputContextsDestroyed == destroyedBefore + (withoutIM ? 0 : iteration + 1),
          "close destroys each input context exactly once before reopening");
  }
  Check(sInputMethodAttempts == 2, "each reopened editor initializes its own input method");
  if (withoutIM)
    Check(sInputLookups == 0, "unavailable IM uses the explicit UTF-8 fallback");
  XChangeKeyboardMapping(host, firstKey, symbolsPerKey, previous, 8);
  XFree(previous);
  XCloseDisplay(host);
}

static void TestParentValidation()
{
  XInitThreads();
  Display* host = XOpenDisplay(nullptr);
  Check(host != nullptr, "open parent validation host");
  if (!host)
    return;
  const Window parent = XCreateSimpleWindow(host, DefaultRootWindow(host), 0, 0, 400, 300, 0, 0, 0);
  const Window destroyed = XCreateSimpleWindow(host, DefaultRootWindow(host), 0, 0, 1, 1, 0, 0, 0);
  XMapWindow(host, parent);
  XDestroyWindow(host, destroyed);
  XSync(host, 0);
  XErrorHandler previous = XSetErrorHandler(HostXErrorHandler);
  sWatchErrorHandler = true;
  for (const uintptr_t value : {static_cast<uintptr_t>(parent), static_cast<uintptr_t>(destroyed),
                                static_cast<uintptr_t>(UINT32_MAX) + 1})
  {
    TestDelegate delegate(true);
    void* editor = delegate.OpenWindow(reinterpret_cast<void*>(value));
    Check(editor != nullptr, "valid parent embeds and invalid parent safely falls back");
    if (editor)
    {
      Window root = 0, actualParent = 0, *children = nullptr;
      unsigned count = 0;
      Check(XQueryTree(host, reinterpret_cast<uintptr_t>(editor), &root, &actualParent, &children, &count),
            "query editor parent");
      Check(actualParent == (value == parent ? parent : DefaultRootWindow(host)),
            "only a valid 32-bit X11 parent is used");
      if (children)
        XFree(children);
    }
    XDestroyWindow(host, destroyed);
    XSync(host, 0);
    delegate.CloseWindow();
  }
  sWatchErrorHandler = false;
  Check(sErrorHandlerChanges == 0, "editor creation never changes the process-wide X11 error handler");
  Check(sHostXErrors == 3, "only unrelated host errors reach the host handler");
  Check(XSetErrorHandler(previous) == HostXErrorHandler, "parent validation preserves the host handler");
  XDestroyWindow(host, parent);
  XCloseDisplay(host);
}

static void TestDisappearingClipboardRequestor(bool hostHandler)
{
  TestDelegate owner(true);
  if (!owner.OpenWindow(nullptr))
  {
    Check(false, "open clipboard owner for cancelled transfers");
    return;
  }
  const char* text = "surviving clipboard \xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e";
  Check(owner.GetUI()->SetTextInClipboard(text), "own clipboard before cancelled transfers");
  Display* client = XOpenDisplay(nullptr);
  if (!client)
  {
    Check(false, "open clipboard requestor connection");
    owner.CloseWindow();
    return;
  }
  const Atom clipboard = XInternAtom(client, "CLIPBOARD", 0);
  const Atom utf8 = XInternAtom(client, "UTF8_STRING", 0);
  const Atom targets = XInternAtom(client, "TARGETS", 0);
  const Atom unsupported = XInternAtom(client, "_IPLUG_UNSUPPORTED_SELECTION", 0);
  const Atom property = XInternAtom(client, "_IPLUG_CLIPBOARD_PROBE", 0);
  XErrorHandler previousHandler = nullptr;
  if (hostHandler)
    previousHandler = XSetErrorHandler(HostXErrorHandler);

  Window destroyed = 0;
  for (Atom target : {targets, utf8, XA_STRING, unsupported})
  {
    for (unsigned i = 0; i < 8; ++i)
    {
      destroyed = XCreateSimpleWindow(client, DefaultRootWindow(client), 0, 0, 1, 1, 0, 0, 0);
      // The server cannot process the owner's reply until this requestor is already gone.
      XGrabServer(client);
      XConvertSelection(client, clipboard, target, property, destroyed, 0);
      XDestroyWindow(client, destroyed);
      XUngrabServer(client);
      XSync(client, 0);
    }
  }

  const Window requestor = XCreateSimpleWindow(client, DefaultRootWindow(client), 0, 0, 1, 1, 0, 0, 0);
  for (Atom target : {targets, utf8, XA_STRING, unsupported})
  {
    XConvertSelection(client, clipboard, target, property, requestor, 0);
    XFlush(client);
    XEvent response {};
    bool received = false;
    const auto deadline = Clock::now() + std::chrono::seconds(2);
    while (!received && Clock::now() < deadline)
    {
      received = XCheckTypedWindowEvent(client, requestor, SelectionNotify, &response);
      if (!received)
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    Check(received, "clipboard worker serves later requests after requestor cancellation");
    if (!received)
      continue;
    Check(response.xselection.target == target, "selection notification preserves requested target");
    if (target == unsupported)
    {
      Check(response.xselection.property == 0, "unsupported target reports conversion failure");
      continue;
    }
    Check(response.xselection.property == property, "successful reply names the result property");
    Atom type = 0;
    int format = 0;
    unsigned long count = 0, remaining = 0;
    unsigned char* data = nullptr;
    Check(XGetWindowProperty(client, requestor, property, 0, 1024, 1, AnyPropertyType,
                             &type, &format, &count, &remaining, &data) == 0,
          "read recovered clipboard reply");
    if (target == targets)
    {
      Check(type == XA_ATOM && format == 32 && count == 3 && remaining == 0,
            "TARGETS reply contains three correctly encoded 32-bit atoms");
      if (data && count == 3)
      {
        const auto* atoms = reinterpret_cast<const Atom*>(data);
        Check(atoms[0] == targets && atoms[1] == utf8 && atoms[2] == XA_STRING,
              "TARGETS reply preserves supported formats on 64-bit Xlib");
      }
    }
    else
      Check(data && type == target && format == 8 && count == strlen(text) && remaining == 0 &&
            memcmp(data, text, count) == 0, "recovered clipboard reply preserves complete UTF-8 text");
    if (data)
      XFree(data);
  }
  if (hostHandler)
  {
    Check(sHostXErrors == 0, "clipboard reply failures never reach the host error handler");
    XDestroyWindow(client, destroyed);
    XSync(client, 0);
    Check(sHostXErrors == 1, "unrelated host X11 errors still reach its own handler");
    Check(XSetErrorHandler(previousHandler) == HostXErrorHandler,
          "clipboard transfers preserve the installed host error handler");
  }
  XDestroyWindow(client, requestor);
  XCloseDisplay(client);
  owner.CloseWindow();
}

static void TestTimer(bool expectFailure)
{
  std::atomic<unsigned> calls {0};
  std::unique_ptr<Timer> timer(Timer::Create([&](Timer&) { ++calls; }, 5));
  if (expectFailure)
  {
    Check(!timer, "failed timer creation returns nullptr");
    Check(calls == 0, "failed timer never calls back");
    return;
  }
  Check(bool(timer), "create core timer");
  if (!timer)
    return;
  const auto deadline = Clock::now() + std::chrono::milliseconds(300);
  while (calls < 3 && Clock::now() < deadline)
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  Check(calls >= 3, "core timer runs even when the child is scheduled first");
  timer->Stop();
  timer->Stop();
  const unsigned stopped = calls;
  std::this_thread::sleep_for(std::chrono::milliseconds(30));
  Check(calls == stopped, "no callbacks after idempotent Stop");
}

int main(int argc, char** argv)
{
  if (argc < 2 || argc > 3)
    return 1;
  if (strcmp(argv[1], "close") == 0)
    TestClose();
  else if (strcmp(argv[1], "glx-context") == 0)
    TestGLXContext();
  else if (strcmp(argv[1], "scaled-keys") == 0)
    TestScaledKeys();
  else if (strcmp(argv[1], "focus") == 0)
    TestFocus();
  else if (strcmp(argv[1], "dnd") == 0 || strcmp(argv[1], "tooltip") == 0)
    TestDragAndTooltip(strcmp(argv[1], "dnd") == 0);
  else if (strcmp(argv[1], "resize") == 0)
    TestResize();
  else if (strcmp(argv[1], "resize-roundtrip") == 0)
    TestResizeRoundtrip();
  else if (strcmp(argv[1], "double-click") == 0)
    TestDoubleClick();
  else if (strcmp(argv[1], "keyboard") == 0 || strcmp(argv[1], "keyboard-no-im") == 0 ||
           strcmp(argv[1], "keyboard-no-im-c") == 0)
    TestKeyboard(strcmp(argv[1], "keyboard") != 0, strcmp(argv[1], "keyboard-no-im-c") != 0);
  else if (strcmp(argv[1], "parent-validation") == 0)
    TestParentValidation();
  else if (strcmp(argv[1], "clipboard") == 0)
    TestClipboard();
  else if (strcmp(argv[1], "clipboard-instances") == 0)
    TestClipboardInstances();
  else if (strcmp(argv[1], "clipboard-fail") == 0)
    TestClipboardFailure();
  else if (strcmp(argv[1], "clipboard-disappearing") == 0 || strcmp(argv[1], "clipboard-host-handler") == 0)
    TestDisappearingClipboardRequestor(strcmp(argv[1], "clipboard-host-handler") == 0);
  else if (strcmp(argv[1], "clipboard-receiver") == 0 && argc == 3)
    TestClipboardReceiver(argv[2]);
  else if (strcmp(argv[1], "native-close") == 0 || strcmp(argv[1], "host-native-close") == 0)
    TestNativeClose(strcmp(argv[1], "host-native-close") == 0);
  else
    TestTimer(strcmp(argv[1], "core-timer-fail") == 0);
  printf("%s: %s\n", argv[1], sFailures ? "FAILED" : "passed");
  return sFailures ? 1 : 0;
}
