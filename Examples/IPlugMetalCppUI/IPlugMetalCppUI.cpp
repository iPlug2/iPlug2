#include "IPlugMetalCppUI.h"
#include "IPlug_include_in_plug_src.h"

#include "MetalRenderer.h"

IPlugMetalCppUI::IPlugMetalCppUI(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");
}

void* IPlugMetalCppUI::OpenWindow(void* pParent)
{
  #ifdef AUv3_API
#ifdef OS_MAC
  const char* bundleID = "com.AcmeInc.app.IPlugMetalCppUI.AUv3Framework";
#else
  const char* bundleID = "com.AcmeInc.IPlugMetalCppUI.AUv3Framework";
#endif
#else
  const char* bundleID = GetBundleID();
#endif
  mpMetalUI = std::make_unique<MetalUI>();
  return mpMetalUI->OpenWindow(pParent, bundleID);
}

void IPlugMetalCppUI::CloseWindow()
{
  mpMetalUI = nullptr;
}

void IPlugMetalCppUI::OnParentWindowResize(int width, int height)
{
  if (mpMetalUI) {
    mpMetalUI->setSize(width, height);
  }
}

void IPlugMetalCppUI::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  const int nChans = NOutChansConnected();
  
  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nChans; c++) {
        outputs[c][s] = inputs[c][s] * gain;
      }
    }
}


