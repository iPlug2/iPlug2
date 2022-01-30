#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "ISender.h"

using namespace iplug;

class IPlugCocoaUI final : public Plugin
{
public:
  IPlugCocoaUI(const InstanceInfo& info);
  ~IPlugCocoaUI();
  
  void* OpenWindow(void* pParent) override;
  
  void OnParentWindowResize(int width, int height) override;
  bool OnHostRequestingSupportedViewConfiguration(int width, int height) override;
  bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override;
  void OnParamChange(int paramIdx) override;
  void OnIdle() override;
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  
private:
  
  IPeakSender<> mSender;
};
