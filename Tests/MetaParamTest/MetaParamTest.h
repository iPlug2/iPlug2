#pragma once

#include "IPlug_include_in_plug_hdr.h"

const int kNumPrograms = 1;

enum EParams
{
  kParamLeftX = 0,
  kParamLeftY,
  kParamRightX,
  kParamRightY,
  kParamLink,
  kParamEnum1,
  kParamEnum2,
  kNumParams
};

enum ECtrlTags
{
  kCtrlLeftXYPad = 0,
  kCtrlRightXYPad,
  kCtrlLeftXKnob,
  kCtrlLeftYKnob,
  kCtrlRightXKnob,
  kCtrlRightYKnob
};

using namespace iplug;
using namespace igraphics;

class MetaParamTest : public Plugin
{
public:
  MetaParamTest(const InstanceInfo& info);

#if IPLUG_DSP // All DSP methods and member variables should be within an IPLUG_DSP guard, should you want distributed UI
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif
};
