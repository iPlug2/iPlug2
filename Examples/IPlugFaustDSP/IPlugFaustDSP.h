#pragma once

#include "IPlug_include_in_plug_hdr.h"

#if IPLUG_DSP
#include "IPlugFaustGen.h"
#endif

#ifndef DSP_FILE
#define DSP_FILE ""
#endif

#include "IPlugFaustIGraphicsUI.h"

enum EControlTags
{
  kCtrlTagScope = 0,
  kNumCtrlTags
};

const int kNumParams = 22;

using namespace iplug;
using namespace igraphics;

class IPlugFaustDSP final : public Plugin
{
public:
  IPlugFaustDSP(const InstanceInfo& info);

#if IPLUG_DSP
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnReset() override;
  void ProcessMidiMsg(const IMidiMsg& msg) override;
  void OnParamChange(int paramIdx) override;
  void OnIdle() override;
private:
//  FAUST_BLOCK(Faust1, mFaustProcessor, DSP_FILE, 1, 1);
  std::unique_ptr<FaustGen> mFaustProcessor;
  std::unique_ptr<IGraphicsFaustUI> mUIBuilder;
  IBufferSender<2> mScopeSender;
#endif
};
