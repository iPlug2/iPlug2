#ifndef __IPLUGMONOSYNTH__
#define __IPLUGMONOSYNTH__

#include "IPlug_include_in_plug_hdr.h"

class IPlugMonoSynth : public IPlug
{
public:
  IPlugMonoSynth(IPlugInstanceInfo instanceInfo);
  ~IPlugMonoSynth();

  void Reset();
  void OnParamChange(int paramIdx);
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

private:
  double mGain;
};

#endif
