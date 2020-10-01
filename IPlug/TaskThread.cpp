
#include <atomic>
#include <algorithm>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <queue>
#include <functional>
#include "IPlugPlatform.h"
#include "IPlugQueue.h"

#ifdef OS_LINUX
#include <pthread.h>
#endif

using namespace std::chrono;

BEGIN_IPLUG_NAMESPACE

using ClockT = std::chrono::steady_clock;
using MutexLock = std::unique_lock<std::mutex>;

template<typename T>
inline static uint64_t ToUsec(T duration)
{
  return (uint64_t)duration_cast<microseconds>(duration).count();
}

static ClockT::time_point sTimeEpoch = ClockT::now();
inline static uint64_t GetTimeNow()
{
  return ToUsec(ClockT::now() - sTimeEpoch);
}

#if !defined(ATOMIC_BOOL_LOCK_FREE)
#error Cannot implement SpinLock on this platform
#endif

class SpinLock
{
public:
  SpinLock();
  
  void Lock();
  void TimedLock(uint64_t timeoutUs);
  void Unlock();

private:
  inline void WaitForLockFree(uint64_t timeoutUs);

  std::atomic_bool mFlag;
};

void SpinLock::TimedLock(uint64_t timeoutUs)
{
  bool prev = true;

  int i = 0;


  while ((i++) < 16 && mFlag.)
  {
    prev = mFlag.exchange()
    prev = mFlag.test_and_set();
  }
  while ((i++) < )
  for (int i = 0; prev; i++)
  {
    if (i < )
  }
  while (prev)
  {

  }
}

#if defined(_MSC_VER)
  #define ALWAYS_INLINE __forceinline
  #if defined(_ARM64_)
    #define YieldCpu asm("yield")
  #elif defined(_X86_) || defined(_AMD64_)
    #define YieldCpu asm("pause")
  #else
    #define YieldCpu {}
  #endif
#elif defined(__GNUC__) || defined(__clang__)
  #define ALWAYS_INLINE __attribute__((always_inline))
  #if defined(__aarch64__)
    #define YieldCpu asm("yield")
  #elif defined(__i386__) || defined(__x86_64__)
    #define YieldCpu asm("pause")
  #else
    #define YieldCpu {}
  #endif
#endif

void SpinLock::WaitForLockFree(uint64_t timeoutUs)
{
  static const int BACKOFF_ITERS[] = { 64, 256, };

  int i = 0;
  while ((i++) < BACKOFF_ITERS[0] && mFlag.load(std::memory_order_relaxed))
  {}
  // Timeout check
  if (GetTimeNow() >= timeoutUs)
  { return; }

  i = 0;
  while (i < BACKOFF_ITERS[1] && mFlag.load(std::memory_order_relaxed))
  {
    for (int j = 0; j < 4; j++)
    {
      YieldCpu;
    }
    i += 4;
    // Timeout check
    if (GetTimeNow() >= timeoutUs)
    { return; }
  }
  i = 0;
  while (i < BACKOFF_ITERS[2] && mFlag.load(std::memory_order_relaxed))
  {
    
  }
}

/** Task ID for cancelling the task. */
typedef uint32_t TaskID;

struct Task
{
  /** Callback for tasks. If the callback returns false the task is cancelled. */
  typedef std::function<bool(uint64_t time)> Callback;

  static Task FromMs(uint64_t timeout, uint64_t interval, Callback callback);
  static Task FromSec(double timeout, double interval, Callback callback);

  // Timeout value in microseconds
  uint64_t time;
  // Interval value in microseconds
  uint64_t interval;
  // Callback 
  Callback callback;
};

struct TaskImpl
{
  ClockT::time_point time;
  ClockT::duration interval;
  Task::Callback callback;
  TaskID id;

  bool operator<(const TaskImpl& rhs)
  {
    return time > rhs.time;
  }
};

class TaskThread
{
public:
  TaskThread(int queueSize=32);
  ~TaskThread();

