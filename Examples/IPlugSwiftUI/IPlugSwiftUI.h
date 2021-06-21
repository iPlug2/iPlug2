#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "ISender.h"

using namespace iplug;

class IPlugSwiftUI final : public Plugin
{
public:
  IPlugSwiftUI(const InstanceInfo& info);

  void* OpenWindow(void* pParent) override;
  
  void OnParentWindowResize(int width, int height) override;
  bool OnHostRequestingSupportedViewConfiguration(int width, int height) override;

  void OnIdle() override;
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override;

  IPeakSender<> mSender;
};
