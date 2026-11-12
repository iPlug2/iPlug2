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
    sTimers.Add(pTimer);
    return pTimer;
  }

  ~SWELLTimer() override { Stop(); }

  void Stop() override
  {
    if (mID)
    {
      KillTimer(nullptr, mID);
      sTimers.DeletePtr(this);
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
    for (int i = 0; i < sTimers.GetSize(); ++i)
    {
      auto* pTimer = sTimers.Get(i);
      if (pTimer->mID == id)
      {
        pTimer->mCallback(*pTimer);
        return;
      }
    }
  }

  static WDL_PtrList<SWELLTimer> sTimers;
  ITimerFunction mCallback;
  UINT_PTR mID = 0;
};

inline WDL_PtrList<SWELLTimer> SWELLTimer::sTimers;

END_IPLUG_NAMESPACE
