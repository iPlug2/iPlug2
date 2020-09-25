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

#ifdef OS_LINUX
#include <thread>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <vector>
#include <pthread.h>
#endif

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

using TaskTime = std::chrono::steady_clock::time_point;
using TaskDuration = std::chrono::microseconds;
inline static TaskTime GetTimeNow()
{
  return std::chrono::steady_clock::now();
}

struct Task
{
  TaskTime time;
  TaskDuration interval;
  Timer_impl* timer;
};

using MutexLock = std::unique_lock<std::mutex>;

class MainThread
{
public:
  MainThread()
    : mRunning(true)
    , mThread(std::bind(&MainThread::loop, this))
  {}

  ~MainThread()
  {
    Stop();
    mThread.join();
  }

  void PushTask(Task t)
  {
    MutexLock lck (mLock);
    mTasks.push_back(t);
    mCond.notify_one();
  }

  void Stop()
  {
    MutexLock lck (mLock);
    mRunning = false;
    mCond.notify_one();
  }

private:
  void loop()
  {
#ifdef OS_LINUX
    int err = pthread_setname_np(pthread_self(), "iPlug2Loop");
#endif

    std::chrono::milliseconds maxTimeout (100);

    MutexLock lck (mLock);
    while (mRunning)
    {
      TaskTime now = GetTimeNow();
      TaskTime until = now + maxTimeout;
      for (int i = mTasks.size() - 1; i >= 0; i--) {
        Task& task = mTasks[i];
        // If the time has passed, execute the task 
        // and replace it with the task at the end
        if (task.time <= now)
        {
          task.timer->Execute();
          mTasks[i] = mTasks.back();
          mTasks.pop_back();
        }
        else if (task.time < until)
        {
          until = task.time;
        }
      }
      mCond.wait_until(lck, until);
    }
  }

private:
  std::thread mThread;
  std::mutex mLock;
  std::condition_variable mCond;
  std::vector<Task> mTasks;
  bool mRunning;
};

static MainThread sMainThread;
WDL_Mutex Timer_impl::sMutex;
WDL_PtrList<Timer_impl> Timer_impl::sTimers;

Timer* Timer::Create(ITimerFunction func, uint32_t intervalMs)
{
  return new Timer_impl(func, intervalMs);
}

void Timer_impl::NotifyCallback(union sigval v)
{
  Timer_impl* timer = (Timer_impl*)v.sival_ptr;
  timer->Execute();
  //sMainThread.PushTask(Task { GetTimeNow(), std::chrono::seconds(0), timer });
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

void Timer_impl::Execute()
{
  mTimerFunc(*this);
}

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
