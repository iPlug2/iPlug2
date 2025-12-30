/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <thread>
#include <atomic>
#include <vector>
#include <chrono>

#include "IPlugQueue.h"

using namespace iplug;

TEST_CASE("IPlugQueue basic operations", "[IPlugQueue]")
{
  SECTION("Push and Pop single element")
  {
    IPlugQueue<int> queue(16);

    REQUIRE(queue.WasEmpty());
    REQUIRE_FALSE(queue.WasFull());

    int value = 42;
    REQUIRE(queue.Push(value));

    REQUIRE_FALSE(queue.WasEmpty());
    REQUIRE(queue.ElementsAvailable() == 1);

    int result;
    REQUIRE(queue.Pop(result));
    REQUIRE(result == 42);

    REQUIRE(queue.WasEmpty());
    REQUIRE(queue.ElementsAvailable() == 0);
  }

  SECTION("Push and Pop multiple elements in order")
  {
    IPlugQueue<int> queue(16);

    for (int i = 0; i < 10; i++)
    {
      REQUIRE(queue.Push(i));
    }

    REQUIRE(queue.ElementsAvailable() == 10);

    for (int i = 0; i < 10; i++)
    {
      int result;
      REQUIRE(queue.Pop(result));
      REQUIRE(result == i);
    }

    REQUIRE(queue.WasEmpty());
  }

  SECTION("Pop from empty queue returns false")
  {
    IPlugQueue<int> queue(16);

    int result;
    REQUIRE_FALSE(queue.Pop(result));
  }

  SECTION("Peek returns element without removing")
  {
    IPlugQueue<int> queue(16);

    queue.Push(100);
    queue.Push(200);

    REQUIRE(queue.Peek() == 100);
    REQUIRE(queue.ElementsAvailable() == 2);

    int result;
    queue.Pop(result);
    REQUIRE(queue.Peek() == 200);
    REQUIRE(queue.ElementsAvailable() == 1);
  }
}

TEST_CASE("IPlugQueue capacity and overflow", "[IPlugQueue]")
{
  SECTION("Queue respects capacity")
  {
    IPlugQueue<int> queue(4);

    // Can push up to capacity
    REQUIRE(queue.Push(1));
    REQUIRE(queue.Push(2));
    REQUIRE(queue.Push(3));
    REQUIRE(queue.Push(4));

    // Queue should be full now
    REQUIRE(queue.WasFull());

    // Cannot push when full
    REQUIRE_FALSE(queue.Push(5));

    // After pop, can push again
    int result;
    queue.Pop(result);
    REQUIRE_FALSE(queue.WasFull());
    REQUIRE(queue.Push(5));
  }

  SECTION("ElementsAvailable wraps correctly")
  {
    IPlugQueue<int> queue(4);

    // Fill and drain multiple times to test wrap-around
    for (int round = 0; round < 3; round++)
    {
      REQUIRE(queue.WasEmpty());

      queue.Push(1);
      queue.Push(2);
      REQUIRE(queue.ElementsAvailable() == 2);

      int result;
      queue.Pop(result);
      queue.Pop(result);

      REQUIRE(queue.ElementsAvailable() == 0);
    }
  }
}

TEST_CASE("IPlugQueue with complex types", "[IPlugQueue]")
{
  SECTION("Works with struct types")
  {
    struct TestData
    {
      int id;
      float value;
      bool flag;
    };

    IPlugQueue<TestData> queue(8);

    TestData input{42, 3.14f, true};
    REQUIRE(queue.Push(input));

    TestData output;
    REQUIRE(queue.Pop(output));
    REQUIRE(output.id == 42);
    REQUIRE(output.value == Catch::Approx(3.14f));
    REQUIRE(output.flag == true);
  }

  SECTION("PushFromArgs constructs in place")
  {
    struct Point
    {
      int x, y;
      Point() : x(0), y(0) {}
      Point(int px, int py) : x(px), y(py) {}
    };

    IPlugQueue<Point> queue(8);

    REQUIRE(queue.PushFromArgs(10, 20));

    Point result;
    REQUIRE(queue.Pop(result));
    REQUIRE(result.x == 10);
    REQUIRE(result.y == 20);
  }
}

TEST_CASE("IPlugQueue resize", "[IPlugQueue]")
{
  SECTION("Resize changes capacity")
  {
    IPlugQueue<int> queue(4);

    queue.Push(1);
    queue.Push(2);
    queue.Push(3);
    queue.Push(4);
    REQUIRE(queue.WasFull());

    // Note: Resize doesn't preserve existing elements
    // This is expected behavior - resize should be called when queue is empty
    queue.Resize(8);

    // After resize, queue state is reset
    for (int i = 0; i < 8; i++)
    {
      REQUIRE(queue.Push(i));
    }
    REQUIRE(queue.WasFull());
  }
}

