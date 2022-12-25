#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "ISender.h"
#include "IControls.h"

const int kNumPresets = 1;

enum EParams
{
  kColorMapRange = 0,
  kColorMapContrast,
  kNumParams
};

using namespace iplug;
using namespace igraphics;

class IPlugScopeControl final : public Plugin
{
public:
  static const int BUFFER_SIZE = 512;
  
  IPlugScopeControl(const InstanceInfo& info);

  void OnIdle() override;

#if IPLUG_DSP // http://bit.ly/2S64BDd
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  IBufferSender<2, BUFFER_SIZE, BUFFER_SIZE*2> mSender;
#endif
  
#if IPLUG_EDITOR
  void OnParentWindowResize(int width, int height) override;
#endif
};
