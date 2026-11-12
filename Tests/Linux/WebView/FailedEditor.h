#pragma once

#include "IPlug_include_in_plug_hdr.h"

class LinuxFailedEditor final : public iplug::Plugin
{
public:
  explicit LinuxFailedEditor(const iplug::InstanceInfo& info);

  void* OpenWindow(void*) override { return nullptr; }
};
