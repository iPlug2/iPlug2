#ifndef __IPLUGEFFECT__
#define __IPLUGEFFECT__

/*
 
 IPlug Resampler example
 Oli Larkin, 2011
 Thanks to Justin Frankel for explaining how to use it

 A simple IPlug plug-in effect that shows how to use WDL_Resampler
 On load it writes a 441hz sine wave into an array of 44100 double precision samples. 
 If you change the sample rate in the host you still hear a 441hz sine wave, 
 The plugin will resample the 44100hz sample to the host samplerate.

*/

#include "IPlug_include_in_plug_hdr.h"

//if the audio you want to resample is single precision uncomment this line 
//and make appropriate changes in ProcessDoubleReplacing()
//#define WDL_RESAMPLE_TYPE float

#include "../../WDL/resample.h"

class IPlugResampler : public IPlug
{
public:

  IPlugResampler(IPlugInstanceInfo instanceInfo);
  ~IPlugResampler();

  void Reset();
  void OnParamChange(int paramIdx);

  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

  private:

  int mSampleIndx;
  WDL_Resampler mResampler;
  WDL_ResampleSample mAudioToResample[44100];
  double mGain;
};

#endif
