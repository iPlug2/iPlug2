/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

// Unit tests for IPlugQueue (IPlugQueue.h), the lock-free single-producer/single-consumer queue.

#include <atomic>
#include <cstdint>
#include <vector>

#ifndef __EMSCRIPTEN__
#include <thread>
#endif

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "IPlugQueue.h"

using namespace iplug;

namespace
{
struct Item
{
  int id = 0;
  double value = 0.;

  Item() {}
  Item(int id, double value) : id(id), value(value) {}
};

struct Block
{
  uint32_t seq;
  uint32_t payload[7];
};
} // namespace

TEST_CASE("IPlugQueue: a new queue is empty", "[unit][queue]")
{
  IPlugQueue<int> q {4};
  REQUIRE(q.ElementsAvailable() == 0);
  REQUIRE(q.WasEmpty());
  REQUIRE_FALSE(q.WasFull());

  int v = 123;
  REQUIRE_FALSE(q.Pop(v));
  REQUIRE(v == 123);
  REQUIRE(q.ElementsAvailable() == 0);
}

TEST_CASE("IPlugQueue: push then pop", "[unit][queue]")
{
  IPlugQueue<int> q {4};
  REQUIRE(q.Push(1));
  REQUIRE(q.ElementsAvailable() == 1);
  REQUIRE_FALSE(q.WasEmpty());
  REQUIRE_FALSE(q.WasFull());

  int v = 0;
  REQUIRE(q.Pop(v));
  REQUIRE(v == 1);
  REQUIRE(q.ElementsAvailable() == 0);
  REQUIRE(q.WasEmpty());
  REQUIRE_FALSE(q.Pop(v));
  REQUIRE(v == 1);
}

TEST_CASE("IPlugQueue: holds exactly its capacity", "[unit][queue]")
{
  const int capacity = GENERATE(1, 2, 3, 7, 8, 64, 1000);
  CAPTURE(capacity);

  IPlugQueue<int> q {capacity};

  for (int i = 0; i < capacity; i++)
  {
    REQUIRE_FALSE(q.WasFull());
    REQUIRE(q.Push(i));
    REQUIRE(q.ElementsAvailable() == static_cast<size_t>(i + 1));
  }

  REQUIRE(q.WasFull());
  REQUIRE_FALSE(q.WasEmpty());

  // pushing to a full queue fails and leaves the contents alone
  REQUIRE_FALSE(q.Push(-1));
  REQUIRE_FALSE(q.Push(-2));
  REQUIRE(q.ElementsAvailable() == static_cast<size_t>(capacity));

  for (int i = 0; i < capacity; i++)
  {
    int v = -100;
    REQUIRE(q.Pop(v));
    REQUIRE(v == i);
    REQUIRE_FALSE(q.WasFull());
  }

  REQUIRE(q.WasEmpty());
  REQUIRE(q.ElementsAvailable() == 0);

  // space is available again once an element has been popped
  REQUIRE(q.Push(5));
  REQUIRE(q.ElementsAvailable() == 1);
}

TEST_CASE("IPlugQueue: a queue of capacity zero accepts nothing", "[unit][queue]")
{
  IPlugQueue<int> q {0};
  REQUIRE(q.WasEmpty());
  REQUIRE(q.WasFull());
  REQUIRE_FALSE(q.Push(1));
  REQUIRE(q.ElementsAvailable() == 0);
  int v = 9;
  REQUIRE_FALSE(q.Pop(v));
  REQUIRE(v == 9);
}

TEST_CASE("IPlugQueue: preserves order across wrap-around", "[unit][queue]")
{
  const int capacity = GENERATE(1, 3, 4, 5, 16);
  const int burst = GENERATE(1, 2, 3);
  CAPTURE(capacity, burst);

  if (burst <= capacity)
  {
    IPlugQueue<int> q {capacity};
    int nextIn = 0;
    int nextOut = 0;
    bool inOrder = true;
    bool countsOK = true;

    // many more elements than the capacity pass through, so the indices wrap repeatedly
    for (int iteration = 0; iteration < capacity * 10 + 7; iteration++)
    {
      for (int i = 0; i < burst; i++)
        REQUIRE(q.Push(nextIn++));

      countsOK = countsOK && (q.ElementsAvailable() == static_cast<size_t>(nextIn - nextOut));

      for (int i = 0; i < burst; i++)
      {
        int v = -1;
        REQUIRE(q.Pop(v));
        inOrder = inOrder && (v == nextOut++);
      }

      countsOK = countsOK && (q.ElementsAvailable() == 0) && q.WasEmpty();
    }

    REQUIRE(inOrder);
    REQUIRE(countsOK);
  }
}

