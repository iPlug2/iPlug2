#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IPlugFaustGen.h"
#include "IControls.h"

#ifndef DSP_FILE
#define DSP_FILE ""
#endif

enum EControlTags
{
  kControlTagScope = 0,
  kNumControlTags
};

const int kNumParams = 8;

class IPlugFaustDSP : public IPlug
{
public:
  IPlugFaustDSP(IPlugInstanceInfo instanceInfo);

  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnReset() override;
  void OnParamChange(int paramIdx) override;
  FAUST_BLOCK(Faust1, mFaustProcessor, DSP_FILE, 1, 1);
    
  void OnIdle() override;
private:
  IVScopeControl<1>::IVScopeBallistics mScopeBallistics { kControlTagScope };
};
