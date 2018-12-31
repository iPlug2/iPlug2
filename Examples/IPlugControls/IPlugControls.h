#pragma once

#include "IPlug_include_in_plug_hdr.h"

const int kNumPrograms = 1;

enum EParams
{
  kGain = 0,
  kMode,
  kNumParams
};

enum EControlTags
{
  kCtrlTagDialogResult = 0,
  kControlTags
};

class IPlugControls : public IPlug
{
public:
  IPlugControls(IPlugInstanceInfo instanceInfo);

#if IPLUG_DSP // All DSP methods and member variables should be within an IPLUG_DSP guard, should you want distributed UI
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif
};
