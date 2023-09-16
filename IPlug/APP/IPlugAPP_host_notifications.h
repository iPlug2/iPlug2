#pragma once

#include <functional>

#include "IPlugPlatform.h"

BEGIN_IPLUG_NAMESPACE

class NotificationListener
{
public:
  NotificationListener();
  ~NotificationListener();
  
private:
  void* mpDevicesChangedID = nullptr;
  void* mpSampleRateChangedID = nullptr;
};

END_IPLUG_NAMESPACE
