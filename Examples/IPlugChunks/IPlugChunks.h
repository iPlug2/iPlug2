#pragma once

#include "IPlug_include_in_plug_hdr.h"

const int kNumPresets = 1;
constexpr int kNumSteps = 16;
constexpr int kBeatDiv = 4;

enum EParams
{
  kParamGain = 0,
  kNumParams
};

enum ECtrlTags
{
  kCtrlMultiSlider = 0
};

enum EMsgTags
{
  kMsgTagSliderChanged = 0
};

using namespace iplug;
using namespace igraphics;

class IPlugChunks final : public Plugin
{
public:
  IPlugChunks(const InstanceInfo& info);

  bool SerializeState(IByteChunk &chunk) const override;
  int UnserializeState(const IByteChunk &chunk, int startPos) override;
//  bool CompareState(const uint8_t* pIncomingState, int startPos) const override;
  
  void OnIdle() override;
  void OnUIOpen() override;
  bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override;

#if IPLUG_DSP // http://bit.ly/2S64BDd
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif
  
private:
  void UpdateUIControls();
  
  std::atomic<int> mStepPos;
  int mPrevPos = -1;
  float mSteps[kNumSteps] = {};
};
