#include "IPlugP5js.h"
#include "IPlug_include_in_plug_src.h"

IPlugP5js::IPlugP5js(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitGain("Gain", -70., -70, 0.);

  SetCustomUrlScheme("iplug2");
  SetEnableDevTools(true);
  
  mEditorInitFunc = [&]() {
    LoadIndexHtml(__FILE__, GetBundleID());
    EnableScroll(false);
  };
  
  MakeDefaultPreset();
}

void IPlugP5js::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
}
