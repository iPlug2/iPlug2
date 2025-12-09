/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc IPlugQueue
 */

#include <atomic>
#include <cstddef>

#include "heapbuf.h"

#include "IPlugPlatform.h"

BEGIN_IPLUG_NAMESPACE

/** A lock-free SPSC queue used to transfer data between threads
 * based on MLQueue.h by Randy Jones
 * based on https://kjellkod.wordpress.com/2012/11/28/c-debt-paid-in-full-wait-free-lock-free-queue/ */
template<typename T>
class IPlugQueue final
{
public:
  /** IPlugQueue constructor
   * @param size The queue capacity (number of elements) */
  IPlugQueue(int size)
  {
    Resize(size);
  }

  ~IPlugQueue(){}

  IPlugQueue(const IPlugQueue&) = delete;
  IPlugQueue& operator=(const IPlugQueue&) = delete;
    
  /** Changes the queue capacity
   * @param size The new queue capacity (number of elements) */
  void Resize(int size)
  {
    mData.Resize(size + 1);
  }

  /** Adds an item to the queue
   * @param item The item to add to the queue
   * @return true if the item was successfully added
   * @return false if the queue is full */
  bool Push(const T& item)
  {
    const auto currentWriteIndex = mWriteIndex.load(std::memory_order_relaxed);
    const auto nextWriteIndex = Increment(currentWriteIndex);
    if(nextWriteIndex != mReadIndex.load(std::memory_order_acquire))
    {
      mData.Get()[currentWriteIndex] = item;
      mWriteIndex.store(nextWriteIndex, std::memory_order_release);
      return true;
    }
    return false;
  }

  /** Removes and retrieves an item from the queue
   * @param item Reference to store the retrieved item
   * @return true if an item was successfully retrieved
   * @return false if the queue is empty */
  bool Pop(T& item)
  {
    const auto currentReadIndex = mReadIndex.load(std::memory_order_relaxed);
    if(currentReadIndex == mWriteIndex.load(std::memory_order_acquire))
    {
      return false; // empty the queue
    }
    item = mData.Get()[currentReadIndex];
    mReadIndex.store(Increment(currentReadIndex), std::memory_order_release);
    return true;
  }

  /** Constructs and adds an item to the queue in-place from arguments
   * @param args... Arguments to forward to the item's constructor
   * @return true if the item was successfully added
   * @return false if the queue is full */
  template <typename... Args>
  bool PushFromArgs(Args ...args)
  {
    const auto currentWriteIndex = mWriteIndex.load(std::memory_order_relaxed);
    const auto nextWriteIndex = Increment(currentWriteIndex);
    if(nextWriteIndex != mReadIndex.load(std::memory_order_acquire))
    {
      mData.Get()[currentWriteIndex] = T(args...);
      mWriteIndex.store(nextWriteIndex, std::memory_order_release);
      return true;
    }
    return false;
  }
  
  /** Returns the number of elements currently in the queue
   * @return The number of elements available to pop */
  size_t ElementsAvailable() const
  {
    size_t write = mWriteIndex.load(std::memory_order_acquire);
    size_t read = mReadIndex.load(std::memory_order_relaxed);

    return (read > write) ? mData.GetSize() - (read - write) : write - read;
  }

  /** Returns a const reference to the next item without removing it
   * Useful for reading elements while a criterion is met. Can be used like:
   * `while (q.ElementsAvailable() && q.Peek().mTime < 100) { T elem; q.Pop(elem); ... }`
   * @return const reference to the next item in the queue */
  const T& Peek()
  {
    const auto currentReadIndex = mReadIndex.load(std::memory_order_relaxed);
    return mData.Get()[currentReadIndex];
  }

  /** Checks if the queue is currently empty
   * @return true if the queue is empty
   * @return false if the queue contains elements */
  bool WasEmpty() const
  {
    return (mWriteIndex.load() == mReadIndex.load());
  }

  /** Checks if the queue is currently full
   * @return true if the queue is full
   * @return false if the queue has space for more elements */
  bool WasFull() const
  {
    const auto nextWriteIndex = Increment(mWriteIndex.load());
    return (nextWriteIndex == mReadIndex.load());
  }

private:
  /** Increments an index in the circular buffer, wrapping around when necessary
   * @param idx The index to increment
   * @return The incremented index, wrapped to stay within bounds */
  size_t Increment(size_t idx) const
  {
    return (idx + 1) % (mData.GetSize());
  }

  WDL_TypedBuf<T> mData;
  std::atomic<size_t> mWriteIndex{0};
  std::atomic<size_t> mReadIndex{0};
};

END_IPLUG_NAMESPACE