TEST_CASE("IPlugQueue thread safety SPSC pattern", "[IPlugQueue][threading]")
{
  SECTION("Single producer single consumer stress test")
  {
    constexpr int kQueueSize = 256;
    constexpr int kNumItems = 100000;

    IPlugQueue<int> queue(kQueueSize);
    std::atomic<bool> producerDone{false};
    std::atomic<int> itemsConsumed{0};
    std::vector<int> consumed;
    consumed.reserve(kNumItems);

    // Producer thread
    std::thread producer([&]()
    {
      for (int i = 0; i < kNumItems; i++)
      {
        while (!queue.Push(i))
        {
          // Queue full, yield and retry
          std::this_thread::yield();
        }
      }
      producerDone.store(true, std::memory_order_release);
    });

    // Consumer thread
    std::thread consumer([&]()
    {
      int value;
      while (!producerDone.load(std::memory_order_acquire) || queue.ElementsAvailable() > 0)
      {
        if (queue.Pop(value))
        {
          consumed.push_back(value);
          itemsConsumed.fetch_add(1, std::memory_order_relaxed);
        }
        else
        {
          std::this_thread::yield();
        }
      }
    });

    producer.join();
    consumer.join();

    // Verify all items were consumed in order
    REQUIRE(consumed.size() == kNumItems);
    for (int i = 0; i < kNumItems; i++)
    {
      REQUIRE(consumed[i] == i);
    }
  }

  SECTION("Producer faster than consumer")
  {
    constexpr int kQueueSize = 32;
    constexpr int kNumItems = 10000;

    IPlugQueue<int> queue(kQueueSize);
    std::atomic<bool> producerDone{false};
    std::vector<int> consumed;
    consumed.reserve(kNumItems);

    std::thread producer([&]()
    {
      for (int i = 0; i < kNumItems; i++)
      {
        while (!queue.Push(i))
        {
          std::this_thread::yield();
        }
      }
      producerDone.store(true, std::memory_order_release);
    });

    std::thread consumer([&]()
    {
      int value;
      while (!producerDone.load(std::memory_order_acquire) || queue.ElementsAvailable() > 0)
      {
        if (queue.Pop(value))
        {
          consumed.push_back(value);
          // Simulate slower consumer
          std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
      }
    });

    producer.join();
    consumer.join();

    REQUIRE(consumed.size() == kNumItems);
    for (int i = 0; i < kNumItems; i++)
    {
      REQUIRE(consumed[i] == i);
    }
  }

  SECTION("Consumer faster than producer")
  {
    constexpr int kQueueSize = 32;
    constexpr int kNumItems = 10000;

    IPlugQueue<int> queue(kQueueSize);
    std::atomic<bool> producerDone{false};
    std::vector<int> consumed;
    consumed.reserve(kNumItems);

    std::thread producer([&]()
    {
      for (int i = 0; i < kNumItems; i++)
      {
        // Simulate slower producer
        std::this_thread::sleep_for(std::chrono::microseconds(1));
        while (!queue.Push(i))
        {
          std::this_thread::yield();
        }
      }
      producerDone.store(true, std::memory_order_release);
    });

    std::thread consumer([&]()
    {
      int value;
      while (!producerDone.load(std::memory_order_acquire) || queue.ElementsAvailable() > 0)
      {
        if (queue.Pop(value))
        {
          consumed.push_back(value);
        }
        else
        {
          std::this_thread::yield();
        }
      }
    });

    producer.join();
    consumer.join();

    REQUIRE(consumed.size() == kNumItems);
    for (int i = 0; i < kNumItems; i++)
    {
      REQUIRE(consumed[i] == i);
    }
  }
}

TEST_CASE("IPlugQueue audio-like scenario", "[IPlugQueue][audio]")
{
  SECTION("Simulated audio buffer transfer")
  {
    // Simulates transferring audio meter data from audio thread to UI
    struct MeterData
    {
      float peak;
      float rms;
      int channel;
    };

    constexpr int kQueueSize = 64;
    constexpr int kNumBlocks = 1000;

    IPlugQueue<MeterData> queue(kQueueSize);
    std::atomic<bool> done{false};
    std::vector<MeterData> received;
    received.reserve(kNumBlocks * 2);

    // Audio thread - produces data at regular intervals
    std::thread audioThread([&]()
    {
      for (int block = 0; block < kNumBlocks; block++)
      {
        MeterData left{0.5f + block * 0.0001f, 0.3f, 0};
        MeterData right{0.4f + block * 0.0001f, 0.25f, 1};

        // In real code, we might drop data if queue is full
        queue.Push(left);
        queue.Push(right);

        // Simulate ~1ms block processing time
        std::this_thread::sleep_for(std::chrono::microseconds(100));
      }
      done.store(true, std::memory_order_release);
    });

    // UI thread - consumes data at irregular intervals
    std::thread uiThread([&]()
    {
      while (!done.load(std::memory_order_acquire) || queue.ElementsAvailable() > 0)
      {
        MeterData data;
        while (queue.Pop(data))
        {
          received.push_back(data);
        }
        // Simulate UI update rate (~16ms)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    });

    audioThread.join();
    uiThread.join();

    // Should have received all meter data
    REQUIRE(received.size() == kNumBlocks * 2);

    // Verify data integrity - alternating channels
    for (size_t i = 0; i < received.size(); i += 2)
    {
      REQUIRE(received[i].channel == 0);
      REQUIRE(received[i + 1].channel == 1);
    }
  }
}
