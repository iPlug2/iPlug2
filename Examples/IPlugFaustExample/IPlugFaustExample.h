#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IPlugFaustGen.h"
#include "IVScopeControl.h"

enum EControlTags
{
  kControlTagScope = 0,
  kNumControlTags
};

const int kNumParams = 8;

class IPlugFaustExample : public IPlug
{
public:
  IPlugFaustExample(IPlugInstanceInfo instanceInfo);

  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnReset() override;
  void OnParamChange(int paramIdx) override;
  FAUST_BLOCK(Faust1, mFaustProcessor, DSP_FILE, 1, 1);
    
  void OnIdle() override;
private:
  IVScopeControl<1>::IVScopeBallistics mScopeBallistics { kControlTagScope };
};
