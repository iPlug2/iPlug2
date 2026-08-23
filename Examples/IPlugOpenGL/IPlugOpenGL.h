#pragma once

#include "IPlug_include_in_plug_hdr.h"


const int kNumPresets = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

using namespace iplug;

class CubeView;

namespace pugl
{
class World;
}

class IPlugOpenGL final : public Plugin
{
public:
  IPlugOpenGL(const InstanceInfo& info);
  
  void OnIdle() override;

  void* OpenWindow(void* pParent) override;
  void CloseWindow() override;
  void OnParentWindowResize(int width, int height) override;

  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  
private:
  std::unique_ptr<pugl::World> mWorld;
  std::unique_ptr<CubeView> mView;
};
