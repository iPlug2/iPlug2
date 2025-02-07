#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "MetalRenderer.h"

const int kNumPresets = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

using namespace iplug;

class IPlugMetalCppUI final : public Plugin
{
public:
  IPlugMetalCppUI(const InstanceInfo& info);
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void* OpenWindow(void* pParent) override;
  void CloseWindow() override;
  void OnParentWindowResize(int width, int height) override;
private:
  std::unique_ptr<MetalUI> mpMetalUI;
};
