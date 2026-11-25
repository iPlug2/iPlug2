// Exercise the actual X11 platform view without a plug-in format or DAW.
#include "IGraphicsLinux.h"
#include <cstdio>
#include <stdexcept>

using namespace iplug;
using namespace iplug::igraphics;

static void Check(bool condition, const char* message)
{
  if (!condition)
    throw std::runtime_error(message);
}

class ResizeDelegate : public IGEditorDelegate
{
public:
  explicit ResizeDelegate(bool enabled) : IGEditorDelegate(0), mEnabled(enabled)
  {
    SetSizeConstraints(1, 2000, 1, 2000);
    SetEditorSize(640, 480);
  }

  void BeginInformHostOfParamChangeFromUI(int) override {}
  void EndInformHostOfParamChangeFromUI(int) override {}
  bool GetHostResizeEnabled() const override { return mEnabled; }
  IGraphics* CreateGraphics() override
  {
    auto* pGraphics = new IGraphicsLinux(*this, 640, 480, 30, 1.f);
    pGraphics->SetHostDriven(true);
    return pGraphics;
  }

  void OnParentWindowResize(int width, int height) override
  {
    ++mNotifications;
    mWidth = width;
    mHeight = height;
    IGEditorDelegate::OnParentWindowResize(width, height);
  }

  int mNotifications = 0;
  int mWidth = 0, mHeight = 0;
private:
  bool mEnabled;
};

class Editor
{
public:
  explicit Editor(bool enabled = true) : mDelegate(enabled)
  {
    XInitThreads();
    mDisplay = XOpenDisplay(nullptr);
    Check(mDisplay != nullptr, "Could not open the X11 display");
    mParent = CreateParent(640, 480);
    Check(mDelegate.SetHostScreenScale(1.f), "Could not set the editor screen scale");
    mChild = reinterpret_cast<uintptr_t>(mDelegate.OpenWindow(reinterpret_cast<void*>(mParent)));
    Check(mChild != 0, "Could not embed the X11 editor");
    Pump();
  }

  ~Editor()
  {
    mDelegate.CloseWindow();
    XCloseDisplay(mDisplay);
  }

  Window CreateParent(int width, int height)
  {
    XSetWindowAttributes attributes{};
    attributes.override_redirect = true;
    const Window parent = XCreateWindow(mDisplay, DefaultRootWindow(mDisplay), 0, 0,
      width, height, 0, CopyFromParent, InputOutput, CopyFromParent, CWOverrideRedirect, &attributes);
    XMapWindow(mDisplay, parent);
    XSync(mDisplay, false);
    return parent;
  }

  void Pump()
  {
    XSync(mDisplay, false);
    static_cast<IGraphicsLinux*>(mDelegate.GetUI())->ProcessX11Events();
  }

  void Expect(int notifications, int width, int height)
  {
    Check(mDelegate.mNotifications == notifications, "Unexpected delegate resize notification count");
    Check(mDelegate.mWidth == width && mDelegate.mHeight == height, "Wrong host dimensions delivered to the delegate");
  }

  ResizeDelegate mDelegate;
  Display* mDisplay = nullptr;
  Window mParent = 0, mChild = 0;
};

static void TestParentResize()
{
  Editor editor;
  editor.Expect(0, 0, 0);
  XResizeWindow(editor.mDisplay, editor.mParent, 800, 610);
  editor.Pump();
  editor.Expect(1, 800, 610);
  auto* pGraphics = editor.mDelegate.GetUI();
  Check(pGraphics->WindowWidth() == 800 && pGraphics->WindowHeight() == 610,
        "Parent-only resize did not resize the graphics view");

  XMoveWindow(editor.mDisplay, editor.mParent, 24, 16);
  editor.Pump();
  editor.Expect(1, 800, 610);

  XResizeWindow(editor.mDisplay, editor.mParent, 800, 550);
  editor.Pump();
  editor.Expect(2, 800, 550);

  pGraphics->Resize(640, 480, 1.f, false);
  XResizeWindow(editor.mDisplay, editor.mParent, 640, 480);
  editor.Pump();
  editor.Expect(2, 800, 550);
}

static void TestReparentAndClose()
{
  Editor editor;
  const auto newParent = editor.CreateParent(720, 520);
  XReparentWindow(editor.mDisplay, editor.mChild, newParent, 0, 0);
  XDestroyWindow(editor.mDisplay, editor.mParent);
  editor.Pump();
  editor.Expect(1, 720, 520);

  XResizeWindow(editor.mDisplay, newParent, 900, 600);
  editor.Pump();
  editor.Expect(2, 900, 600);

  editor.mDelegate.CloseWindow();
  XResizeWindow(editor.mDisplay, newParent, 960, 640);
  XSync(editor.mDisplay, false);
  editor.Expect(2, 900, 600);
}

static void TestDisabledHostResize()
{
  Editor editor(false);
  XResizeWindow(editor.mDisplay, editor.mParent, 800, 610);
  editor.Pump();
  editor.Expect(0, 0, 0);
}

int main()
{
  try
  {
    TestParentResize();
    TestReparentAndClose();
    TestDisabledHostResize();
    return 0;
  }
  catch (const std::exception& error)
  {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
  }
}
