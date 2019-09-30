/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

/**
 * @file
 * @brief MIDI and sysex structs/utilites
 * @ingroup IPlugStructs
 */

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <algorithm>

#include "IPlugLogger.h"

BEGIN_IPLUG_NAMESPACE

/** Encapsulates a MIDI message and provides helper functions
 * @ingroup IPlugStructs */
struct IMidiMsg
{
  int mOffset;
  uint8_t mStatus, mData1, mData2;
  
  /** /todo */
  enum EStatusMsg
  {
    kNone = 0,
    kNoteOff = 8,
    kNoteOn = 9,
    kPolyAftertouch = 10,
    kControlChange = 11,
    kProgramChange = 12,
    kChannelAftertouch = 13,
    kPitchWheel = 14
  };
  
  /** /todo */
  enum EControlChangeMsg
  {
    kModWheel = 1,
    kBreathController = 2,
    kUndefined003 = 3,
    kFootController = 4,
    kPortamentoTime = 5,
    kChannelVolume = 7,
    kBalance = 8,
    kUndefined009 = 9,
    kPan = 10,
    kExpressionController = 11,
    kEffectControl1 = 12,
    kEffectControl2 = 13,
    kUndefined014 = 14,
    kUndefined015 = 15,
    kGeneralPurposeController1 = 16,
    kGeneralPurposeController2 = 17,
    kGeneralPurposeController3 = 18,
    kGeneralPurposeController4 = 19,
    kUndefined020 = 20,
    kUndefined021 = 21,
    kUndefined022 = 22,
    kUndefined023 = 23,
    kUndefined024 = 24,
    kUndefined025 = 25,
    kUndefined026 = 26,
    kUndefined027 = 27,
    kUndefined028 = 28,
    kUndefined029 = 29,
    kUndefined030 = 30,
    kUndefined031 = 31,
    kSustainOnOff = 64,
    kPortamentoOnOff = 65,
    kSustenutoOnOff = 66,
    kSoftPedalOnOff = 67,
    kLegatoOnOff = 68,
    kHold2OnOff = 69,
    kSoundVariation = 70,
    kResonance = 71,
    kReleaseTime = 72,
    kAttackTime = 73,
    kCutoffFrequency = 74,
    kDecayTime = 75,
    kVibratoRate = 76,
    kVibratoDepth = 77,
    kVibratoDelay = 78,
    kSoundControllerUndefined = 79,
    kUndefined085 = 85,
    kUndefined086 = 86,
    kUndefined087 = 87,
    kUndefined088 = 88,
    kUndefined089 = 89,
    kUndefined090 = 90,
    kTremoloDepth = 92,
    kChorusDepth = 93,
    kPhaserDepth = 95,
    kUndefined102 = 102,
    kUndefined103 = 103,
    kUndefined104 = 104,
    kUndefined105 = 105,
    kUndefined106 = 106,
    kUndefined107 = 107,
    kUndefined108 = 108,
    kUndefined109 = 109,
    kUndefined110 = 110,
    kUndefined111 = 111,
    kUndefined112 = 112,
    kUndefined113 = 113,
    kUndefined114 = 114,
    kUndefined115 = 115,
    kUndefined116 = 116,
    kUndefined117 = 117,
    kUndefined118 = 118,
    kUndefined119 = 119,
    kAllNotesOff = 123
  };
  
  /** /todo 
   * @param offs /todo
   * @param s /todo
   * @param d1 /todo
   * @param d2 /todo */
  IMidiMsg(int offs = 0, uint8_t s = 0, uint8_t d1 = 0, uint8_t d2 = 0)
  : mOffset(offs)
  , mStatus(s)
  , mData1(d1)
  , mData2(d2)
  {}
  
  /** /todo 
   * @param noteNumber /todo
   * @param velocity /todo
   * @param offset /todo
   * @param channel /todo */
  void MakeNoteOnMsg(int noteNumber, int velocity, int offset, int channel = 0)
  {
    Clear();
    mStatus = channel | (kNoteOn << 4) ;
    mData1 = noteNumber;
    mData2 = velocity;
    mOffset = offset;
  }
  
  /** /todo 
   * @param noteNumber /todo
   * @param offset /todo
   * @param channel /todo */
  void MakeNoteOffMsg(int noteNumber, int offset, int channel = 0)
  {
    Clear();
    mStatus = channel | (kNoteOff << 4);
    mData1 = noteNumber;
    mOffset = offset;
  }

