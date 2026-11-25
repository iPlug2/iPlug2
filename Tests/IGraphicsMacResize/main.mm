// Exercise the real Cocoa view and delegate without an Audio Unit or a DAW.
#import <Cocoa/Cocoa.h>
#include "IGraphicsMac.h"
#include <algorithm>
#include <cmath>
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
  ResizeDelegate(bool enabled = true, bool scale = false)
  : IGEditorDelegate(0), mEnabled(enabled), mScale(scale)
  {
    SetEditorSize(640, 480);
  }

  void BeginInformHostOfParamChangeFromUI(int) override {}
  void EndInformHostOfParamChangeFromUI(int) override {}
  // Also builds against the preceding delegate, which has no virtual getter.
  bool GetHostResizeEnabled() const { return mEnabled; }
  IGraphics* CreateGraphics() override { return new IGraphicsMac(*this, 640, 480, 30, 1.f); }

  void OnParentWindowResize(int width, int height) override
  {
    ++mNotifications;
    if (mScale)
    {
      const auto dpi = GetUI()->GetPlatformWindowScale();
      const float scale = std::min(width / (640.f * dpi), height / (480.f * dpi));
      GetUI()->Resize(640, 480, scale, false);
    }
    else
      IGEditorDelegate::OnParentWindowResize(width, height);
  }

  int mNotifications = 0;
private:
  bool mEnabled, mScale;
};

struct Editor
{
  ResizeDelegate mDelegate;
  NSWindow* mWindow;
  NSView* mParent;
  NSView* mView;

  Editor(bool enabled = true, bool scale = false) : mDelegate(enabled, scale)
  {
    mParent = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 640, 480)];
    mWindow = [[NSWindow alloc] initWithContentRect:[mParent frame]
                                        styleMask:NSWindowStyleMaskTitled
                                          backing:NSBackingStoreBuffered defer:NO];
    [mWindow setReleasedWhenClosed:NO];
    [mWindow setContentView:mParent];
    mView = static_cast<NSView*>(mDelegate.OpenWindow(mParent));
    Check(mView != nil, "Could not open the Cocoa editor");
    mDelegate.mNotifications = 0;
  }

  ~Editor()
  {
    if ([mWindow contentView] == mView)
      [mWindow setContentView:mParent];
    mDelegate.CloseWindow();
    [mWindow close];
    [mWindow release];
    [mParent release];
  }
};

static void TestParentAndDirectViewChanges()
{
  Editor editor;
  [editor.mParent setFrameSize:NSMakeSize(800, 600)];
  Check(editor.mDelegate.mNotifications == 1, "Parent resize was not reported once");
  Check(editor.mDelegate.GetUI()->Width() == 800 && editor.mDelegate.GetUI()->Height() == 600,
        "Parent resize did not reach IGraphics");

  editor.mDelegate.mNotifications = 0;
  [editor.mView setFrameSize:NSMakeSize(900, 650)];
  Check(editor.mDelegate.mNotifications == 1, "Direct view resize was not reported once");
  Check(editor.mDelegate.GetUI()->Width() == 900 && editor.mDelegate.GetUI()->Height() == 650,
        "Direct view resize did not reach IGraphics");

  [editor.mParent setFrameOrigin:NSMakePoint(12, 17)];
  Check(editor.mDelegate.mNotifications == 1, "Moving the parent reported an unchanged size");
}

static void TestScaleRequestsAndReentrancy()
{
  Editor editor(true, true);
  editor.mDelegate.GetUI()->Resize(640, 480, 1.5f);
  Check(editor.mDelegate.mNotifications == 0, "UI scaling was echoed as a host resize");
  [editor.mParent setFrameOrigin:NSMakePoint(12, 17)];
  Check(editor.mDelegate.GetUI()->GetDrawScale() == 1.5f, "Unchanged parent undid a scale request");

  [editor.mParent setFrameSize:NSMakeSize(1000, 600)];
  Check(editor.mDelegate.mNotifications == 1, "Fitting the view reentered the resize callback");
  Check(editor.mDelegate.GetUI()->GetDrawScale() == 1.25f, "Host resize did not fit both dimensions");
  [editor.mParent setFrameOrigin:NSMakePoint(20, 25)];
  Check(editor.mDelegate.mNotifications == 1, "Unused parent space caused another resize");

  [editor.mView setFrameSize:NSMakeSize(960, 800)];
  Check(editor.mDelegate.mNotifications == 2, "Direct view fitting reentered the callback");
  Check(editor.mDelegate.GetUI()->GetDrawScale() == 1.5f, "Direct view fitting used stale parent bounds");
}

static void TestDisabledResizing()
{
  Editor editor(false);
  [editor.mParent setFrameSize:NSMakeSize(800, 600)];
  [editor.mView setFrameSize:NSMakeSize(900, 650)];
  Check(editor.mDelegate.mNotifications == 0, "Disabled host resizing was notified");
  Check(editor.mDelegate.GetUI()->Width() == 640, "Disabled host resizing changed IGraphics");
}

static void TestReparentAndClose()
{
  Editor editor;
  NSView* pParent = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 700, 500)];
  [editor.mParent addSubview:pParent];
  [pParent addSubview:editor.mView];
  editor.mDelegate.mNotifications = 0;
  [editor.mParent setFrameSize:NSMakeSize(850, 650)];
  Check(editor.mDelegate.mNotifications == 0, "The old parent still reports resizes");
  [pParent setFrameSize:NSMakeSize(800, 600)];
  Check(editor.mDelegate.mNotifications == 1, "The new parent did not report its resize");
  editor.mDelegate.CloseWindow();
  [pParent setFrameSize:NSMakeSize(900, 700)];
  Check(editor.mDelegate.mNotifications == 1, "Closed editor still receives resize callbacks");
  [pParent removeFromSuperview];
  [pParent release];
}

static void TestContentView()
{
  Editor editor;
  [editor.mWindow setContentView:editor.mView];
  editor.mDelegate.mNotifications = 0;
  [editor.mWindow setContentSize:NSMakeSize(800, 600)];
  Check(editor.mDelegate.mNotifications == 1, "Content-view resize was not reported once");
  Check(editor.mDelegate.GetUI()->Width() == 800 && editor.mDelegate.GetUI()->Height() == 600,
        "Content-view resize included the window title bar");
}

int main()
{
  @autoreleasepool
  {
    [NSApplication sharedApplication];
    try
    {
      TestParentAndDirectViewChanges();
      TestScaleRequestsAndReentrancy();
      TestDisabledResizing();
      TestReparentAndClose();
      TestContentView();
      std::puts("Cocoa AUv2 resize regression passed");
    }
    catch (const std::exception& error)
    {
      std::fprintf(stderr, "%s\n", error.what());
      return 1;
    }
  }
}
