#include "IPlugTimer.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <future>
#include <memory>
#include <string>
#include <thread>

using namespace iplug;
using namespace std::chrono_literals;

static int sFailures = 0;
static void Check(bool condition, const char* message)
{
  if (!condition)
  {
    fprintf(stderr, "FAIL: %s\n", message);
    ++sFailures;
  }
}

static void TestSelfDestruction()
{
  std::promise<void> ready, completed;
  auto readyFuture = ready.get_future().share();
  auto completedFuture = completed.get_future();
  auto capture = std::make_shared<std::string>("callback survives its timer");
  std::weak_ptr<std::string> lifetime = capture;
  std::unique_ptr<Timer> timer;
  timer.reset(Timer::Create([&, readyFuture, capture](Timer&) {
    readyFuture.wait();
    timer.reset();
    Check(*capture == "callback survives its timer", "callback captures survive self-destruction");
    completed.set_value();
  }, 1));
  Check(bool(timer), "create self-destroying timer");
  if (!timer)
    return;
  capture.reset();
  ready.set_value();
  completedFuture.wait();
  const auto deadline = std::chrono::steady_clock::now() + 2s;
  while (!lifetime.expired() && std::chrono::steady_clock::now() < deadline)
    std::this_thread::yield();
  Check(lifetime.expired(), "worker releases its callback after self-destruction");
}

static void TestCallbackStop()
{
  std::promise<void> stopped;
  auto stoppedFuture = stopped.get_future();
  std::atomic<unsigned> calls {0};
  std::unique_ptr<Timer> timer(Timer::Create([&](Timer& self) {
    ++calls;
    self.Stop();
    stopped.set_value();
  }, 1));
  Check(bool(timer), "create self-stopping timer");
  if (!timer)
    return;
  stoppedFuture.wait();
  timer->Stop();
  timer->Stop();
  Check(calls == 1, "self-stop prevents another callback and remains externally joinable");
}

static void TestImmediateSelfDestruction()
{
  for (int iteration = 0; iteration < 50; ++iteration)
  {
    std::promise<void> completed;
    auto future = completed.get_future();
    // The callback can finish before Create() returns; the factory must not
    // inspect an already destroyed Timer after starting its worker.
    Timer::Create([&](Timer& self) {
      delete &self;
      completed.set_value();
    }, 0);
    future.wait();
  }
}

static void TestConcurrentStop()
{
  std::promise<void> entered, release;
  auto enteredFuture = entered.get_future();
  auto releaseFuture = release.get_future().share();
  std::unique_ptr<Timer> timer(Timer::Create([&](Timer& self) {
    entered.set_value();
    releaseFuture.wait();
    self.Stop(); // Must not wait for an external Stop() that is joining this callback.
  }, 1));
  Check(bool(timer), "create blocked timer");
  if (!timer)
    return;
  enteredFuture.wait();
  std::promise<void> firstEntered, secondEntered;
  auto stop = [&](std::promise<void>& started) {
    started.set_value();
    timer->Stop();
  };
  auto first = std::async(std::launch::async, [&] { stop(firstEntered); });
  auto second = std::async(std::launch::async, [&] { stop(secondEntered); });
  firstEntered.get_future().wait();
  secondEntered.get_future().wait();
  Check(first.wait_for(30ms) == std::future_status::timeout, "first Stop waits for callback completion");
  Check(second.wait_for(30ms) == std::future_status::timeout, "concurrent Stop also waits for callback completion");
  release.set_value();
  first.get();
  second.get();
  timer.reset();
}

static void TestWakeOnStop()
{
  std::atomic<unsigned> calls {0};
  std::unique_ptr<Timer> timer(Timer::Create([&](Timer&) { ++calls; }, 60000));
  Check(bool(timer), "create long-interval timer");
  if (!timer)
    return;
  const auto start = std::chrono::steady_clock::now();
  timer.reset();
  Check(std::chrono::steady_clock::now() - start < 1s, "destruction wakes a sleeping worker promptly");
  Check(calls == 0, "a stopped sleeping timer never invokes its callback");
}

int main()
{
  TestSelfDestruction();
  TestImmediateSelfDestruction();
  TestCallbackStop();
  TestConcurrentStop();
  TestWakeOnStop();
  return sFailures ? 1 : 0;
}
