#pragma once

#include <functional>
#include <cstdint>
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
  TaskID Push(const Task& t);
  /** Cancel the task with the given task ID. */
  void Cancel(TaskID id);
  /** Stop the task thread. */
  void Stop();

private:
  IPlugTaskThread_Impl* mImpl;
};

END_IPLUG_NAMESPACE
