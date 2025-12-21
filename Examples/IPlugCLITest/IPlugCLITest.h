#pragma once

#include "IPlug_include_in_plug_hdr.h"

const int kNumPresets = 1;

enum EParams
{
  kGain = 0,
  kFrequency,
  kAttack,
  kDecay,
  kNumParams
};

using namespace iplug;
using namespace igraphics;

class IPlugCLITest final : public Plugin
{
public:
  IPlugCLITest(const InstanceInfo& info);

#if IPLUG_DSP
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
  void OnReset() override;

private:
  double mPhase = 0.0;
  double mFreq = 440.0;
  double mEnvValue = 0.0;
  double mEnvStage = 0; // 0=idle, 1=attack, 2=decay
  int mNote = -1;
  double mVelocity = 0.0;
  double mAttackRate = 0.0;
  double mDecayRate = 0.0;
#endif
};
