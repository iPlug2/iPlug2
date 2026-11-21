/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#pragma once

#include "IPlugTimer.h"
#include "IPlugSWELL.h"
#include <cstdio>

BEGIN_IPLUG_NAMESPACE

/** A Linux timer dispatched by the owning SWELL main loop, never a worker. */
class SWELLTimer final : public Timer
{
public:
  static Timer* Create(ITimerFunction func, uint32_t intervalMs)
  {
    auto* pTimer = new SWELLTimer(func, intervalMs);
    if (!pTimer->mID)
    {
      fprintf(stderr, "Could not create the Linux main-loop timer\n");
      delete pTimer;
      return nullptr;
    }
    Timers().Add(pTimer);
    return pTimer;
  }

  ~SWELLTimer() override { Stop(); }

  void Stop() override
  {
    if (mID)
    {
      KillTimer(nullptr, mID);
      Timers().DeletePtr(this);
      mID = 0;
    }
  }

private:
  SWELLTimer(ITimerFunction func, uint32_t intervalMs)
    : mCallback(func)
  {
    mID = SetTimer(nullptr, 0, intervalMs, TimerProc);
  }

  static void TimerProc(HWND, UINT, UINT_PTR id, DWORD)
  {
    for (int i = 0; i < Timers().GetSize(); ++i)
    {
      auto* pTimer = Timers().Get(i);
      if (pTimer->mID == id)
      {
        pTimer->mCallback(*pTimer);
        return;
      }
    }
  }

  /** The list is never destroyed, so timers owned by other statics can still be stopped during static destruction. */
  static WDL_PtrList<SWELLTimer>& Timers()
  {
    static auto* sTimers = new WDL_PtrList<SWELLTimer>();
    return *sTimers;
  }

  ITimerFunction mCallback;
  UINT_PTR mID = 0;
};

END_IPLUG_NAMESPACE
