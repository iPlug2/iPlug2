#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IPlugEffect_DSP.h"

const int kNumPrograms = 1;

#define MAX_VOICES 32

enum EParams
{
  kPolyMode = 0,
  kNumParams
};

class IPlugEffect : public IPlug
{
public:
  IPlugEffect(IPlugInstanceInfo instanceInfo);
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnReset() override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
  void OnParamChange(int paramIdx, EParamSource) override;
private:
  IPlugEffectDSP mDSP;
};