  /** /todo 
   * @param value range [-1, 1], converts to [0, 16384) where 8192 = no pitch change.
   * @param channel /todo
   * @param offset /todo */
  void MakePitchWheelMsg(double value, int channel = 0, int offset = 0)
  {
    Clear();
    mStatus = channel | (kPitchWheel << 4);
    int i = 8192 + (int) (value * 8192.0);
    i = std::min(std::max(i, 0), 16383);
    mData2 = i>>7;
    mData1 = i&0x7F;
    mOffset = offset;
  }
  
  /** /todo
   * 
   * @param idx /todo
   * @param value range [0, 1] /todo
   * @param channel /todo
   * @param offset /todo
   */
  void MakeControlChangeMsg(EControlChangeMsg idx, double value, int channel = 0, int offset = 0)
  {
    Clear();
    mStatus = channel | (kControlChange << 4);
    mData1 = idx;
    mData2 = (int) (value * 127.0);
    mOffset = offset;
  }
  
  /** /todo  
   * @param pressure /todo
   * @param offset /todo
   * @param channel /todo */
  void MakeChannelATMsg(int pressure, int offset, int channel)
  {
    Clear();
    mStatus = channel | (kChannelAftertouch << 4);
    mData1 = pressure;
    mData2 = 0;
    mOffset = offset;
  }
  
  /** /todo 
   * @param noteNumber /todo
   * @param pressure /todo
   * @param offset /todo
   * @param channel /todo */
  void MakePolyATMsg(int noteNumber, int pressure, int offset, int channel)
  {
    Clear();
    mStatus = channel | (kPolyAftertouch << 4);
    mData1 = noteNumber;
    mData2 = pressure;
    mOffset = offset;
  }
  
  /** @return [0, 15] for midi channels 1 ... 16 */
  int Channel() const
  {
    return mStatus & 0x0F;
  }
  
  /** /todo  
   * @return EStatusMsg /todo */
  EStatusMsg StatusMsg() const
  {
    unsigned int e = mStatus >> 4;
    if (e < kNoteOff || e > kPitchWheel)
    {
      return kNone;
    }
    return (EStatusMsg) e;
  }
  
  /** @return [0, 127), -1 if NA. */
  int NoteNumber() const
  {
    switch (StatusMsg())
    {
      case kNoteOn:
      case kNoteOff:
      case kPolyAftertouch:
        return mData1;
      default:
        return -1;
    }
  }
  
  /** @return returns [0, 127), -1 if NA. */
  int Velocity() const
  {
    switch (StatusMsg())
    {
      case kNoteOn:
      case kNoteOff:
        return mData2;
      default:
        return -1;
    }
  }
  
  /** @return [0, 127), -1 if NA. */
  int PolyAfterTouch() const
  {
    switch (StatusMsg())
    {
      case kPolyAftertouch:
        return mData2;
      default:
        return -1;
    }
  }
  
  /** @return [0, 127), -1 if NA. */
  int ChannelAfterTouch() const
  {
    switch (StatusMsg())
    {
      case kChannelAftertouch:
        return mData1;
      default:
        return -1;
    }
  }
  
  /** @return [0, 127), -1 if NA. */
  int Program() const
  {
    if (StatusMsg() == kProgramChange)
    {
      return mData1;
    }
    return -1;
  }
  
  /** @return [-1.0, 1.0], zero if NA.*/
  double PitchWheel() const
  {
    if (StatusMsg() == kPitchWheel)
    {
      int iVal = (mData2 << 7) + mData1;
      return (double) (iVal - 8192) / 8192.0;
    }
    return 0.0;
  }
  
  /** /todo 
   * @return EControlChangeMsg /todo */
  EControlChangeMsg ControlChangeIdx() const
  {
    return (EControlChangeMsg) mData1;
  }
  
  /** @return [0, 1], -1 if NA.*/
  double ControlChange(EControlChangeMsg idx) const
  {
    if (StatusMsg() == kControlChange && ControlChangeIdx() == idx)
    {
      return (double) mData2 / 127.0;
    }
    return -1.0;
  }
  
  /** /todo 
   * @param msgValue /todo
   * @return \c true = on */
  static bool ControlChangeOnOff(double msgValue)
  {
    return (msgValue >= 0.5);
  }
  
  /** /todo */
  void Clear()
  {
    mOffset = 0;
    mStatus = mData1 = mData2 = 0;
  }
  
