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

/** A lock-free SPSC queue used to transfer data between threads
 * based on MLQueue.h by Randy Jones
 * based on https://kjellkod.wordpress.com/2012/11/28/c-debt-paid-in-full-wait-free-lock-free-queue/ */
template<typename T>
class IPlugQueue final
{
public:
  /** IPlugQueue constructor 
   * @param size /todo */
  IPlugQueue(int size)
  {
    Resize(size);
  }

  ~IPlugQueue(){}

  IPlugQueue(const IPlugQueue&) = delete;
  IPlugQueue& operator=(const IPlugQueue&) = delete;
    
  /** /todo 
   * @param size /todo */
  void Resize(int size)
  {
    mData.Resize(size + 1);
    mReadIndex.store(0);
    mWriteIndex.store(0);
  }

  /** /todo 
   * @param item /todo
   * @return true /todo
   * @return false /todo */
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
    return false; // full queue
  }

  /** /todo 
   * @param item /todo
   * @return true /todo
   * @return false /todo */
  bool Pop(T& item)
  {
    const auto currentReadIndex = mReadIndex.load(std::memory_order_relaxed);
    if(currentReadIndex == mWriteIndex.load(std::memory_order_acquire))
    {
      return false; // empty queue
    }
    item = mData.Get()[currentReadIndex];
    mReadIndex.store(Increment(currentReadIndex), std::memory_order_release);
    return true;
  }

  /** /todo 
   * @return true /todo
   * @return false /todo */
  bool WasEmpty() const
  {
    return (mWriteIndex.load() == mReadIndex.load());
  }

  /** /todo 
   * @return true /todo
   * @return false /todo */
  bool WasFull() const
  {
    const auto nextWriteIndex = Increment(mWriteIndex.load());
    return (nextWriteIndex == mReadIndex.load());
  }

private:
  /** /todo 
   * @param idx /todo
   * @return size_t /todo */
  size_t Increment(size_t idx) const
  {
    return (idx + 1) % (mData.GetSize());
  }

  WDL_TypedBuf<T> mData;
  std::atomic<size_t> mWriteIndex{0};
  std::atomic<size_t> mReadIndex{0};
};

