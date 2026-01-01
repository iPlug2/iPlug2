#include "IPlugMinimal.h"
#include "IPlug_include_in_plug_src.h"

IPlugMinimal::IPlugMinimal(const InstanceInfo& info)
  : Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitDouble("Gain", 100., 0., 100.0, 0.01, "%");

#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() { return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT)); };

  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    const IPattern pattern = IPattern::CreateLinearGradient(pGraphics->GetBounds(), EDirection::Horizontal, {{COLOR_WHITE, 0.f}, {COLOR_BLACK, 1.f}});
    pGraphics->AttachPanelBackground(pattern);
  };
#endif
}

#if IPLUG_DSP
void IPlugMinimal::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  const int nChans = NOutChansConnected();

  for (int s = 0; s < nFrames; s++)
  {
    for (int c = 0; c < nChans; c++)
    {
      outputs[c][s] = inputs[c][s] * gain;
    }
  }
}
#endif
