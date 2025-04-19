#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "ISender.h"

using namespace iplug;

constexpr int SCOPE_BUFFER_SIZE = 512;

class IPlugVger final : public Plugin
{
public:
  IPlugVger(const InstanceInfo& info);

  void* OpenWindow(void* pParent) override;
  
  void OnParentWindowResize(int width, int height) override;
  bool OnHostRequestingSupportedViewConfiguration(int width, int height) override;

  void OnIdle() override;
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override;

  using OscilloscopeSender = IBufferSender<1, 1, SCOPE_BUFFER_SIZE>;

  OscilloscopeSender mScopeSender {-100.0};
};
