#pragma once
#include "IPlug_include_in_plug_hdr.h"

class LinuxWebEditor final : public iplug::Plugin
{
public:
  explicit LinuxWebEditor(const iplug::InstanceInfo& info);
  void OnWebContentLoaded() override;
  void OnMessageFromWebView(const char* json) override;
};