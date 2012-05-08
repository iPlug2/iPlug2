#ifndef __IPLUGHOSTDETECT__
#define __IPLUGHOSTDETECT__

#include "IPlug_include_in_plug_hdr.h"

class IPlugHostDetect : public IPlug
{
public:

  IPlugHostDetect(IPlugInstanceInfo instanceInfo);
  ~IPlugHostDetect();

  void Reset();
  void OnParamChange(int paramIdx);

  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);
  void OnHostIdentified();

private:
  ITextControl* mHostNameControl;
  ITextControl* mHostVersionControl;
};

#endif
