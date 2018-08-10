#pragma once

#include "IGraphics_include_in_plug_hdr.h"
#include "IGraphicsEditorDelegate.h"
#include "config.h"

class IGraphicsTest : public IGEditorDelegate
{
public:
  IGraphicsTest();
  ~IGraphicsTest() {}

  //IGEditorDelegate
  void SendParameterValueFromUI(int paramIdx, double value) override { };
  void BeginInformHostOfParamChangeFromUI(int paramIdx) override { };
  void EndInformHostOfParamChangeFromUI(int paramIdx) override { }
  void CreateUI(IGraphics* pGraphics) override;
};
