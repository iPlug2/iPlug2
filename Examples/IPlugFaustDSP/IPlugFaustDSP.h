#pragma once

#include "IPlug_include_in_plug_hdr.h"

#if IPLUG_DSP
#include "IPlugFaustGen.h"
#endif

#include "IControls.h"

#ifndef DSP_FILE
#define DSP_FILE ""
#endif

enum EControlTags
{
  kControlTagScope = 0,
  kNumControlTags
};

const int kNumParams = 4;

class IPlugFaustDSP : public IPlug
{
public:
  IPlugFaustDSP(IPlugInstanceInfo instanceInfo);

#if IPLUG_DSP
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnReset() override;
  void OnParamChange(int paramIdx) override;
  void OnIdle() override;
private:
  FAUST_BLOCK(Faust1, mFaustProcessor, DSP_FILE, 1, 1);
  IVScopeControl<1>::IVScopeBallistics mScopeBallistics { kControlTagScope };
#endif
};
