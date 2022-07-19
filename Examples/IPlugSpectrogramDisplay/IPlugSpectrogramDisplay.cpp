#include "IPlugSpectrogramDisplay.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

IPlugSpectrogramDisplay::IPlugSpectrogramDisplay(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    const IRECT b = pGraphics->GetBounds();
    pGraphics->AttachControl(new ISpectrogramControl<>(b), 0);
  };
#endif
}

void IPlugSpectrogramDisplay::OnIdle()
{
  mSender.TransmitData(*this);
}

#if IPLUG_DSP
void IPlugSpectrogramDisplay::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  mSender.ProcessBlock(inputs, nFrames, 0);
}
#endif
