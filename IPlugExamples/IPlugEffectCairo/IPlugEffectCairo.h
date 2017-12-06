#pragma once

#include "IPlug_include_in_plug_hdr.h"

class IPlugEffectCairo : public IPlug
{
public:
  IPlugEffectCairo(IPlugInstanceInfo instanceInfo);
  ~IPlugEffectCairo();

  void Reset() override;
  void OnParamChange(int paramIdx) override;
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames) override;

private:
};
