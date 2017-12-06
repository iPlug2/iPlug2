#pragma once

#include "IPlug_include_in_plug_hdr.h"

class IPlugEffect : public IPlug
{
public:
  IPlugEffect(IPlugInstanceInfo instanceInfo);
  ~IPlugEffect();

  void Reset() override;
  void OnParamChange(int paramIdx) override;
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames) override;

private:
};
