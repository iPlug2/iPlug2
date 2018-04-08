#pragma once

#include "IGraphics_include_in_plug_hdr.h"
#include "IPlugGraphicsDelegate.h"

#define UI_WIDTH 700
#define UI_HEIGHT 700
#define UI_FPS 60

class IGraphicsTest : public IGraphicsDelegate
{
public:
  IGraphicsTest();
  ~ IGraphicsTest() {}

  //IDelegate
  void SetParameterValueFromUI(int paramIdx, double value) override;
  void BeginInformHostOfParamChangeFromUI(int paramIdx) override { };
  void EndInformHostOfParamChangeFromUI(int paramIdx) override { };
};
