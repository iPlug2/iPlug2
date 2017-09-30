#ifndef __IPLUGMULTICHANNEL__
#define __IPLUGMULTICHANNEL__

// This example demonstrates a multi-channel plug-in (max 4ins, 4outs)
// the app_wrapper.* code is modified to provide four channels of audio
// currently the IPlugStandalone class and app_wrapper code does not guarantee buffers full of zeros , like VST and AU formats do
// Needless to say, this could be improved (a lot).

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
