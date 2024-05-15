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
   * @param size \todo */
  IPlugQueue(int size)
  {
    Resize(size);
  }

  ~IPlugQueue(){}

  IPlugQueue(const IPlugQueue&) = delete;
  IPlugQueue& operator=(const IPlugQueue&) = delete;
    
  /** \todo 
   * @param size \todo */
  void Resize(int size)
  {
    mData.Resize(size + 1);
  }

  /** \todo 
   * @param item \todo
   * @return true \todo
   * @return false \todo */
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

  /** \todo 
   * @param item \todo
   * @return true \todo
   * @return false \todo */
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

  /** \todo
   * @param args... \todo
   * @return true \todo
   * @return false \todo */
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
  
  /** \todo 
   * @return size_t \todo */
  size_t ElementsAvailable() const
  {
    size_t write = mWriteIndex.load(std::memory_order_acquire);
    size_t read = mReadIndex.load(std::memory_order_relaxed);

    return (read > write) ? mData.GetSize() - (read - write) : write - read;
  }

  /** \todo
   * useful for reading elements while a criterion is met. Can be used like
   * while IPlugQueue.ElementsAvailable() && q.peek().mTime < 100 { elem = q.pop() ... }
   * @return const T& \todo */
  const T& Peek()
  {
    const auto currentReadIndex = mReadIndex.load(std::memory_order_relaxed);
    return mData.Get()[currentReadIndex];
  }

  /** \todo 
   * @return true \todo
   * @return false \todo */
  bool WasEmpty() const
  {
    return (mWriteIndex.load() == mReadIndex.load());
  }

  /** \todo 
   * @return true \todo
   * @return false \todo */
  bool WasFull() const
  {
    const auto nextWriteIndex = Increment(mWriteIndex.load());
    return (nextWriteIndex == mReadIndex.load());
  }

private:
  /** \todo 
   * @param idx \todo
   * @return size_t \todo */
  size_t Increment(size_t idx) const
  {
    return (idx + 1) % (mData.GetSize());
  }

  WDL_TypedBuf<T> mData;
  std::atomic<size_t> mWriteIndex{0};
  std::atomic<size_t> mReadIndex{0};
};

END_IPLUG_NAMESPACE
