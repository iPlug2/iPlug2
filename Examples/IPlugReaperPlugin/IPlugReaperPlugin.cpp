#include "IPlugReaperPlugin.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

#ifdef VST2_API
extern int (*GetPlayState)();
#endif

IPlugReaperPlugin::IPlugReaperPlugin(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");

#ifdef VST2_API
  // Get function pointers to the APIs you need
  *(VstIntPtr*)&GetPlayState = GetHostCallback()(NULL, 0xdeadbeef, 0xdeadf00d, 0, (void*) "GetPlayState", 0.0);
#endif

  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    const IRECT b = pGraphics->GetBounds();

    #ifdef VST2_API
    pGraphics->AttachControl(new ILambdaControl(b,
      [](ILambdaControl* pCaller, IGraphics& g, IRECT& rect) {
      
        if (GetPlayState && GetPlayState()) {
          g.FillRect(COLOR_GREEN, rect);
            g.DrawText({30}, "PLAYING", rect);
        }
        else {
          g.FillRect(COLOR_RED, rect);
          g.DrawText({30}, "STOPPED", rect);
        }
      
    }, DEFAULT_ANIMATION_DURATION, true /*loop*/, true /*start immediately*/));
    #else
    pGraphics->AttachControl(new ITextControl(b, "Please build the VST2 plugin"));
    #endif
  };
}

void IPlugReaperPlugin::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  const int nChans = NOutChansConnected();
  
  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s] * gain;
    }
  }
}
