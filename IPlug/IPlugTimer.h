#pragma once

#include <cstring>
#include <stdint.h>
#include "ptrlist.h"

#include "IPlugPlatform.h"

#ifndef OS_WEB
#include "swell.h"
#else
typedef uintptr_t UINT_PTR;
typedef void* HWND;
typedef signed char BOOL;
typedef unsigned int DWORD;
typedef unsigned int UINT;
#define CALLBACK
typedef void (*TIMERPROC)(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
static UINT_PTR SetTimer(HWND hwnd, UINT_PTR timerid, UINT rate, TIMERPROC tProc) { return 0; }
static BOOL KillTimer(HWND hwnd, UINT_PTR timerid) { return 0; }
#endif

/**
 * @file This file includes classes for implementing timers - in order to get a regular callback on the message thread
 * The code is based on the api of Steinberg's timer.cpp from the VST3_SDK, rewritten using SWELL: base/source/timer.cpp, so thanks to them
 *
 */

class Timer;

class ITimerCallback
{
public:
  virtual ~ITimerCallback() {}
  virtual void OnTimer(Timer& t) = 0;
};

struct Timer
{
  virtual ~Timer() {};
  static Timer* Create(ITimerCallback& callback, uint32_t intervalMs);
  virtual void Stop() = 0;
  UINT_PTR ID = 0;
};

class Timer_impl : public Timer
{
public:
  Timer_impl(ITimerCallback& callback, uint32_t intervalMs)
  : mCallBackClass(callback)
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
        pTimer->mCallBackClass.OnTimer(*pTimer);
        return;
      }
    }
  }
  
private:
  static WDL_PtrList<Timer_impl> sTimers;
  ITimerCallback& mCallBackClass;
};
