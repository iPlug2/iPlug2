#ifndef __IPLUGMULTICHANNEL__
#define __IPLUGMULTICHANNEL__

#include "IPlug_include_in_plug_hdr.h"

class IPlugMultiChannel : public IPlug
{
public:

  IPlugMultiChannel(IPlugInstanceInfo instanceInfo);
  ~IPlugMultiChannel();

  void Reset();
  void OnParamChange(int paramIdx);

  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

private:

  double mGain;
};

#endif
