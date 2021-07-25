#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "ISender.h"

using namespace iplug;

class IPlugUIKit final : public Plugin
{
public:
  IPlugUIKit(const InstanceInfo& info);

  void* OpenWindow(void* pParent) override;
  
  void OnIdle() override;
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override;
  void OnParamChange(int paramIdx) override;
  
  IPeakSender<> mSender;
};
