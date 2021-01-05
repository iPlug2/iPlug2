#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IPlugSOUL_DSP.h"
#include <map>

const int kNumPresets = 1;

using namespace iplug;
using namespace igraphics;
using DSP = IPlugSOUL_DSP;

class IPlugSOUL final : public Plugin
{
public:
  IPlugSOUL(const InstanceInfo& info);

#if IPLUG_DSP // http://bit.ly/2S64BDd
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnReset() override;
  void OnParamChange(int paramIdx) override;
  
  int GetIPlugParamIdx(const char* soulParamUID);
private:
  DSP mDSP;
  int mSessionID = 0;
  IPlugQueue<int> mParamsToUpdate {DSP::numParameters};
  std::vector<DSP::Parameter> mSOULParams;
  std::map<const char*, int> mParamMap;
#endif
};
