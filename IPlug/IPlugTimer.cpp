/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

/**
 * @file
 * @brief Timer implementation
 */

#include "IPlugTimer.h"
#include "IPlugTaskThread.h"

using namespace iplug;

#if defined OS_MAC || defined OS_IOS

Timer* Timer::Create(ITimerFunction func, uint32_t intervalMs)
{
  return new Timer_impl(func, intervalMs);
}

Timer_impl::Timer_impl(ITimerFunction func, uint32_t intervalMs)
: mTimerFunc(func)
, mIntervalMs(intervalMs)
{
  CFRunLoopTimerContext context;
  context.version = 0;
  context.info = this;
  context.retain = nullptr;
  context.release = nullptr;
  context.copyDescription = nullptr;
  CFTimeInterval interval = intervalMs / 1000.0;
  CFRunLoopRef runLoop = CFRunLoopGetMain();
  mOSTimer = CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent(), interval, 0, 0, TimerProc, &context);
  CFRunLoopAddTimer(runLoop, mOSTimer, kCFRunLoopCommonModes);
}

Timer_impl::~Timer_impl()
{
  Stop();
}

void Timer_impl::Stop()
{
  if (mOSTimer)
  {
    CFRunLoopTimerInvalidate(mOSTimer);
    CFRelease(mOSTimer);
    mOSTimer = nullptr;
  }
}

void Timer_impl::TimerProc(CFRunLoopTimerRef timer, void *info)
{
  Timer_impl* itimer = (Timer_impl*) info;
  itimer->mTimerFunc(*itimer);
}

#elif defined OS_WIN || (defined OS_LINUX && defined APP_API)

Timer* Timer::Create(ITimerFunction func, uint32_t intervalMs)
{
  return new Timer_impl(func, intervalMs);
}

WDL_Mutex Timer_impl::sMutex;
WDL_PtrList<Timer_impl> Timer_impl::sTimers;

Timer_impl::Timer_impl(ITimerFunction func, uint32_t intervalMs)
: mTimerFunc(func)
, mIntervalMs(intervalMs)

{
  ID = SetTimer(0, 0, intervalMs, TimerProc); //TODO: timer ID correct?
  
  if (ID)
  {
    WDL_MutexLock lock(&sMutex);
    sTimers.Add(this);
  }
}

Timer_impl::~Timer_impl()
{
  Stop();
}

void Timer_impl::Stop()
{
  if (ID)
  {
    KillTimer(0, ID);
    WDL_MutexLock lock(&sMutex);
    sTimers.DeletePtr(this);
    ID = 0;
  }
}

void CALLBACK Timer_impl::TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
  WDL_MutexLock lock(&sMutex);

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
#elif defined OS_LINUX

#if IPLUG_EDITOR
static IPlugTaskThread sMainThread;
#endif

WDL_Mutex Timer_impl::sMutex;
WDL_PtrList<Timer_impl> Timer_impl::sTimers;

Timer* Timer::Create(ITimerFunction func, uint32_t intervalMs)
{
  return new Timer_impl(func, intervalMs);
}

#if IPLUG_EDITOR
Timer_impl::Timer_impl(ITimerFunction func, uint32_t intervalMs)
: mTimerFunc(func)
, mIntervalMs(intervalMs)
, mID(0)
{
  auto cb = [&](uint64_t time) -> bool { mTimerFunc(*this); return true; };
  Task task = Task::FromMs(intervalMs, intervalMs, cb);
  mID = sMainThread.Push(task);
}

Timer_impl::~Timer_impl()
{
  Stop();
}

void Timer_impl::Stop()
{
  if (mID)
  {
    sMainThread.Cancel(mID);
    mID = 0;
  }
}

#else // No IPLUG_EDITOR
void Timer_impl::NotifyCallback(union sigval v)
{
  Timer_impl* timer = (Timer_impl*)v.sival_ptr;
  timer->mTimerFunc(*timer);
}

Timer_impl::Timer_impl(ITimerFunction func, uint32_t intervalMs)
: mTimerFunc(func)
, mIntervalMs(intervalMs)
, mID(0)
{
  int err;
  struct sigevent evt;
  evt.sigev_notify = SIGEV_THREAD;
  evt.sigev_signo = 0;
  evt.sigev_value.sival_ptr = this;
  evt.sigev_notify_function = &Timer_impl::NotifyCallback;
  evt.sigev_notify_attributes = nullptr;

  err = timer_create(CLOCK_MONOTONIC, &evt, &mID);
  if (err != 0)
  {
    mID = 0;
    return;
  }

  auto MakeTimespec = [](uint32_t ms) -> struct timespec {
    struct timespec t = { ms / 1000, (ms % 1000) * 1000000 }; return t;
  };

  struct itimerspec ntime;
  ntime.it_value = MakeTimespec(intervalMs);
  ntime.it_interval = MakeTimespec(intervalMs);
  err = timer_settime(mID, 0, &ntime, nullptr);
  if (err != 0)
  {
    timer_delete(mID);
    mID = 0;
    return;
  }

  WDL_MutexLock lock(&sMutex);
  sTimers.Add(this);
}

Timer_impl::~Timer_impl()
{
  Stop();
}

void Timer_impl::Stop()
{
  if (mID)
  {
    // NB. timer_delete returns an error code. Should we check it?
    timer_delete(mID);
    WDL_MutexLock lock(&sMutex);
    sTimers.DeletePtr(this);
    mID = 0;
  }
}
#endif // IPLUG_EDITOR

#elif defined OS_WEB
Timer* Timer::Create(ITimerFunction func, uint32_t intervalMs)
{
  return new Timer_impl(func, intervalMs);
}

Timer_impl::Timer_impl(ITimerFunction func, uint32_t intervalMs)
: mTimerFunc(func)
{
  ID = emscripten_set_interval(TimerProc, intervalMs, this);
}

Timer_impl::~Timer_impl()
{
  Stop();
}

void Timer_impl::Stop()
{
  emscripten_clear_interval(ID);
}

void Timer_impl::TimerProc(void* userData)
{
  Timer_impl* itimer = (Timer_impl*) userData;
  itimer->mTimerFunc(*itimer);
}
#endif
