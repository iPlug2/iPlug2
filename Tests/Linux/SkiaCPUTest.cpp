#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>
#include <thread>
#include "IGraphicsLinux.h"
#include "IControl.h"

using namespace iplug;
using namespace iplug::igraphics;

// Keep resize requests ordered across the editor and screenshot connections,
// as in the existing Linux runtime fixture. Otherwise an older ConfigureNotify
// can be consumed while the editor's latest resize is still buffered in Xlib.
extern "C" int XResizeWindow(Display* display, Window window, unsigned width, unsigned height)
{
  static auto resize = reinterpret_cast<decltype(&XResizeWindow)>(dlsym(RTLD_NEXT, "XResizeWindow"));
  if (!resize)
    abort();
  const int result = resize(display, window, width, height);
  XSync(display, 0);
  return result;
}

class ColorControl final : public IControl
{
public:
  ColorControl() : IControl(IRECT(0, 0, 320, 240)) {}
  void Draw(IGraphics& graphics) override
  {
    graphics.FillRect(COLOR_RED, IRECT(0, 0, 160, 120));
    graphics.FillRect(COLOR_BLUE, IRECT(160, 0, 320, 120));
    graphics.FillRect(COLOR_GREEN, IRECT(0, 120, 320, 240));
    graphics.FillRect(IColor(128, 255, 0, 0), IRECT(240, 0, 320, 120));
  }
};

class Delegate final : public IGEditorDelegate
{
public:
  Delegate() : IGEditorDelegate(0)
  {
    mMakeGraphicsFunc = [this]() {
      auto* graphics = new IGraphicsLinux(*this, 320, 240, 60, 1.f);
      graphics->SetHostDriven(true);
      return graphics;
    };
    mLayoutFunc = [](IGraphics* graphics) { graphics->AttachControl(new ColorControl); };
  }
  void BeginInformHostOfParamChangeFromUI(int) override {}
  void EndInformHostOfParamChangeFromUI(int) override {}
};

static unsigned Channel(unsigned long pixel, unsigned long mask)
{
  if (!mask)
    return 0;
  while (!(mask & 1)) { pixel >>= 1; mask >>= 1; }
  return static_cast<unsigned>(((pixel & mask) * 255 + mask / 2) / mask);
}

static bool CheckFrame(Display* display, IGraphicsLinux& graphics, int width, int height)
{
  const auto window = reinterpret_cast<uintptr_t>(graphics.GetWindow());
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
  while (std::chrono::steady_clock::now() < deadline)
  {
    graphics.OnDisplayTimer();
    XWindowAttributes attributes{};
    XSync(display, 0);
    if (XGetWindowAttributes(display, window, &attributes) && attributes.map_state == IsViewable &&
        attributes.width == width && attributes.height == height)
    {
      XImage* image = XGetImage(display, window, 0, 0, width, height, AllPlanes, ZPixmap);
      if (image)
      {
        const auto matches = [image](int x, int y, int red, int green, int blue) {
          const unsigned long pixel = XGetPixel(image, x, y);
          const auto near = [](int a, int b) { return std::abs(a - b) <= 8; };
          return near(Channel(pixel, image->red_mask), red) &&
                 near(Channel(pixel, image->green_mask), green) &&
                 near(Channel(pixel, image->blue_mask), blue);
        };
        const bool passed = matches(width / 4, height / 4, 255, 0, 0) &&
                            matches(width * 5 / 8, height / 4, 0, 0, 255) &&
                            matches(width / 4, height * 3 / 4, 0, 255, 0) &&
                            matches(width * 7 / 8, height / 4, 128, 0, 127);
        XDestroyImage(image);
        if (passed)
          return true;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  XWindowAttributes actual{};
  XGetWindowAttributes(display, window, &actual);
  fprintf(stderr, "Incorrect CPU frame or dimensions (%dx%d), actual %dx%d design %dx%d draw=%f screen=%f\n", width, height, actual.width, actual.height, graphics.Width(), graphics.Height(), graphics.GetDrawScale(), graphics.GetScreenScale());
  return false;
}

int main()
{
  Display* display = XOpenDisplay(nullptr);
  if (!display)
    return 1;
  int opcode, event, error;
  if (XQueryExtension(display, "GLX", &opcode, &event, &error))
  {
    fprintf(stderr, "This CPU regression must run without GLX\n");
    XCloseDisplay(display);
    return 1;
  }
  Delegate delegate;
  bool passed = true;
  for (int cycle = 0; cycle < 3; ++cycle)
  {
    if (!delegate.OpenWindow(nullptr))
    {
      passed = false;
      break;
    }
    auto* graphics = static_cast<IGraphicsLinux*>(delegate.GetUI());
    // The delegate remembers editor dimensions across close/open. Reset the
    // baseline explicitly before checking this cycle.
    graphics->SetScreenScaleFromHost(1.f);
    graphics->Resize(320, 240, 1.f);
    passed &= CheckFrame(display, *graphics, 320, 240);
    // Draw scale and screen scale both change the physical raster dimensions.
    graphics->Resize(320, 240, 1.5f);
    passed &= CheckFrame(display, *graphics, 480, 360);
    graphics->SetScreenScaleFromHost(1.25f);
    passed &= CheckFrame(display, *graphics, 600, 450);
    // Server damage must repaint even when controls were previously clean.
    XClearArea(display, reinterpret_cast<uintptr_t>(graphics->GetWindow()), 0, 0, 0, 0, 1);
    XSync(display, 0);
    passed &= CheckFrame(display, *graphics, 600, 450);
    delegate.CloseWindow();
  }
  XCloseDisplay(display);
  return passed ? 0 : 1;
}
