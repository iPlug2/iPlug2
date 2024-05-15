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
  
  /** Constants for the status byte of a MIDI message */
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
  
  /** Constants for MIDI CC messages */
  enum EControlChangeMsg
  {
    kNoCC = -1,
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
  
  /** Create an IMidiMsg, an abstraction for a MIDI message
   * @param offset Sample offset in block
   * @param status Status byte
   * @param data1 Data byte 1
   * @param data2 Data byte 2 */
  IMidiMsg(int offset = 0, uint8_t status = 0, uint8_t data1 = 0, uint8_t data2 = 0)
  : mOffset(offset)
  , mStatus(status)
  , mData1(data1)
  , mData2(data2)
  {}
  
  /** Make a Note On message
   * @param noteNumber Note number
   * @param velocity Note on velocity
   * @param offset Sample offset in block
   * @param channel MIDI channel [0, 15] */
  void MakeNoteOnMsg(int noteNumber, int velocity, int offset, int channel = 0)
  {
    Clear();
    mStatus = channel | (kNoteOn << 4) ;
    mData1 = noteNumber;
    mData2 = velocity;
    mOffset = offset;
  }
  
  /** Make a Note Off message
   * @param noteNumber Note number
   * @param offset Sample offset in block
   * @param channel MIDI channel [0, 15] */
  void MakeNoteOffMsg(int noteNumber, int offset, int channel = 0)
  {
    Clear();
    mStatus = channel | (kNoteOff << 4);
    mData1 = noteNumber;
    mOffset = offset;
  }

  /** Create a pitch wheel/bend message
   * @param value Range [-1, 1], converts to [0, 16384) where 8192 = no pitch change
   * @param channel MIDI channel [0, 15]
   * @param offset Sample offset in block */
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
  
  /** Create a CC message
   * @param idx Controller index
   * @param value Range [0, 1]
   * @param channel MIDI channel [0, 15]
   * @param offset Sample offset in block */
  void MakeControlChangeMsg(EControlChangeMsg idx, double value, int channel = 0, int offset = 0)
  {
    Clear();
    mStatus = channel | (kControlChange << 4);
    mData1 = idx;
    mData2 = (int) (value * 127.0);
    mOffset = offset;
  }

  /** Create a Program Change message
   * @param program Program index
   * @param channel MIDI channel [0, 15] 
   * @param offset Sample offset in block */
  void MakeProgramChange(int program, int channel = 0, int offset = 0)
  {
    Clear();
    mStatus = channel | (kProgramChange << 4);
    mData1 = program;
    mOffset = offset;
  }

  /** Create a Channel AfterTouch message 
   * @param pressure Range [0, 127]
   * @param offset Sample offset in block
   * @param channel MIDI channel [0, 15] */
  void MakeChannelATMsg(int pressure, int offset, int channel)
  {
    Clear();
    mStatus = channel | (kChannelAftertouch << 4);
    mData1 = pressure;
    mData2 = 0;
    mOffset = offset;
  }
  
  /** Create a Poly AfterTouch message
   * @param noteNumber Note number
   * @param pressure Range [0, 127]
   * @param offset Sample offset in block
   * @param channel MIDI channel [0, 15] */
  void MakePolyATMsg(int noteNumber, int pressure, int offset, int channel)
  {
    Clear();
    mStatus = channel | (kPolyAftertouch << 4);
    mData1 = noteNumber;
    mData2 = pressure;
    mOffset = offset;
  }
  
  /** Gets the channel of a MIDI message
   * @return [0, 15] for midi channels 1 ... 16. */
  int Channel() const
  {
    return mStatus & 0x0F;
  }
  
  /** Gets the MIDI Status message
   * @return EStatusMsg */
  EStatusMsg StatusMsg() const
  {
    unsigned int e = mStatus >> 4;
    if (e < kNoteOff || e > kPitchWheel)
    {
      return kNone;
    }
    return (EStatusMsg) e;
  }
  
  /** Gets the MIDI note number
   * @return [0, 127], -1 if NA. */
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
  
  /** Get the velocity value of a NoteOn/NoteOff message
   * @return returns [0, 127], -1 if NA. */
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
  
  /** Get the Pressure value from a PolyAfterTouch message
   * @return [0, 127], -1 if NA. */
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
  
  /** Get the Pressure value from an AfterTouch message
   * @return [0, 127], -1 if NA. */
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
  
  /** Get the program index from a Program Change message
   * @return [0, 127], -1 if NA. */
  int Program() const
  {
    if (StatusMsg() == kProgramChange)
    {
      return mData1;
    }
    return -1;
  }
  
  /** Get the value from a Pitchwheel message
   * @return [-1.0, 1.0], zero if NA.*/
  double PitchWheel() const
  {
    if (StatusMsg() == kPitchWheel)
    {
      int iVal = (mData2 << 7) + mData1;
      return static_cast<double>(iVal - 8192) / 8192.0;
    }
    return 0.0;
  }
  
  /** Gets the controller index of a CC message
   * @return EControlChangeMsg as an Enum of varying values, refert to the definition of EControlChangeMsg.*/
  EControlChangeMsg ControlChangeIdx() const
  {
    return (EControlChangeMsg) mData1;
  }
  
  /** Get the value of a CC message
   * @return [0, 1], -1 if NA.*/
  double ControlChange(EControlChangeMsg idx) const
  {
    if (StatusMsg() == kControlChange && ControlChangeIdx() == idx)
    {
      return (double) mData2 / 127.0;
    }
    return -1.0;
  }
  
  /** Helper to get a boolean value from a CC messages
   * @param msgValue The normalized CC value [0, 1]
   * @return \c true = on */
  static bool ControlChangeOnOff(double msgValue)
  {
    return (msgValue >= 0.5);
  }
  
  /** Clear the message */
  void Clear()
  {
    mOffset = 0;
    mStatus = mData1 = mData2 = 0;
  }
  
  /** Get the Status Message as a CString
   * @param msg The Status Message
   * @return CString describing the status byte */
  static const char* StatusMsgStr(EStatusMsg msg)
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
  
  /** Get the CC name as a CString
   * @param idx Index of the MIDI CC [0-127]
   * @return CString describing the controller */
  static const char* CCNameStr(int idx)
  {
    static const char* ccNameStrs[128] =
    {
      "BankSel.MSB",
      "Modulation",
      "BreathCtrl",
      "Contr. 3",
      "Foot Ctrl",
      "Porta.Time",
      "DataEntMSB",
      "MainVolume",
      "Balance",
      "Contr. 9",
      "Pan",
      "Expression",
      "FXControl1",
      "FXControl2",
      "Contr. 14",
      "Contr. 15",
      "Gen.Purp.1",
      "Gen.Purp.2",
      "Gen.Purp.3",
      "Gen.Purp.4",
      "Contr. 20",
      "Contr. 21",
      "Contr. 22",
      "Contr. 23",
      "Contr. 24",
      "Contr. 25",
      "Contr. 26",
      "Contr. 27",
      "Contr. 28",
      "Contr. 29",
      "Contr. 30",
      "Contr. 31",
      "BankSel.LSB",
      "Modul. LSB",
      "BrthCt LSB",
      "Contr. 35",
      "FootCt LSB",
      "Port.T LSB",
      "DataEntLSB",
      "MainVolLSB",
      "BalanceLSB",
      "Contr. 41",
      "Pan LSB",
      "Expr. LSB",
      "Contr. 44",
      "Contr. 45",
      "Contr. 46",
      "Contr. 47",
      "Gen.P.1LSB",
      "Gen.P.2LSB",
      "Gen.P.3LSB",
      "Gen.P.4LSB",
      "Contr. 52",
      "Contr. 53",
      "Contr. 54",
      "Contr. 55",
      "Contr. 56",
      "Contr. 57",
      "Contr. 58",
      "Contr. 59",
      "Contr. 60",
      "Contr. 61",
      "Contr. 62",
      "Contr. 63",
      "Damper Ped",
      "Porta. Ped",
      "Sostenuto ",
      "Soft Pedal",
      "Legato Sw",
      "Hold 2",
      "SoundCont 1",
      "SoundCont 2",
      "SoundCont 3",
      "SoundCont 4",
      "SoundCont 5",
      "SoundCont 6",
      "SoundCont 7",
      "SoundCont 8",
      "SoundCont 9",
      "SoundCont 10",
      "Gen.Purp.5",
      "Gen.Purp.6",
      "Gen.Purp.7",
      "Gen.Purp.8",
      "Portamento",
      "Contr. 85",
      "Contr. 86",
      "Contr. 87",
      "Contr. 88",
      "Contr. 89",
      "Contr. 90",
      "FX 1 Depth",
      "FX 2 Depth",
      "FX 3 Depth",
      "FX 4 Depth",
      "FX 5 Depth",
      "Data Incr",
      "Data Decr",
      "Non-RegLSB",
      "Non-RegMSB",
      "Reg LSB",
      "Reg MSB",
      "Contr. 102",
      "Contr. 103",
      "Contr. 104",
      "Contr. 105",
      "Contr. 106",
      "Contr. 107",
      "Contr. 108",
      "Contr. 109",
      "Contr. 110",
      "Contr. 111",
      "Contr. 112",
      "Contr. 113",
      "Contr. 114",
      "Contr. 115",
      "Contr. 116",
      "Contr. 117",
      "Contr. 118",
      "Contr. 119",
      "Contr. 120",
      "Reset Ctrl",
      "Local Ctrl",
      "AllNoteOff",
      "OmniModOff",
      "OmniModeOn",
      "MonoModeOn",
      "PolyModeOn"
    };
    
    return ccNameStrs[idx];
  }

  /** Log a message (TRACER BUILDS) */
  void LogMsg()
  {
    Trace(TRACELOC, "midi:(%s:%d:%d:%d)", StatusMsgStr(StatusMsg()), Channel(), mData1, mData2);
  }
  
  /** Print a message (DEBUG BUILDS) */
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
  
  /** Create an ISysex 
   * @param offset The sample offset for the sysex message
   * @param pData Ptr to the data, which must stay valid while this object is used
   * @param size Size of the data in bytes */
  ISysEx(int offset = 0, const uint8_t* pData = nullptr, int size = 0)
  : mOffset(offset)
  , mSize(size)
  , mData(pData)
  {}
  
  /** Clear the data pointer and size (does not modify the external data!)  */
  void Clear()
  {
    mOffset = mSize = 0;
    mData = NULL;
  }
  
  /** Get the bytes of a sysex message as a CString
   * @param str Buffer for CString
   * @param maxLen size of the CString buffer
   * @param pData Ptr to the bytes of the sysex message
   * @param size Size of the data in bytes
   * @return The CString result */
  char* SysExStr(char* str, int maxLen, const uint8_t* pData, int size)
  {
    assert(str != NULL && maxLen >= 3);
    
    if (!pData || !size) {
      *str = '\0';
      return str;
    }
    
    char* pStr = str;
    int n = maxLen / 3;
    if (n > size) n = size;
    for (int i = 0; i < n; ++i, ++pData) {
      snprintf(pStr, maxLen, "%02X", (int)*pData);
      pStr += 2;
      *pStr++ = ' ';
    }
    *--pStr = '\0';
    
    return str;
  }
  
  /** Log a message (TRACER BUILDS) */
  void LogMsg()
  {
    char str[96];
    Trace(TRACELOC, "sysex:(%d:%s)", mSize, SysExStr(str, sizeof(str), mData, mSize));
  }

};

