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

#elif defined OS_WIN

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
#elif defined OS_LINUX
#include <cstdio>
#include <chrono>
#include <condition_variable>
#include <mutex>

// The worker retains this state even if its callback destroys the Timer itself.
struct Timer_impl::State
{
  State(Timer_impl* owner, ITimerFunction func, uint32_t intervalMs)
  : mOwner(owner), mCallback(std::move(func)), mIntervalMs(intervalMs) {}

  Timer_impl* mOwner;
  ITimerFunction mCallback;
  uint32_t mIntervalMs;
  std::mutex mMutex;
  std::condition_variable mWake;
  pthread_t mThread = 0;
  bool mStarted = false;
  bool mRunning = true;
  bool mJoining = false;
  bool mJoined = false;
};

Timer* Timer::Create(ITimerFunction func, uint32_t intervalMs)
{
  auto* pTimer = new Timer_impl(std::move(func), intervalMs);
  if (!pTimer->Start())
  {
    delete pTimer;
    return nullptr;
  }
  return pTimer;
}

Timer_impl::Timer_impl(ITimerFunction func, uint32_t intervalMs)
: mState(std::make_shared<State>(this, std::move(func), intervalMs))
{}

bool Timer_impl::Start()
{
  auto state = mState;
  auto* workerState = new std::shared_ptr<State>(state);
  // Publish the thread handle before the worker can invoke a callback.
  std::lock_guard<std::mutex> lock(state->mMutex);
  const int error = pthread_create(&state->mThread, nullptr, ThreadProc, workerState);
  if (error)
  {
    delete workerState;
    state->mRunning = false;
    fprintf(stderr, "Could not start Linux timer: %s\n", strerror(error));
  }
  else
    state->mStarted = true;
  // Do not access the Timer after releasing the lock: a zero-interval callback
  // may destroy it before Create() returns.
  return state->mStarted;
}

Timer_impl::~Timer_impl()
{
  Stop();
  std::lock_guard<std::mutex> lock(mState->mMutex);
  // External Stop() joins the worker. Self-destruction must let it finish on its own.
  if (mState->mStarted && !mState->mJoining && !mState->mJoined &&
      pthread_equal(mState->mThread, pthread_self()))
    pthread_detach(mState->mThread);
}

void Timer_impl::Stop()
{
  auto state = mState;
  std::unique_lock<std::mutex> lock(state->mMutex);
  state->mRunning = false;
  state->mWake.notify_all();
  if (!state->mStarted || state->mJoined || pthread_equal(state->mThread, pthread_self()))
    return;
  if (state->mJoining)
  {
    state->mWake.wait(lock, [&] { return state->mJoined; });
    return;
  }
  state->mJoining = true;
  lock.unlock();
  pthread_join(state->mThread, nullptr);
  lock.lock();
  state->mJoined = true;
  state->mWake.notify_all();
}

void* Timer_impl::ThreadProc(void* pParam)
{
  auto* workerState = static_cast<std::shared_ptr<State>*>(pParam);
  auto state = std::move(*workerState);
  delete workerState;
  std::unique_lock<std::mutex> lock(state->mMutex);
  while (state->mRunning)
  {
    if (state->mWake.wait_for(lock, std::chrono::milliseconds(state->mIntervalMs),
                             [&] { return !state->mRunning; }))
      break;
    lock.unlock();
    state->mCallback(*state->mOwner);
    lock.lock();
  }
  return nullptr;
}
#endif
