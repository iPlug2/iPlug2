#include "IPlugP5js.h"
#include "IPlug_include_in_plug_src.h"

#include <cstring>

IPlugP5js::IPlugP5js(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitGain("Gain", -70., -70, 0.);

#ifdef WEBVIEW_EDITOR_DELEGATE
  SetCustomUrlScheme("iplug2");
  SetEnableDevTools(true);
  
  mEditorInitFunc = [&]() {
    LoadIndexHtml(__FILE__, GetBundleID());
    EnableScroll(false);
  };
#endif
  
  MakeDefaultPreset();
}

#if IPLUG_DSP
void IPlugP5js::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  (void) inputs;

  const int nChans = NOutChansConnected();

  for (int c = 0; c < nChans; c++)
  {
    memset(outputs[c], 0, nFrames * sizeof(sample));
  }
}
#endif