  /** /todo  
   * @param msg /todo
   * @return const char* /todo */
  const char* StatusMsgStr(EStatusMsg msg) const
  {
    switch (msg)
    {
      case kNone: return "none";
      case kNoteOff: return "noteoff";
      case kNoteOn: return "noteon";
      case kPolyAftertouch: return "aftertouch";
      case kControlChange: return "controlchange";
      case kProgramChange: return "programchange";
      case kChannelAftertouch: return "channelaftertouch";
      case kPitchWheel: return "pitchwheel";
      default:  return "unknown";
    };
  }
  
  /** /todo */
  void LogMsg()
  {
    Trace(TRACELOC, "midi:(%s:%d:%d:%d)", StatusMsgStr(StatusMsg()), Channel(), mData1, mData2);
  }
  
  /** /todo */
  void PrintMsg() const
  {
    DBGMSG("midi: offset %i, (%s:%d:%d:%d)\n", mOffset, StatusMsgStr(StatusMsg()), Channel(), mData1, mData2);
  }
};

/** A struct for dealing with SysEx messages. Does not own the data.
  * @ingroup IPlugStructs */
struct ISysEx
{
  int mOffset, mSize;
  const uint8_t* mData;
  
  /** /todo  
   * @param offs /todo
   * @param pData /todo
   * @param size /todo */
  ISysEx(int offs = 0, const uint8_t* pData = nullptr, int size = 0)
  : mOffset(offs)
  , mData(pData)
  , mSize(size)
  {}
  
  /** /todo */
  void Clear()
  {
    mOffset = mSize = 0;
    mData = NULL;
  }
  
  /** /todo 
   * @param str /todo
   * @param maxlen /todo
   * @param pData /todo
   * @param size /todo
   * @return char* /todo */
  char* SysExStr(char *str, int maxlen, const uint8_t* pData, int size)
  {
    assert(str != NULL && maxlen >= 3);
    
    if (!pData || !size) {
      *str = '\0';
      return str;
    }
    
    char* pStr = str;
    int n = maxlen / 3;
    if (n > size) n = size;
    for (int i = 0; i < n; ++i, ++pData) {
      sprintf(pStr, "%02X", (int)*pData);
      pStr += 2;
      *pStr++ = ' ';
    }
    *--pStr = '\0';
    
    return str;
  }
  
  void LogMsg()
  {
    char str[96];
    Trace(TRACELOC, "sysex:(%d:%s)", mSize, SysExStr(str, sizeof(str), mData, mSize));
  }

};

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

void MyPlug::OnReset()
{
  mMidiQueue.Resize(GetBlockSize());
}

void MyPlug::ProcessMidiMsg(IMidiMsg* pMsg)
{
  mMidiQueue.Add(pMsg);
}

void MyPlug::ProcessBlock(double** inputs, double** outputs, int nFrames)
{
  for (int offset = 0; offset < nFrames; ++offset)
  {
    while (!mMidiQueue.Empty())
    {
      IMidiMsg* pMsg = mMidiQueue.Peek();
      if (msg.mOffset > offset) break;

      // To-do: Handle the MIDI message

      mMidiQueue.Remove();
    }

    // To-do: Process audio

  }
  mMidiQueue.Flush(nFrames);
}

*/

#ifndef DEFAULT_BLOCK_SIZE
  #define DEFAULT_BLOCK_SIZE 512
#endif

/** A class to help with queuing timestamped MIDI messages
  * @ingroup IPlugUtilities */
class IMidiQueue
{
public:
  IMidiQueue(int size = DEFAULT_BLOCK_SIZE)
  : mBuf(NULL), mSize(0), mGrow(Granulize(size)), mFront(0), mBack(0)
  {
    Expand();
  }
  
  ~IMidiQueue()
  {
    free(mBuf);
  }

  // Adds a MIDI message at the back of the queue. If the queue is full,
  // it will automatically expand itself.
  void Add(const IMidiMsg& msg)
  {
    if (mBack >= mSize)
    {
      if (mFront > 0)
        Compact();
      else if (!Expand()) return;
    }

#ifndef DONT_SORT_IMIDIQUEUE
    // Insert the MIDI message at the right offset.
    if (mBack > mFront && msg.mOffset < mBuf[mBack - 1].mOffset)
    {
      int i = mBack - 2;
      while (i >= mFront && msg.mOffset < mBuf[i].mOffset) --i;
      i++;
      memmove(&mBuf[i + 1], &mBuf[i], (mBack - i) * sizeof(IMidiMsg));
      mBuf[i] = msg;
    }
    else
#endif
      mBuf[mBack] = msg;
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
  inline IMidiMsg& Peek() const { return mBuf[mFront]; }

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
};

END_IPLUG_NAMESPACE
