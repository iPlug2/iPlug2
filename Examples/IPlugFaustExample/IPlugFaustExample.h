#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IPlugFaustGen.h"
//#include "IPlugFaust_edit.h"

#define DSP_FILE "/Users/oli/Dev/MyPlugins/Examples/IPlugFaustExample/IPlugFaustExample.dsp"

class IPlugFaustExample : public IPlug
{
public:
  IPlugFaustExample(IPlugInstanceInfo instanceInfo);

  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnReset() override;
  void OnParamChange(int paramIdx) override;
  FAUST_BLOCK(Faust1, mFaustProcessor, DSP_FILE, 1, 1);
};
