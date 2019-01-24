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
#if IPLUG_EDITOR
  void LayoutUI(IGraphics* pGraphics) override;
public:
  int mNumberOfThings = 16;
  int mKindOfThing = 0;
#endif
};
