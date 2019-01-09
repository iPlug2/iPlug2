#pragma once

#include "IPlug_include_in_plug_hdr.h"

enum EParam
{
  kParamDummy = 0,
  kNumParams
};

enum EControlTags
{
  kCtrlTagNumThings = 0,
  kCtrlTagTestNum
};

class IGraphicsStressTest : public IPlug
{
public:
  IGraphicsStressTest(IPlugInstanceInfo instanceInfo);
  
public:
  int mNumberOfThings = 16;
  int mKindOfThing = 0;
};
