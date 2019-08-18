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
  kCtrlTagTestNum,
  kCtrlTagButton1,
  kCtrlTagButton2,
  kCtrlTagButton3,
  kCtrlTagButton4,
  kCtrlTagButton5
};

using namespace iplug;
using namespace igraphics;

class IGraphicsStressTest : public Plugin
{
public:
  IGraphicsStressTest(const InstanceInfo& info);
#if IPLUG_EDITOR
  void LayoutUI(IGraphics* pGraphics) override;
public:
  int mNumberOfThings = 16;
  int mKindOfThing = 0;
#endif
};
