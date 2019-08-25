#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "Oscillator.h"

using namespace iplug;

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

enum ECtrlTags
{
  kCtrlTagMeter = 0,
};

class ParamSmooth
{
private:
  sample mA = 0.99;
  sample mB = 0.01;
  sample mOutM1 = 0.;
  
public:
  inline sample Process(sample input)
  {
    mOutM1 = (input * mB) + (mOutM1 * mA);
    return mOutM1;
  }
};

class IPlugWebView : public Plugin
{
public:
  IPlugWebView(const InstanceInfo& info);
  
  void OnIdle() override;
  bool OnMessage(int messageTag, int controlTag, int dataSize, const void* pData) override;
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  
private:
  float mLastPeak = 0.;
  FastSinOscillator<sample> mOscillator {0., 440.};
  ParamSmooth mGainSmoother;
};
