#include "ReaperExt_include_in_plug_hdr.h"
#include <thread>

using namespace iplug;
using namespace iplug::igraphics;

static const auto sMainThread = std::this_thread::get_id();
static unsigned sIdle = 0, sOpens = 0, sCloses = 0, sErrors = 0, sDraws = 0;

class LinuxReaperProbe final : public iplug::ReaperExtBase
{
public:
  explicit LinuxReaperProbe(reaper_plugin_info_t* pRec);
  void OnIdle() override
  {
    ++sIdle;
    sErrors += std::this_thread::get_id() != sMainThread;
  }
  void OnUIOpen() override { ++sOpens; }
  void OnUIClose() override
  {
    ++sCloses;
    sErrors += std::this_thread::get_id() != sMainThread || !GetUI() ||
      !GetUI()->GetBackgroundControl() ||
      static_cast<iplug::igraphics::IGraphicsLinux*>(GetUI())->IsInPlatformCallback();
  }
};

#include "ReaperExt_include_in_plug_src.h"
#include "IControls.h"

class ProbeControl final : public iplug::igraphics::IPanelControl
{
public:
  explicit ProbeControl(LinuxReaperProbe& probe)
  : IPanelControl(iplug::igraphics::IRECT(100, 100, 400, 400), iplug::igraphics::COLOR_WHITE)
  , mProbe(probe)
  {
    mIgnoreMouse = false;
  }

  void Draw(iplug::igraphics::IGraphics& graphics) override
  {
    ++sDraws;
    sErrors += std::this_thread::get_id() != sMainThread;
    IPanelControl::Draw(graphics);
  }

  void OnMouseDown(float, float, const iplug::igraphics::IMouseMod&) override
  {
    sErrors += std::this_thread::get_id() != sMainThread;
    auto* graphics = mProbe.GetUI();
    if (sOpens == 1)
      mProbe.ToggleDocking();
    else
      mProbe.ShowHideMainWindow();
    sErrors += mProbe.GetUI() != graphics;
    SWELL_RunMessageLoop();
    sErrors += mProbe.GetUI() != graphics;
  }

private:
  LinuxReaperProbe& mProbe;
};

LinuxReaperProbe::LinuxReaperProbe(reaper_plugin_info_t* pRec)
: ReaperExtBase(pRec)
{
  mMakeGraphicsFunc = [this] { return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS); };
  mLayoutFunc = [this](iplug::igraphics::IGraphics* graphics) {
    graphics->AttachPanelBackground(iplug::igraphics::COLOR_GRAY);
    graphics->AttachControl(new ProbeControl(*this));
  };
}

extern "C" void LinuxReaperProbeShow() { gPlug->ShowHideMainWindow(); }
extern "C" void LinuxReaperProbeStop() { gPlug.reset(); }
extern "C" void* LinuxReaperProbeWindow()
{
  return gPlug->GetUI() ? gPlug->GetUI()->GetWindow() : nullptr;
}
extern "C" HWND LinuxReaperProbeContainer() { return gHWND; }
extern "C" unsigned LinuxReaperProbeCount(unsigned kind)
{
  switch (kind)
  {
    case 0: return sIdle;
    case 1: return sOpens;
    case 2: return sCloses;
    case 3: return sErrors;
    case 4: return sDraws;
    default: fprintf(stderr, "Invalid REAPER probe counter\n"); abort();
  }
}
