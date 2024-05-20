#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

IPlugEffect::IPlugEffect(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS);
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    const IRECT b = pGraphics->GetBounds();

#ifdef AUv3_API
    std::vector<std::string> auv3UserPresets;
    this->GetListOfAUv3UserPresets(auv3UserPresets);
    pGraphics->AttachControl(new ILambdaControl(b.GetPadded(-10),
      [auv3UserPresets](ILambdaControl* pCaller, IGraphics& g, IRECT& rect) {
        g.FillRect(COLOR_WHITE, rect);
        for (auto i=0; i<auv3UserPresets.size(); i++) {
          g.DrawText(IText(20), auv3UserPresets[i].c_str(), rect.SubRectVertical(int(auv3UserPresets.size()), i));
        }
      
    }, DEFAULT_ANIMATION_DURATION, false, false));
#endif
    
  };
#endif
}

#if IPLUG_DSP
void IPlugEffect::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  const int nChans = NOutChansConnected();
  
  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s] * gain;
    }
  }
}
#endif
