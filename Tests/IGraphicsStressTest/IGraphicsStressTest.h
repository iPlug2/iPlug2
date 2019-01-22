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
#if IPLUG_EDITOR
  IGraphicsStressTest(IPlugInstanceInfo instanceInfo);
  void LayoutUI(IGraphics* pGraphics) override;
#endif
public:
  int mNumberOfThings = 16;
  int mKindOfThing = 0;
};
