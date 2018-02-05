#pragma once

#include "IPlug_include_in_plug_hdr.h"

const int kNumPrograms = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

enum ELayout
{
  kWidth = 300,
  kHeight = 300,
  
  kTextX = 10,
  kTextY = 10,
  kGainX = 100,
  kGainY = 100
};

class IPlugEffect : public IPlug
{
public:
  IPlugEffect(IPlugInstanceInfo instanceInfo);
//  ~IPlugEffect();
//   void OnReset() override;
//  void OnParamChange(int paramIdx) override;
  void ProcessBlock(double** inputs, double** outputs, int nFrames) override;
  
//  void OnActivate(bool activate) override
//  {
//    DBGMSG("number of inputs connected %i\n", NInChansConnected());
//    DBGMSG("number of outputs connected %i\n", NOutChansConnected());
//  }
};
