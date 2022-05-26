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
 * rewritten using SWELL: base/source/timer.cpp, so thanks to them
*/

#include <cstring>
#include <stdint.h>
#include <cstring>
#include <cmath>
#include <functional>
#include "ptrlist.h"
#include "mutex.h"

#include "IPlugPlatform.h"

#if defined OS_LINUX && defined APP_API
#include "swell.h"
#elif defined OS_LINUX
#include <signal.h>
#include <time.h>
#endif

#if defined OS_MAC || defined OS_IOS
#include <CoreFoundation/CoreFoundation.h>
#elif defined OS_WEB
#include <emscripten/html5.h>
#endif

BEGIN_IPLUG_NAMESPACE

/** Base class for timer */
struct Timer
{
  Timer() = default;
  Timer(const Timer&) = delete;
  Timer& operator=(const Timer&) = delete;
  
  using ITimerFunction = std::function<void(Timer& t)>;

  static Timer* Create(ITimerFunction func, uint32_t intervalMs);

  virtual ~Timer() {};
  virtual void Stop() = 0;
};

#if defined OS_MAC || defined OS_IOS

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
#elif defined OS_WIN || (defined OS_LINUX && defined APP_API)
class Timer_impl : public Timer
{
public:
  Timer_impl(ITimerFunction func, uint32_t intervalMs);
  ~Timer_impl();
  void Stop() override;
  static void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
  
private:
  static WDL_Mutex sMutex;
  static WDL_PtrList<Timer_impl> sTimers;

  UINT_PTR ID = 0;
  ITimerFunction mTimerFunc;
  uint32_t mIntervalMs;
};
#elif defined OS_LINUX
// other API on Linux do not support unrelated timers
class Timer_impl : public Timer
{
public:
  Timer_impl(ITimerFunction func, uint32_t intervalMs);
  ~Timer_impl();

  void Stop() override;
  static void NotifyCallback(union sigval v);


private:
  static WDL_Mutex sMutex;
  static WDL_PtrList<Timer_impl> sTimers;

#if IPLUG_EDITOR
  uint32_t mID;
#else
  timer_t mID;
#endif
  ITimerFunction mTimerFunc;
  uint32_t mIntervalMs;
};

#elif defined OS_WEB
class Timer_impl : public Timer
{
public:
  Timer_impl(ITimerFunction func, uint32_t intervalMs);
  ~Timer_impl();
  void Stop() override;
  static void TimerProc(void *userData);
  
private:
  long ID = 0;
  ITimerFunction mTimerFunc;
};
#else
  #error NOT IMPLEMENTED
#endif

END_IPLUG_NAMESPACE
