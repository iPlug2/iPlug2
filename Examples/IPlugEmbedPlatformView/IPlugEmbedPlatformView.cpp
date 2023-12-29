#include "IPlugEmbedPlatformView.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

#include "IVTabbedPagesControl.h"
#include "MyPlatformControl.h"

typedef IContainerBase Parent;

IPlugEmbedPlatformView::IPlugEmbedPlatformView(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");

  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, 1.0f);
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    const IRECT b_r = pGraphics->GetBounds();
    const IRECT m_r = b_r.GetPadded(-10);

    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    
    auto resizeFunc = [](IContainerBase* pCaller, const IRECT& r) {
      auto innerBounds = r.GetPadded(-10);
      pCaller->GetChild(0)->SetTargetAndDrawRECTs(innerBounds);
    };
    
    pGraphics->AttachControl(new IVTabbedPagesControl(m_r,
    {
      {"VSS", new IVTabPage([](Parent* pParent, const IRECT& r) {
        pParent->AddChildControl(new MyPlatformControl(IRECT()), kCtrlTagPlatformView);
      }, resizeFunc)},
      {"Levels", new IVTabPage([](Parent* pParent, const IRECT& r) {
        pParent->AddChildControl(new IVPeakAvgMeterControl<32>(IRECT(), "", DEFAULT_STYLE.WithShowValue(false)), kCtrlTagInputMeters);
      }, resizeFunc)}
    }, "", DEFAULT_STYLE.WithDrawFrame(false), 15.0f));
  };
}
