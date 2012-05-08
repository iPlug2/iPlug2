#ifndef _IMIDIQUEUE_
#define _IMIDIQUEUE_

/*

IMidiQueue
(c) Theo Niessink 2009-2011
<http://www.taletn.com/>


This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software in a
   product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.


IMidiQueue is a fast, lean & mean MIDI queue for IPlug instruments or
effects. Here are a few code snippets showing how to implement IMidiQueue in
an IPlug project:


MyPlug.h:

#include "WDL/IPlug/IMidiQueue.h"

class MyPlug: public IPlug
{
protected:
  IMidiQueue mMidiQueue;
}


MyPlug.cpp:

void MyPlug::Reset()
{
  mMidiQueue.Resize(GetBlockSize());
}

void MyPlug::ProcessMidiMsg(IMidiMsg* pMsg)
{
  mMidiQueue.Add(pMsg);
}

void MyPlug::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  for (int offset = 0; offset < nFrames; ++offset)
  {
    while (!mMidiQueue.Empty())
    {
      IMidiMsg* pMsg = mMidiQueue.Peek();
      if (pMsg->mOffset > offset) break;

      // To-do: Handle the MIDI message

      mMidiQueue.Remove();
    }

    // To-do: Process audio

  }
  mMidiQueue.Flush(nFrames);
}

*/


class IMidiQueue
{
public:
  IMidiQueue(int size = DEFAULT_BLOCK_SIZE): mBuf(NULL), mSize(0), mGrow(Granulize(size)), mFront(0), mBack(0) { Expand(); }
  ~IMidiQueue() { free(mBuf); }

  // Adds a MIDI message add the back of the queue. If the queue is full,
  // it will automatically expand itself.
  void Add(IMidiMsg* pMsg)
  {
    if (mBack >= mSize)
    {
      if (mFront > 0)
        Compact();
      else if (!Expand()) return;
    }

#ifndef DONT_SORT_IMIDIQUEUE
    // Insert the MIDI message at the right offset.
    if (mBack > mFront && pMsg->mOffset < mBuf[mBack - 1].mOffset)
    {
      int i = mBack - 2;
      while (i >= mFront && pMsg->mOffset < mBuf[i].mOffset) --i;
      i++;
      memmove(&mBuf[i + 1], &mBuf[i], (mBack - i) * sizeof(IMidiMsg));
      mBuf[i] = *pMsg;
    }
    else
#endif
      mBuf[mBack] = *pMsg;
    ++mBack;
  }

  // Removes a MIDI message from the front of the queue (but does *not*
  // free up its space until Compact() is called).
  inline void Remove() { ++mFront; }

  // Returns true if the queue is empty.
  inline bool Empty() const { return mFront == mBack; }

  // Returns the number of MIDI messages in the queue.
  inline int ToDo() const { return mBack - mFront; }

  // Returns the number of MIDI messages for which memory has already been
  // allocated.
  inline int GetSize() const { return mSize; }

  // Returns the "next" MIDI message (all the way in the front of the
  // queue), but does *not* remove it from the queue.
  inline IMidiMsg* Peek() const { return &mBuf[mFront]; }

  // Moves back MIDI messages all the way to the front of the queue, thus
  // freeing up space at the back, and updates the sample offset of the
  // remaining MIDI messages by substracting nFrames.
  inline void Flush(int nFrames)
  {
    // Move everything all the way to the front.
    if (mFront > 0) Compact();

    // Update the sample offset.
    for (int i = 0; i < mBack; ++i) mBuf[i].mOffset -= nFrames;
  }

  // Clears the queue.
  inline void Clear() { mFront = mBack = 0; }

  // Resizes (grows or shrinks) the queue, returns the new size.
  int Resize(int size)
  {
    if (mFront > 0) Compact();
    mGrow = size = Granulize(size);
    // Don't shrink below the number of currently queued MIDI messages.
    if (size < mBack) size = Granulize(mBack);
    if (size == mSize) return mSize;

    void* buf = realloc(mBuf, size * sizeof(IMidiMsg));
    if (!buf) return mSize;

    mBuf = (IMidiMsg*)buf;
    mSize = size;
    return size;
  }

protected:
  // Automatically expands the queue.
  bool Expand()
  {
    if (!mGrow) return false;
    int size = (mSize / mGrow + 1) * mGrow;

    void* buf = realloc(mBuf, size * sizeof(IMidiMsg));
    if (!buf) return false;

    mBuf = (IMidiMsg*)buf;
    mSize = size;
    return true;
  }

  // Moves everything all the way to the front.
  inline void Compact()
  {
    mBack -= mFront;
    if (mBack > 0) memmove(&mBuf[0], &mBuf[mFront], mBack * sizeof(IMidiMsg));
    mFront = 0;
  }

  // Rounds the MIDI queue size up to the next 4 kB memory page size.
  inline int Granulize(int size) const
  {
    int bytes = size * sizeof(IMidiMsg);
    int rest = bytes % 4096;
    if (rest) size = (bytes - rest + 4096) / sizeof(IMidiMsg);
    return size;
  }

  IMidiMsg* mBuf;

  int mSize, mGrow;
  int mFront, mBack;
} WDL_FIXALIGN;


#endif // _IMIDIQUEUE_
