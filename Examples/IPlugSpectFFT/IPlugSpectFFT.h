#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IControls.h"
#include "FFTRect.h"

const int kNumPrograms = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

enum ECtrlTags
{
  kCtrlTagMeter = 0,
  kCtrlTagFFT,
  kNumCtrlTags
};

class IPlugSpectFFT : public IPlug
{
public:
  IPlugSpectFFT(IPlugInstanceInfo instanceInfo);

#if IPLUG_DSP   // All DSP methods and member variables should be within an IPLUG_DSP guard, should you want distributed UI
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnIdle() override;
  void OnParamChange(int paramIdx) override;
  void OnReset() override;
private:
  //have pointers to controls in order to reset samplerate, change scale, etc... after the control is created
  IControl* pFFTAnalyzer, *pFFTFreqDraw;
  gFFTAnalyzer<>::Sender mSender{ kCtrlTagFFT };
  double mGain;
#endif
};
