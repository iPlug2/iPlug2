#include "IPlugTaskThread.h"

#include <atomic>
#include <algorithm>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <queue>

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


uint64_t Task::Now()
{
  return GetTimeNow();
}

Task Task::FromMs(uint64_t timeout, uint64_t interval, Task::Callback cb)
{
  return Task { (timeout * 1000) + Now(), interval * 1000, cb };
}

Task Task::FromSec(double timeout, double interval, Task::Callback cb)
{
  return Task { (uint64_t)(timeout * 1000000.0) + Now(), (uint64_t)(interval * 1000000.0), cb };
}

struct TaskImpl
{
  ClockT::time_point time;
  ClockT::duration interval;
  Task::Callback callback;
  TaskID id;

  bool operator<(const TaskImpl& rhs) const
  {
    return time > rhs.time;
  }
};


class IPlugTaskThread_Impl
{
public:
  IPlugTaskThread_Impl(int queueSize)
    // Init next task ID with a new thread ID + 1
    : mNextTaskID((sThreadId.fetch_add(1) << 20) + 1)
    , mCancelQueue(queueSize)
  {
    mRunning.store(true);
    mThread = new std::thread(std::bind(&IPlugTaskThread_Impl::loop, this));
  }

  ~IPlugTaskThread_Impl()
  {
    if (mThread)
    {
      Stop();
      mThread->join();
      delete mThread;
      mThread = nullptr;
    }
  }

  TaskID Push(const Task& t)
  {
    TaskImpl t2;
    t2.time = sTimeEpoch + microseconds(t.time);
    t2.interval = microseconds(t.interval);
    t2.callback = t.callback;
    t2.id = mNextTaskID.fetch_add(1, std::memory_order_seq_cst);

    // Add item and signal task thread that an item has been added
    mSignalLock.lock();
    mQueueIn.push_back(t2);
    mSignalFlag.store(true);
    mSignalCond.notify_one();
    mSignalLock.unlock();
    // Return the TaskID
    return t2.id;
  }

  void Cancel(TaskID id)
  {
    if (id == 0)
      return;
    mSignalLock.lock();
    mCancelQueue.Push(id);
    mSignalLock.unlock();
  }

  void Stop()
  {
    mRunning.store(false);
  }

  void loop()
  {
    printf("iPlug2Loop started\n");
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
      mSignalLock.lock();
      while (mQueueIn.size() > 0)
      {
        TaskImpl t2 = mQueueIn.back();
        mQueueIn.pop_back();
        mTasks.push(t2);
      }
      mSignalLock.unlock();

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
          bool keep = task.callback(ToUsec(now - sTimeEpoch));
          if (task.interval == ClockT::duration::zero())
          {
            keep = false;
          }
          // Rescheduling logic
          if (keep)
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
      {
        MutexLock lck(mSignalLock);
        mSignalFlag.store(false);
        mSignalCond.wait_until(lck, until);
      }
    } // end while(mRunning.load())
    printf("iPlug2Loop ended\n");
  }

  std::thread* mThread;
  std::atomic<bool> mRunning;

  std::vector<TaskImpl> mQueueIn;
  IPlugQueue<TaskID> mCancelQueue;
  
  std::mutex mSignalLock;
  std::condition_variable mSignalCond;
  std::atomic_bool mSignalFlag;

  /** Next task ID */
  std::atomic_uint32_t mNextTaskID;
  /** Tasks sorted by next time they occur. */
  std::priority_queue<TaskImpl> mTasks;

  // To help differentiate task IDs between different threads.
  static std::atomic_uint_fast16_t sThreadId;
};

std::atomic_uint_fast16_t IPlugTaskThread_Impl::sThreadId { 1 };

IPlugTaskThread::IPlugTaskThread(int queueSize)
  : mImpl(new IPlugTaskThread_Impl(queueSize))
{}

IPlugTaskThread::~IPlugTaskThread()
{
  if (mImpl)
  {
    Stop();
    delete mImpl;
    mImpl = nullptr;
  }
}

TaskID IPlugTaskThread::Push(const Task& t)
{
  return mImpl->Push(t);
}

void IPlugTaskThread::Cancel(TaskID id)
{
  return mImpl->Cancel(id);
}

void IPlugTaskThread::Stop()
{
  mImpl->Stop();
}

END_IPLUG_NAMESPACE
