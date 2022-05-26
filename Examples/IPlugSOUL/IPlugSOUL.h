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

#if IPLUG_DSP
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
  void OnReset() override;
  void OnParamChange(int paramIdx) override;
private:
  DSP mDSP;
  int mSessionID = 0;
  IPlugQueue<int> mParamsToUpdate {DSP::numParameters};
  std::vector<DSP::Parameter> mSOULParams;
  std::vector<DSP::MIDIMessage> mIncomingMIDIMessages;
#endif
  int GetIPlugParamIdx(const char* soulParamID) { return mParamMap[soulParamID]; }
  std::map<const char*, int> mParamMap;
};
