#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "IWebViewControl.h"

IPlugEffect::IPlugEffect(const InstanceInfo& info)
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
    pGraphics->AttachControl(new IVButtonControl(b.GetFromTop(300).GetCentredInside(100,100),
    [](IControl* pControl){
      SplashClickActionFunc(pControl);
    }))->SetAnimationEndActionFunction([](IControl* pControl){
      pControl->GetUI()->GetControlWithTag(0)->Hide(!pControl->GetUI()->GetControlWithTag(0)->IsHidden());
    });
//    pGraphics->AttachControl(new ITextControl(b.GetMidVPadded(50), "Hello iPlug 2!", IText(50)));
//    pGraphics->AttachControl(new IVKnobControl(b.GetCentredInside(100).GetVShifted(-100), kGain));
    
    bool showDevTools = false;
    
#if DEBUG
    showDevTools = true;
#endif
    
    IWebViewControl* pWebViewControl;
    pGraphics->AttachControl(pWebViewControl = new IWebViewControl(b.GetFromBottom(300), false, [](IWebViewControl* pWebView){
      pWebView->LoadHTML("OSC Console");
    }, nullptr, showDevTools), 0);
    pWebViewControl->SetIgnoreMouse(true);
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