/*

IMidiQueueBase is a template adapted by Alex Harker from the following source
It has been adapted to allow different types (e.g. IMidiMsg or ISysEx)
It is then mapped to IMidiQueue as an alias
 
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
template <class T>
class IMidiQueueBase
{
public:
  IMidiQueueBase(int size = DEFAULT_BLOCK_SIZE)
  : mBuf(NULL), mSize(0), mGrow(Granulize(size)), mFront(0), mBack(0)
  {
    Expand();
  }
  
  ~IMidiQueueBase()
  {
    free(mBuf);
  }

  // Adds a MIDI message at the back of the queue. If the queue is full,
  // it will automatically expand itself.
  void Add(const T& msg)
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
      memmove(&mBuf[i + 1], &mBuf[i], (mBack - i) * sizeof(T));
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
  inline T& Peek() const { return mBuf[mFront]; }

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

    void* buf = realloc(mBuf, size * sizeof(T));
    if (!buf) return mSize;

    mBuf = (T*)buf;
    mSize = size;
    return size;
  }

protected:
  // Automatically expands the queue.
  bool Expand()
  {
    if (!mGrow) return false;
    int size = (mSize / mGrow + 1) * mGrow;

    void* buf = realloc(mBuf, size * sizeof(T));
    if (!buf) return false;

    mBuf = (T*)buf;
    mSize = size;
    return true;
  }

  // Moves everything all the way to the front.
  inline void Compact()
  {
    mBack -= mFront;
    if (mBack > 0) memmove(&mBuf[0], &mBuf[mFront], mBack * sizeof(T));
    mFront = 0;
  }

  // Rounds the MIDI queue size up to the next 4 kB memory page size.
  inline int Granulize(int size) const
  {
    int bytes = size * sizeof(T);
    int rest = bytes % 4096;
    if (rest) size = (bytes - rest + 4096) / sizeof(T);
    return size;
  }

  T* mBuf;

  int mSize, mGrow;
  int mFront, mBack;
};

using IMidiQueue = IMidiQueueBase<IMidiMsg>;

END_IPLUG_NAMESPACE
