/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#pragma once

#if defined OS_LINUX
#include <cstdio>
#include <functional>
#include "base/source/fobject.h"
#include "pluginterfaces/gui/iplugview.h"

BEGIN_IPLUG_NAMESPACE

class IPlugVST3Timer final : public Steinberg::FObject, public Steinberg::Linux::ITimerHandler
{
public:
  explicit IPlugVST3Timer(std::function<void()> callback)
  : mCallback(callback)
  {}

  ~IPlugVST3Timer() override { Stop(); }

  bool Start(Steinberg::FUnknown* context)
  {
    if (mActive)
      return true;
    mRunLoop = Steinberg::FUnknownPtr<Steinberg::Linux::IRunLoop>(context);
    if (!mRunLoop)
    {
      fprintf(stderr, "VST3 idle dispatch requires a host IRunLoop; waiting for a frame run loop\n");
      return false;
    }
    if (mRunLoop->registerTimer(this, IDLE_TIMER_RATE) != Steinberg::kResultOk)
    {
      fprintf(stderr, "The VST3 host rejected the Linux idle timer\n");
      mRunLoop = nullptr;
      return false;
    }
    mActive = true;
    return true;
  }

  void Stop()
  {
    if (mActive)
    {
      mActive = false;
      if (mRunLoop->unregisterTimer(this) != Steinberg::kResultOk)
        fprintf(stderr, "Could not unregister the VST3 idle timer\n");
    }
    mRunLoop = nullptr;
  }

  void PLUGIN_API onTimer() override
  {
    if (mActive)
      mCallback();
  }

  Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID iid, void** object) override
  {
    QUERY_INTERFACE(iid, object, Steinberg::Linux::ITimerHandler::iid, Steinberg::Linux::ITimerHandler)
    return FObject::queryInterface(iid, object);
  }

  Steinberg::uint32 PLUGIN_API addRef() override { return FObject::addRef(); }
  Steinberg::uint32 PLUGIN_API release() override { return FObject::release(); }

private:
  std::function<void()> mCallback;
  Steinberg::IPtr<Steinberg::Linux::IRunLoop> mRunLoop;
  bool mActive = false;
};

END_IPLUG_NAMESPACE
#endif
