/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/


#pragma once

/** @file This file includes classes for implementing timers - in order to get a regular callback on the main thread
 * The interface is partially based on the api of Steinberg's timer.cpp from the VST3_SDK for compatibility,
 * rewritten using SWELL: base/source/timer.cpp, so thanks to them */

#include <cstring>
#include <stdint.h>
#include <cstring>
#include <functional>
#include "ptrlist.h"

#include "IPlugPlatform.h"

struct Timer;

typedef std::function<void(Timer& t)> ITimerFunction;

#if defined OS_WEB
class Timer
{
public:
  static Timer* Create(ITimerFunction func, uint32_t intervalMs)
  {
    return new Timer();
  }
  
  void Stop()
  {
  }
};

#else

#if defined OS_MAC
#include "swell.h"
#elif defined OS_IOS
typedef bool BOOL;
typedef unsigned int UINT;
typedef unsigned int DWORD;
typedef void* HWND;
typedef void (*TIMERPROC)(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
#define CALLBACK
UINT_PTR SetTimer(HWND hwnd, UINT_PTR timerid, UINT rate, TIMERPROC tProc);
BOOL KillTimer(HWND hwnd, UINT_PTR timerid);
#endif

struct Timer
{
  virtual ~Timer() {};
  static Timer* Create(ITimerFunction func, uint32_t intervalMs);
  virtual void Stop() = 0;
  UINT_PTR ID = 0;
};

class Timer_impl : public Timer
{
public:
  Timer_impl(ITimerFunction func, uint32_t intervalMs)
  : mTimerFunc(func)
  , mIntervalMs(intervalMs)

  {
    ID = SetTimer(0, 0, intervalMs, TimerProc);
    
    if(ID)
     AddTimer(this);
  }
  
  ~Timer_impl()
  {
    Stop();
  }
  
  void Stop() override
  {
    if (!ID)
      return;
    
    KillTimer(0, ID);
    RemoveTimer(this);
    ID = 0;
  }
  
  void AddTimer(Timer_impl* pTimer)
  {
    sTimers.Add(pTimer);
//    DBGMSG("Add NTimers % i\n", sTimers.GetSize());
  }
  
  void RemoveTimer(Timer_impl* pTimer)
  {
    sTimers.DeletePtr(pTimer);
//    DBGMSG("Remove NTimers % i\n", sTimers.GetSize());
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
  static WDL_PtrList<Timer_impl> sTimers;
  ITimerFunction mTimerFunc;
  uint32_t mIntervalMs;
};

#endif
