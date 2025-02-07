#pragma once

#include "IPlug_include_in_plug_hdr.h"

const int kNumPresets = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

namespace MTK {
class View;
}

namespace MTL {
class Device;
}

namespace NS {
class AutoreleasePool;
}

class MyMTKViewDelegate;

using namespace iplug;

class IPlugMetalCPP final : public Plugin
{
public:
  IPlugMetalCPP(const InstanceInfo& info);
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void* OpenWindow(void* pParent) override;
  void CloseWindow() override;
private:
  MTK::View* mpMtkView;
  MTL::Device* mpDevice;
  MyMTKViewDelegate* mpViewDelegate = nullptr;
  NS::AutoreleasePool* mpAutoreleasePool;
};