TEST_CASE("IPlugQueue: ElementsAvailable is right when the write index has wrapped", "[unit][queue]")
{
  IPlugQueue<int> q {4};
  int v = 0;

  // advance both indices towards the end of the storage
  for (int i = 0; i < 3; i++)
  {
    REQUIRE(q.Push(i));
    REQUIRE(q.Pop(v));
  }

  // now fill: the write index wraps past the read index
  for (int i = 0; i < 4; i++)
  {
    REQUIRE(q.Push(10 + i));
    REQUIRE(q.ElementsAvailable() == static_cast<size_t>(i + 1));
  }

  REQUIRE(q.WasFull());
  REQUIRE_FALSE(q.Push(99));

  for (int i = 0; i < 4; i++)
  {
    REQUIRE(q.ElementsAvailable() == static_cast<size_t>(4 - i));
    REQUIRE(q.Pop(v));
    REQUIRE(v == 10 + i);
  }

  REQUIRE(q.ElementsAvailable() == 0);
}

TEST_CASE("IPlugQueue: keeping the queue partially full", "[unit][queue]")
{
  IPlugQueue<int> q {8};
  int nextIn = 0;
  int nextOut = 0;

  for (int i = 0; i < 5; i++)
    REQUIRE(q.Push(nextIn++));

  bool ok = true;
  for (int i = 0; i < 100; i++)
  {
    int v = -1;
    ok = ok && q.Pop(v) && (v == nextOut++);
    ok = ok && q.Push(nextIn++);
    ok = ok && (q.ElementsAvailable() == 5);
  }
  REQUIRE(ok);
}

TEST_CASE("IPlugQueue: Peek returns the next element without removing it", "[unit][queue]")
{
  IPlugQueue<int> q {3};
  q.Push(10);
  q.Push(20);

  REQUIRE(q.Peek() == 10);
  REQUIRE(q.Peek() == 10);
  REQUIRE(q.ElementsAvailable() == 2);

  int v = 0;
  q.Pop(v);
  REQUIRE(v == 10);
  REQUIRE(q.Peek() == 20);

  q.Push(30);
  q.Push(40);
  REQUIRE(q.Peek() == 20);
  q.Pop(v);
  REQUIRE(q.Peek() == 30);
  q.Pop(v);
  REQUIRE(q.Peek() == 40);

  SECTION("can be used to pop while a condition holds")
  {
    IPlugQueue<Item> items {8};
    for (int i = 0; i < 6; i++)
      items.Push(Item(i, i * 10.));

    int nPopped = 0;
    while (items.ElementsAvailable() && items.Peek().value < 35.)
    {
      Item item;
      items.Pop(item);
      nPopped++;
    }

    REQUIRE(nPopped == 4);
    REQUIRE(items.ElementsAvailable() == 2);
    REQUIRE(items.Peek().id == 4);
  }
}

TEST_CASE("IPlugQueue: PushFromArgs constructs the element", "[unit][queue]")
{
  IPlugQueue<Item> q {2};
  REQUIRE(q.PushFromArgs(1, 0.5));
  REQUIRE(q.PushFromArgs(2, 1.5));
  REQUIRE(q.WasFull());
  REQUIRE_FALSE(q.PushFromArgs(3, 2.5));
  REQUIRE(q.ElementsAvailable() == 2);

  Item item;
  REQUIRE(q.Pop(item));
  REQUIRE(item.id == 1);
  REQUIRE(item.value == 0.5);
  REQUIRE(q.Pop(item));
  REQUIRE(item.id == 2);
  REQUIRE(item.value == 1.5);
  REQUIRE_FALSE(q.Pop(item));

  // Push and PushFromArgs can be mixed
  REQUIRE(q.Push(Item(4, 4.)));
  REQUIRE(q.PushFromArgs(5, 5.));
  REQUIRE(q.Pop(item));
  REQUIRE(item.id == 4);
  REQUIRE(q.Pop(item));
  REQUIRE(item.id == 5);
}

