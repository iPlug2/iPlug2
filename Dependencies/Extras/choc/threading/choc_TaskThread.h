//
//    ██████ ██   ██  ██████   ██████
//   ██      ██   ██ ██    ██ ██            ** Classy Header-Only Classes **
//   ██      ███████ ██    ██ ██
//   ██      ██   ██ ██    ██ ██           https://github.com/Tracktion/choc
//    ██████ ██   ██  ██████   ██████
//
//   CHOC is (C)2022 Tracktion Corporation, and is offered under the terms of the ISC license:
//
//   Permission to use, copy, modify, and/or distribute this software for any purpose with or
//   without fee is hereby granted, provided that the above copyright notice and this permission
//   notice appear in all copies. THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
//   AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
//   CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
//   WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
//   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#ifndef CHOC_TASKTHREAD_HEADER_INCLUDED
#define CHOC_TASKTHREAD_HEADER_INCLUDED

#include <functional>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <mutex>
#include "../platform/choc_Assert.h"

namespace choc::threading
{

//==============================================================================
/**
    Manages a thread which will invoke a given callback function, either
    repeatedly at a given time-interval, or in response to another
    thread calling TaskThread::trigger().

    It's quite common to need a thread which performs a background task repeatedly,
    with a period of sleeping in between. But the standard library makes it quite a
    palaver to do this in a way that lets you interrupt the sleep to either retrigger
    the task immediately, or to destroy the thread. This class makes that job
    nice and easy.
*/
struct TaskThread
{
    TaskThread();
    ~TaskThread();

    /// Starts the thread running with a given interval and task function.
    /// If repeatIntervalMillisecs == 0, the task function will be invoked
    /// only when trigger() is called. If the interval is > 0, then
    /// whenever this number of milliseconds have passed without it being
    /// triggered, it'll be automatically invoked again.
    /// If the thread is already running when this it called, it will
    /// first be stopped.
    void start (uint32_t repeatIntervalMillisecs,
                std::function<void()> task);

    /// Stops the thread, waiting for it to finish. This may involve
    /// waiting for the callback function to complete if it's currently
    /// in progress.
    void stop();

    /// This causes the task to be invoked as soon as the thread
    /// is free to do so. Calling it multiple times may result in
    /// only one call to the task being invoked.
    void trigger();

private:
    //==============================================================================
    std::function<void()> taskFunction;
    std::thread thread;
    std::mutex mutex;
    std::condition_variable condition;
    std::atomic<bool> triggered { false };
    std::atomic<bool> threadShouldExit { false };
    uint32_t interval = 0;

    void wait();
};


//==============================================================================
//        _        _           _  _
//     __| |  ___ | |_   __ _ (_)| | ___
//    / _` | / _ \| __| / _` || || |/ __|
//   | (_| ||  __/| |_ | (_| || || |\__ \ _  _  _
//    \__,_| \___| \__| \__,_||_||_||___/(_)(_)(_)
//
//   Code beyond this point is implementation detail...
//
//==============================================================================

inline TaskThread::TaskThread() = default;
inline TaskThread::~TaskThread() { stop(); }

inline void TaskThread::start (uint32_t repeatIntervalMillisecs, std::function<void()> f)
{
    CHOC_ASSERT (f != nullptr);

    stop();
    taskFunction = std::move (f);
    interval = repeatIntervalMillisecs;
    threadShouldExit = false;
    triggered = false;

    thread = std::thread ([this]
    {
        wait();

        while (! threadShouldExit)
        {
            taskFunction();
            wait();
        }
    });
}

inline void TaskThread::stop()
{
    if (thread.joinable())
    {
        threadShouldExit = true;
        trigger();
        thread.join();
    }
}

inline void TaskThread::trigger()
{
    std::lock_guard<std::mutex> lock (mutex);
    triggered = true;
    condition.notify_all();
}

inline void TaskThread::wait()
{
    std::unique_lock<std::mutex> lock (mutex);

    if (! triggered)
    {
        if (interval != 0)
            condition.wait_for (lock, std::chrono::milliseconds (interval),
                                [this] { return triggered == true; });
        else
            condition.wait (lock, [this] { return triggered == true; });
    }

    triggered = false;
}


} // namespace choc::threading

#endif // CHOC_TASKTHREAD_HEADER_INCLUDED
