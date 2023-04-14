#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "ISender.h"

using namespace iplug;

const int kNumPresets = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

//enum EMsgTags
//{
//};

enum EControlTags
{
  kCtrlTagMeter = 0,
};

class IPlugP5js final : public Plugin
{
public:
  IPlugP5js(const InstanceInfo& info);
  
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
  void OnReset() override;
  void OnIdle() override;
  bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override;
  void OnParamChange(int paramIdx) override;

  void OnMidiMsgUI(const IMidiMsg& msg) override;
  
private:
  IPeakAvgSender<2> mSender;
};
