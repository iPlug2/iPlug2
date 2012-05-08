#ifndef __IPLUGTEXT__
#define __IPLUGTEXT__

#include "IPlug_include_in_plug_hdr.h"

class IPlugText : public IPlug
{
public:

  IPlugText(IPlugInstanceInfo instanceInfo);
  ~IPlugText();

  void Reset();
  void OnParamChange(int paramIdx);

  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

private:

  double mGain;
};

#endif
