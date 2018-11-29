#pragma once

#include "IPlug_include_in_plug_hdr.h"

class IGraphicsTest : public IPlug
{
public:
  IGraphicsTest(IPlugInstanceInfo instanceInfo);
  
  IGraphics* CreateGraphics() override;
  void LayoutUI(IGraphics* pGraphics) override;
};
