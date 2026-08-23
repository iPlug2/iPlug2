#pragma once

#include "IPlug_include_in_plug_hdr.h"

const int kNumPresets = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

using namespace iplug;

namespace cycfi {
namespace elements {
class view;
}
}

class IPlugElements final : public Plugin
{
public:
  IPlugElements(const InstanceInfo& info);

  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  
  void* OpenWindow(void* pParent) override;
  
private:
  std::unique_ptr<cycfi::elements::view> mView;
};
