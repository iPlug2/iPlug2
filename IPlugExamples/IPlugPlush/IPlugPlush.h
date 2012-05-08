#ifndef __IPLUGPLUSH__
#define __IPLUGPLUSH__

#include "IPlug_include_in_plug_hdr.h"

class IPlugPlush : public IPlug
{
public:

  IPlugPlush(IPlugInstanceInfo instanceInfo);
  ~IPlugPlush();

  void Reset();
  void OnParamChange(int paramIdx);

  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

private:

  double mGain;
};

#endif
