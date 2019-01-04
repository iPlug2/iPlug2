/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/


#pragma once

/** @file
 * @brief This file includes classes for implementing timers - in order to get a regular callback on the main thread
 * The interface is partially based on the api of Steinberg's timer.cpp from the VST3_SDK for compatibility,
 * rewritten using SWELL: base/source/timer.cpp, so thanks to them */

#include <cstring>
#include <stdint.h>
#include <cstring>
#include <functional>
#include "ptrlist.h"

#include "IPlugPlatform.h"

#if defined OS_WEB
/** Base class for timer */
struct Timer
{
  static Timer* Create(ITimerFunction func, uint32_t intervalMs)
  {
    return new Timer();
  }
  
  void Stop()
  {
  }
};
#else
/** Base class for timer */
struct Timer
{
  typedef std::function<void(Timer& t)> ITimerFunction;

  static Timer* Create(ITimerFunction func, uint32_t intervalMs);
  virtual ~Timer() {};
  virtual void Stop() = 0;
};
#endif

#if defined OS_MAC || defined OS_IOS

#include <CoreFoundation/CoreFoundation.h>

class Timer_impl : public Timer
{
public:
  
  Timer_impl(ITimerFunction func, uint32_t intervalMs);
  ~Timer_impl();
  
  void Stop() override;
  static void TimerProc(CFRunLoopTimerRef timer, void *info);
  
private:
  CFRunLoopTimerRef mOSTimer;
  ITimerFunction mTimerFunc;
  uint32_t mIntervalMs;
};
#elif defined OS_WIN
class Timer_impl : public Timer
{
public:
  Timer_impl(ITimerFunction func, uint32_t intervalMs)
  : mTimerFunc(func)
  , mIntervalMs(intervalMs)

  {
    ID = SetTimer(0, 0, intervalMs, TimerProc);
    
    if (ID)
     sTimers.Add(this);
  }
  
  ~Timer_impl()
  {
    Stop();
  }
  
  void Stop() override
  {
    if (ID)
    {
      KillTimer(0, ID);
      sTimers.DeletePtr(this);
      ID = 0;
    }
  }
  
  static void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
  {
    for (auto i = 0; i < sTimers.GetSize(); i++)
    {
      Timer_impl* pTimer = sTimers.Get(i);
      
      if (pTimer->ID == idEvent)
      {
        pTimer->mTimerFunc(*pTimer);
        return;
      }
    }
  }
  
private:
  static WDL_Mutex sMutex;
  static WDL_PtrList<Timer_impl> sTimers;
  UINT_PTR ID = 0;
  ITimerFunction mTimerFunc;
  uint32_t mIntervalMs;
};
#elif
  #error NOT IMPLEMENTED
#endif
