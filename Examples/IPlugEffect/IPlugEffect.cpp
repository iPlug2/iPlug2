#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "IWebViewControl.h"

IPlugEffect::IPlugEffect(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
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
    pGraphics->AttachControl(new ILambdaControl(b,
    [](ILambdaControl* pCaller, IGraphics& g, IRECT& r) {
      const float radius = r.W();
      const float x = r.MW();
      const float y = r.MH();
      const float rotate = float(pCaller->GetAnimationProgress() * PI);
      
      for(int index = 0, limit = 40; index < limit; ++index)
      {
        float firstAngle = float ((index * 2 * PI) / limit);
        float secondAngle = float (((index + 1) * 2 * PI) / limit);
        
        g.PathTriangle(x, y,
                       x + std::sin(firstAngle + rotate) * radius, y + std::cos(firstAngle + rotate) * radius,
                       x + std::sin(secondAngle + rotate) * radius, y + std::cos(secondAngle + rotate) * radius);
        
        if(index % 2)
          g.PathFill(COLOR_RED);
        else
          g.PathFill(pCaller->mMouseInfo.ms.L ? COLOR_VIOLET : COLOR_BLUE);
      }
      
    }, 1000, false));
    pGraphics->AttachControl(new IWebViewControl(b.GetCentredInside(300), false, [](IWebViewControl* pControl){
      pControl->LoadHTML("<body style='<background-color: rgba(0,0,0,0)'><h1 style='color: white'>Hello WebView</h1></body>");
    }))->SetIgnoreMouse(true);
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
