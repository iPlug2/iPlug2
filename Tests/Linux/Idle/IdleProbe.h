#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include <thread>

class LinuxIdleProbe final : public iplug::Plugin
{
public:
  explicit LinuxIdleProbe(const iplug::InstanceInfo& info);
  ~LinuxIdleProbe() override;
  void OnIdle() override;
  void OnUIOpen() override;
  void OnUIClose() override;
  void SendParameterValueFromDelegate(int paramIdx, double value, bool normalized) override;

private:
  const std::thread::id mMainThread = std::this_thread::get_id();
};
