#pragma once

#include "IPlug_include_in_plug_hdr.h"

using namespace iplug;
using namespace igraphics;

class IGraphicsTest : public Plugin
{
public:
  IGraphicsTest(const InstanceInfo& info);
  
  void OnHostSelectedViewConfiguration(int width, int height) override;
  bool OnHostRequestingSupportedViewConfiguration(int width, int height) override;
};
