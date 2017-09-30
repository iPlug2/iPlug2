#ifndef __IPLUGMOUSETEST__
#define __IPLUGMOUSETEST__

#include "IPlug_include_in_plug_hdr.h"
#include "IPlugMouseTestDSP.h"

class IPlugMouseTest : public IPlug
{
public:

  IPlugMouseTest(IPlugInstanceInfo instanceInfo);
  ~IPlugMouseTest();

  void Reset();
  void OnParamChange(int paramIdx);
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

private:
  double mSampleRate;
  CWTOsc* mOsc;
  CWTOscState mOsc1_ctx, mOsc2_ctx;
  double* mTable;
};

#endif
