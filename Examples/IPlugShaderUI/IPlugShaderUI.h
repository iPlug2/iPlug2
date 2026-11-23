#pragma once

#include "IPlug_include_in_plug_hdr.h"

using namespace iplug;
using namespace igraphics;

class IPlugShaderUI final : public Plugin
{
public:
  IPlugShaderUI(const InstanceInfo& info);
  
  void OnHostSelectedViewConfiguration(int width, int height) override;
  bool OnHostRequestingSupportedViewConfiguration(int width, int height) override;
  void OnParentWindowResize(int width, int height) override;
};
