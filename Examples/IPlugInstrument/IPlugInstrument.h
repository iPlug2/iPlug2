#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IPlugInstrument_DSP.h"

const int kNumPrograms = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

class IPlugInstrument : public IPlug
{
public:
  IPlugInstrument(IPlugInstanceInfo instanceInfo);

#if IPLUG_DSP // All DSP methods and member variables should be within an IPLUG_DSP guard, should you want distributed UI
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
  void OnReset() override;
private:
  IPlugInstrumentDSP mDSP {16};
#endif
};
