#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "ISender.h"
#include "IControls.h"

const int kNumPresets = 1;

enum EParams
{
  kColorMapRange = 0,
  kColorMapContrast,
  kNumParams
};

using namespace iplug;
using namespace igraphics;

class IPlugSpectrogramDisplay final : public Plugin
{
public:
  IPlugSpectrogramDisplay(const InstanceInfo& info);

  void OnIdle() override;

#if IPLUG_DSP // http://bit.ly/2S64BDd
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnReset() override;
  bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override;
  ISpectrumSender<> mSender;
#endif
  
#if IPLUG_EDITOR
  void OnParentWindowResize(int width, int height) override;
  void OnParamChangeUI(int paramIdx, EParamSource source) override;
#endif
  
 protected:
  int mFFTSize = 1024;
  int mOverlap = 2;
};
