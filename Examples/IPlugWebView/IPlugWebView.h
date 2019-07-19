#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "Oscillator.h"

const int kNumPrograms = 3;

enum EParams
{
  kGain = 0,
  kNumParams
};

enum EMsgTags
{
  kMsgTagButton1 = 0,
  kMsgTagButton2 = 1,
  kMsgTagButton3 = 2,
};

class IPlugWebView : public IPlug
{
public:
  IPlugWebView(IPlugInstanceInfo instanceInfo);
  
  void OnIdle() override;
  bool OnMessage(int messageTag, int controlTag, int dataSize, const void* pData) override;
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  
private:
  float mLastPeakL = 0.;
  float mLastPeakR = 0.;
  FastSinOscillator<sample> mOscillator {0., 440.};
};
