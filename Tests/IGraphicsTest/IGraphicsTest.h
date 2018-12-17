#pragma once

#include "IPlug_include_in_plug_hdr.h"

class IGraphicsTest : public IPlug
{
public:
  IGraphicsTest(IPlugInstanceInfo instanceInfo);
  
  void OnHostSelectedViewConfiguration(int width, int height) override;
  bool OnHostRequestingSupportedViewConfiguration(int width, int height) override;
};
