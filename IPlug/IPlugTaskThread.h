#pragma once

#include <functional>
#include <cstdint>
#include <vector>
#include "mutex.h"
#include "IPlugPlatform.h"
#include "IPlugQueue.h"

BEGIN_IPLUG_NAMESPACE

/** Task ID for cancelling the task. */
typedef uint32_t TaskID;

struct Task
{
  /** Callback for tasks. If the callback returns false the task is cancelled. */
  using Callback = std::function<bool(uint64_t time)>;

  /** Returns the current (absolute) timer value in microseconds. */
  static uint64_t Now();

  /** Create a Task with values in milliseconds.
   * @param timeout Milliseconds from now until the callback will be called
   * @param interval Milliseconds between callbacks (0 = only once)
   * @param callback Callback for the task
   */
  static Task FromMs(uint64_t timeout, uint64_t interval, Callback callback);

  /** Create a Task with values in seconds.
   * @param timeout Seconds from now until the callback will be called
   * @param interval Seconds between callbacks (0 = only once)
   * @param callback Callback for the task
   */
  static Task FromSec(double timeout, double interval, Callback callback);

  // Timeout value in microseconds, absolute
  uint64_t time;
  // Interval value in microseconds. 0 means occur only once.
  uint64_t interval;
  // Callback
  Callback callback;
};

class IPlugTaskThread_Impl;
class IPlugTaskThread
{
public:
  IPlugTaskThread(int queueSize=32);
  ~IPlugTaskThread();

  /** Push a new task. */
  TaskID Push(Task&& t);

  /** Add a one-shot task. */
  void AddOnce(Task::Callback&& callback);

  /** Cancel the task with the given task ID. */
  void Cancel(TaskID id);
  /** Stop the task thread. */
  void Stop();

  /**
   * Get the global \c IPlugTaskThread instance. Users may create new task threads,
   * but this one will always exist, at least on non-web platforms.
   */
  static IPlugTaskThread* instance();

private:
  IPlugTaskThread_Impl* mImpl;
};

/**
 * A very simple task list that is thread-safe.
 * Items may be added from any thread, and will be processed in order
 * in the "receiver" thread.
 */
template<class... FnArgs>
class ThreadSafeCallList
{
public:
  typedef std::function<void(FnArgs...)> functor_t;

private:
  /// @brief List of tasks
  std::vector<functor_t> mTasks;
  /// @brief Lock
  WDL_Mutex mLock;

public:

  void Add(functor_t&& task)
  {
    mLock.Enter();
    mTasks.push_back(std::move(task));
    mLock.Leave();
  }

  void Process(FnArgs... args)
  {
    // create a new task list and swap it with our pending list
    // so we can safely work on the pending list without holding the lock.
    std::vector<functor_t> taskList;
    mLock.Enter();
    std::swap(taskList, mTasks);
    mLock.Leave();

    // run all pending tasks, they will get deleted when taskList goes out of scope
    for (size_t i = 0; i < taskList.size(); i++) {
      taskList[i](args...);
    }
  }
};



END_IPLUG_NAMESPACE