TEST_CASE("IPlugQueue: elements are copied in and out", "[unit][queue]")
{
  IPlugQueue<Block> q {4};

  Block in;
  in.seq = 1;
  for (int i = 0; i < 7; i++)
    in.payload[i] = 100u + static_cast<uint32_t>(i);

  REQUIRE(q.Push(in));

  // changing the source after pushing does not affect the queued copy
  in.seq = 2;
  in.payload[0] = 0;
  REQUIRE(q.Push(in));

  Block out {};
  REQUIRE(q.Pop(out));
  REQUIRE(out.seq == 1);
  for (int i = 0; i < 7; i++)
    REQUIRE(out.payload[i] == 100u + static_cast<uint32_t>(i));

  REQUIRE(q.Pop(out));
  REQUIRE(out.seq == 2);
  REQUIRE(out.payload[0] == 0);
  REQUIRE(out.payload[6] == 106u);
}

TEST_CASE("IPlugQueue: Resize changes the capacity of an empty queue", "[unit][queue]")
{
  IPlugQueue<int> q {2};
  REQUIRE(q.Push(1));
  REQUIRE(q.Push(2));
  REQUIRE_FALSE(q.Push(3));

  int v = 0;
  REQUIRE(q.Pop(v));
  REQUIRE(q.Pop(v));
  REQUIRE(q.WasEmpty());

  SECTION("growing")
  {
    q.Resize(10);
    REQUIRE(q.ElementsAvailable() == 0);

    for (int i = 0; i < 10; i++)
      REQUIRE(q.Push(i));

    REQUIRE(q.WasFull());
    REQUIRE_FALSE(q.Push(10));

    for (int i = 0; i < 10; i++)
    {
      REQUIRE(q.Pop(v));
      REQUIRE(v == i);
    }
  }

  SECTION("growing keeps queued elements when the indices have not wrapped")
  {
    IPlugQueue<int> fresh {2};
    REQUIRE(fresh.Push(7));
    REQUIRE(fresh.Push(8));
    fresh.Resize(4);
    REQUIRE(fresh.ElementsAvailable() == 2);
    REQUIRE(fresh.Push(9));
    REQUIRE(fresh.Push(10));
    REQUIRE_FALSE(fresh.Push(11));

    for (int expected = 7; expected <= 10; expected++)
    {
      REQUIRE(fresh.Pop(v));
      REQUIRE(v == expected);
    }
  }
}

#ifndef __EMSCRIPTEN__
TEST_CASE("IPlugQueue: single producer / single consumer stress", "[unit][queue][threads]")
{
  const uint32_t nItems = 200000;
  IPlugQueue<Block> q {64};

  std::atomic<bool> producerDone {false};
  uint32_t nPushFailures = 0;

  std::thread producer([&]() {
    Block block;
    for (uint32_t seq = 0; seq < nItems; seq++)
    {
      block.seq = seq;
      for (int i = 0; i < 7; i++)
        block.payload[i] = seq * 7u + static_cast<uint32_t>(i);

      while (!q.Push(block))
      {
        nPushFailures++;
        std::this_thread::yield();
      }
    }
    producerDone.store(true);
  });

  uint32_t nReceived = 0;
  uint32_t nOutOfOrder = 0;
  uint32_t nCorrupt = 0;
  size_t maxAvailable = 0;

  std::thread consumer([&]() {
    Block block {};
    for (;;)
    {
      const size_t available = q.ElementsAvailable();
      if (available > maxAvailable)
        maxAvailable = available;

      if (q.Pop(block))
      {
        if (block.seq != nReceived)
          nOutOfOrder++;

        for (int i = 0; i < 7; i++)
        {
          if (block.payload[i] != block.seq * 7u + static_cast<uint32_t>(i))
            nCorrupt++;
        }

        nReceived++;
      }
      else if (producerDone.load() && q.WasEmpty())
      {
        break;
      }
      else
      {
        std::this_thread::yield();
      }
    }
  });

  producer.join();
  consumer.join();

  REQUIRE(nReceived == nItems);
  REQUIRE(nOutOfOrder == 0);
  REQUIRE(nCorrupt == 0);
  REQUIRE(maxAvailable <= 64);
  REQUIRE(q.ElementsAvailable() == 0);
  REQUIRE(q.WasEmpty());
}
#endif // !__EMSCRIPTEN__
