#pragma once

#include "IPlug_include_in_plug_hdr.h"

using namespace iplug;
using namespace igraphics;

class IGraphicsTest : public Plugin
{
public:
  IGraphicsTest(IPlugInstanceInfo instanceInfo);
  
  void OnHostSelectedViewConfiguration(int width, int height) override;
  bool OnHostRequestingSupportedViewConfiguration(int width, int height) override;
};
