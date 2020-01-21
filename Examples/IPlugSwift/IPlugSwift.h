#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "Oscillator.h"
#include "Smoothers.h"
#include "IPlugSwiftSharedConstants.h"

using namespace iplug;

class IPlugSwift : public Plugin
{
public:
  IPlugSwift(const InstanceInfo& info);
  
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
  bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override;
  void OnParamChange(int paramIdx) override;
  void OnIdle() override;
  
private:
  FastSinOscillator<float> mOsc;
  float mFreqCPS = 440.f;
  LogParamSmooth<sample> mGainSmoother { 20.f };
  float mVizBuffer1[kDataPacketSize];
  float mVizBuffer2[kDataPacketSize];
  float* mActiveBuffer = &mVizBuffer2[0];
  int mCount = 0;
  bool mBufferFull = false;
};