  /** Push a new task. */
  TaskID Push(Task t);
  /** Cancel the task with the given task ID. */
  void Cancel(TaskID id);
  void Stop();

private:
  void loop();

  std::thread* mThread = nullptr;
  std::atomic<bool> mRunning;

  std::mutex mQueueLock;
  IPlugQueue<TaskImpl> mQueueIn;
  IPlugQueue<TaskID> mCancelQueue;
  
  std::mutex mSignalLock;
  std::condition_variable mSignalCond;
  std::atomic<bool> mSignalFlag;

  std::atomic<uint32_t> mNextTaskID;
  std::priority_queue<TaskImpl> mTasks;
};

TaskThread::TaskThread(int queueSize)
  : mNextTaskID(0)
  , mQueueIn(queueSize)
  , mCancelQueue(queueSize)
{
  mThread = new std::thread(std::bind(&TaskThread::loop, this));
}

TaskThread::~TaskThread()
{
  if (mThread)
  {
    delete mThread;
    mThread = nullptr;
  }
}

TaskID TaskThread::Push(Task t)
{
  TaskImpl t2;
  t2.time = t.time;
  t2.interval = t.interval;
  t2.callback = t.callback;
  t2.id = mNextTaskID.fetch_add(1, std::memory_order_seq_cst);

  // Add the new item to the queue
  mQueueLock.lock();
  mQueueIn.Push(t2);
  mQueueLock.unlock();
  // Signal the task thread that an item has been added
  mSignalLock.lock();
  mSignalFlag.store(true);
  mSignalCond.notify_one();
  mSignalLock.unlock();
  // Return the TaskID
  return t2.id;
}

void TaskThread::Cancel(TaskID id)
{

}

void TaskThread::loop()
{
#ifdef OS_LINUX
  int err = pthread_setname_np(pthread_self(), "iPlug2Loop");
#endif

  std::vector<TaskID> cancelList;
  ClockT::duration maxTimeout = milliseconds(100);
  ClockT::time_point now;
  ClockT::time_point until;

  while (mRunning.load())
  {
    now = ClockT::now();
    until = now + maxTimeout;

    // Grab new tasks
    while (mQueueIn.ElementsAvailable())
    {
      TaskImpl t;
      if (mQueueIn.Pop(t))
      {
        mTasks.push(t);
      }
    }

    // Cancel any cancelled tasks
    while (mCancelQueue.ElementsAvailable())
    {
      TaskID id;
      if (mCancelQueue.Pop(id))
      {
        cancelList.push_back(id);
      }
    }

    // Process tasks until either we're out of tasks
    // or we hit one that is later than now.
    while (mTasks.size() > 0)
    {
      TaskImpl task;
      
      task = mTasks.top();
      // Determine if we are supposed to cancel this task
      auto cancelIter = std::find(cancelList.begin(), cancelList.end(), task.id);
      if (cancelIter != cancelList.end())
      {
        mTasks.pop();
        cancelList.erase(cancelIter);
        continue;
      }
      // Process the task
      if (task.time <= now)
      {
        mTasks.pop();
        bool remove = task.callback(ToUsec(now - sTimeEpoch));
        if (task.interval == ClockT::duration::zero())
        {
          remove = true;
        }
        // Rescheduling logic
        if (!remove)
        {
          task.time = now + task.interval;
          mTasks.push(task);
        }
      }
      else
      {
        // This task is in the future, break the loop.
        until = task.time;
        break;
      }
    }

    // Wait for timeout or signal
    MutexLock lck (mSignalLock);
    mSignalFlag.store(false);
    do
    {
      mSignalCond.wait_until(lck, until);
    }
    while (ClockT::now() < until && !mSignalFlag.load());
  } // end while(mRunning.load())
}


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

  }

private:
  std::thread mThread;
  std::mutex mLock;
  std::condition_variable mCond;
  std::vector<Task> mTasks;
  bool mRunning;
};

END_IPLUG_NAMESPACE
