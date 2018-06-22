/*
 ==============================================================================
 
 This file is part of the iPlug 2 library
 
 Oli Larkin et al. 2018 - https://www.olilarkin.co.uk
 
 iPlug 2 is an open source library subject to commercial or open-source
 licensing.
 
 The code included in this file is provided under the terms of the WDL license
 - https://www.cockos.com/wdl/
 
 ==============================================================================
 */

#pragma once

#include <algorithm>
#include "wdlstring.h"
#include "ptrlist.h"

#include "IPlugConstants.h"
#include "IPlugPlatform.h"
#include "IPlugMidi.h" // <- Midi related structs in here


/** In certain cases we need to queue parameter changes for transferral between threads */
struct IParamChange
{
  int paramIdx;
  double value;
  bool normalized; // TODO: Remove this
};

/** Manages a block of memory, for plug-in settings store/recall */
class IByteChunk
{
public:
  IByteChunk() {}
  ~IByteChunk() {}
  
  /** This method is used in order to place the IPlug version number in the chunk when serialising data. In theory this is for backwards compatibility.
   * @param chunk reference to the chunk where the version number will be placed */
  static void InitChunkWithIPlugVer(IByteChunk& chunk)
  {
    chunk.Clear();
    int magic = IPLUG_VERSION_MAGIC;
    chunk.Put(&magic);
    int ver = IPLUG_VERSION;
    chunk.Put(&ver);
  }
  
  /** Helper method to retrieve the IPlug version number from the beginning of the byte chunk
   * @param chunk The incoming byte chunk that contains the version number
   * @param pos The position (in bytes) to start looking
   * @return The IPlug version number, retrieved from the chunk, or 0 if it failed */
  static int GetIPlugVerFromChunk(const IByteChunk& chunk, int& position)
  {
    int magic = 0, ver = 0;
    int magicpos = chunk.Get(&magic, position);
    
    if (magicpos > position && magic == IPLUG_VERSION_MAGIC)
      position = chunk.Get(&ver, magicpos);
    
    return ver;
  }
  
  /**
   * Copies data into the chunk
   * @param pBuf Pointer to the object to copy data from
   * @param size Number of bytes to copy
   */
  inline int PutBytes(const void* pBuf, int size)
  {
    int n = mBytes.GetSize();
    mBytes.Resize(n + size);
    memcpy(mBytes.Get() + n, pBuf, size);
    return mBytes.GetSize();
  }
  
  inline int GetBytes(void* pBuf, int size, int startPos) const
  {
    int endPos = startPos + size;
    if (startPos >= 0 && endPos <= mBytes.GetSize())
    {
      memcpy(pBuf, mBytes.Get() + startPos, size);
      return endPos;
    }
    return -1;
  }
  
  template <class T> inline int Put(const T* pVal)
  {
    return PutBytes(pVal, sizeof(T));
  }
  
  template <class T> inline int Get(T* pVal, int startPos) const
  {
    return GetBytes(pVal, sizeof(T), startPos);
  }
  
  inline int PutStr(const char* str)
  {
    int slen = (int) strlen(str);
    Put(&slen);
    return PutBytes(str, slen);
  }
  
  inline int GetStr(WDL_String& str, int startPos) const
  {
    int len;
    int strStartPos = Get(&len, startPos);
    if (strStartPos >= 0)
    {
      int strEndPos = strStartPos + len;
      if (strEndPos <= mBytes.GetSize())
      {
        if (len > 0)
          str.Set((char*) (mBytes.Get() + strStartPos), len);
        else
          str.Set("");
      }
      return strEndPos;
    }
    return -1;
  }
  
  inline int PutChunk(IByteChunk* pRHS)
  {
    return PutBytes(pRHS->GetBytes(), pRHS->Size());
  }
  
  /** @brief Clears the chunk
   *
   * This also sets the size to 0 bytes
   */
  inline void Clear()
  {
    mBytes.Resize(0);
  }
  
  /**
   * Returns the current size of the chunk
   * @return Current size (in bytes)
   */
  inline int Size()
  {
    return mBytes.GetSize();
  }
  
  /** Resizes the chunk /todo check
   * @param newSize Desired size (in bytes)
   * @return Old size (in bytes) */
  inline int Resize(int newSize)
  {
    int n = mBytes.GetSize();
    mBytes.Resize(newSize);
    if (newSize > n)
    {
      memset(mBytes.Get() + n, 0, (newSize - n));
    }
    return n;
  }
  
  inline uint8_t* GetBytes() // TODO: BAD NAME!
  {
    return mBytes.Get();
  }
  
  inline bool IsEqual(IByteChunk& otherChunk)
  {
    return (otherChunk.Size() == Size() && !memcmp(otherChunk.GetBytes(), GetBytes(), Size()));
  }
  
private:
  WDL_TypedBuf<uint8_t> mBytes;
};

/** Helper struct to set compile time options to an API class constructor  */
struct IPlugConfig
{
  int nParams;
  int nPresets;
  const char* channelIOStr;
  const char* pluginName;
  const char* productName;
  const char* mfrName;
  int vendorVersion;
  int uniqueID;
  int mfrID;
  int latency;
  bool plugDoesMidi;
  bool plugDoesChunks;
  bool plugIsInstrument;
  bool plugHasUI;
  int plugWidth;
  int plugHeight;
  
  IPlugConfig(int nParams,
              int nPresets,
              const char* channelIOStr,
              const char* pluginName,
              const char* productName,
              const char* mfrName,
              int vendorVersion,
              int uniqueID,
              int mfrID,
              int latency,
              bool plugDoesMidi,
              bool plugDoesChunks,
              bool plugIsInstrument,
              bool plugHasUI,
              int plugWidth,
              int plugHeight)
              
  : nParams(nParams)
  , nPresets(nPresets)
  , channelIOStr(channelIOStr)
  , pluginName(pluginName)
  , productName(productName)
  , mfrName(mfrName)
  , vendorVersion(vendorVersion)
  , uniqueID(uniqueID)
  , mfrID(mfrID)
  , latency(latency)
  , plugDoesMidi(plugDoesMidi)
  , plugDoesChunks(plugDoesChunks)
  , plugIsInstrument(plugIsInstrument)
  , plugHasUI(plugHasUI)
  , plugWidth(plugWidth)
  , plugHeight(plugHeight)
  {};
};

/** Used to manage scratch buffers for each channel of I/O, which may involve converting from single to double precision */
template<class TIN = PLUG_SAMPLE_SRC, class TOUT = PLUG_SAMPLE_DST>
struct IChannelData
{
  bool mConnected = false;
  TOUT** mData = nullptr; // If this is for an input channel, points into IPlugProcessor::mInData, if it's for an output channel points into IPlugProcessor::mOutData
  TIN* mIncomingData = nullptr;
  WDL_TypedBuf<TOUT> mScratchBuf;
  WDL_String mLabel = WDL_String("");
};

struct IBusInfo
{
  ERoute mDirection;
  int mNChans;
  WDL_String mLabel;
  
  IBusInfo(ERoute direction, int nchans = 0, const char* label = "")
  : mDirection(direction)
  , mNChans(nchans)
  {
    if(CStringHasContents(label))
      mLabel.Set(label);
    else
      mLabel.Set(RoutingDirStrs[direction]);
  }
};

/** An IOConfig is used to store bus info for each input/output configuration defined in the channel io string */
struct IOConfig
{
  WDL_PtrList<IBusInfo> mBusInfo[2];  // A particular valid io config may have multiple input buses or output busses
  
  ~IOConfig()
  {
    mBusInfo[0].Empty(true);
    mBusInfo[1].Empty(true);
  }
  
  void AddBusInfo(ERoute direction, int NChans, const char* label = "")
  {
    mBusInfo[direction].Add(new IBusInfo(direction, NChans, label));
  }
  
  IBusInfo* GetBusInfo(ERoute direction, int index)
  {
    assert(index >= 0 && index < mBusInfo[direction].GetSize());
    return mBusInfo[direction].Get(index);
  }
  
  int NChansOnBusSAFE(ERoute direction, int index)
  {
    int NChans = 0;
    
    if(index >= 0 && index < mBusInfo[direction].GetSize())
      NChans = mBusInfo[direction].Get(index)->mNChans;

    return NChans;
  }
  
  int NBuses(ERoute direction)
  {
    return mBusInfo[direction].GetSize();
  }
  
  /** Get the total number of channels across all direction buses for this IOConfig */
  int GetTotalNChannels(ERoute direction) const
  {
    int total = 0;
    
    for(int i = 0; i < mBusInfo[direction].GetSize(); i++)
      total += mBusInfo[direction].Get(i)->mNChans;
    
    return total;
  }
  
  bool ContainsWildcard(ERoute direction)
  {
    for(auto i = 0; i < mBusInfo[direction].GetSize(); i++)
    {
      if(mBusInfo[direction].Get(i)->mNChans < 0)
        return true;
    }

    return false;
  }
};

/** Encapsulates information about the host transport state */
struct ITimeInfo
{
  double mTempo = DEFAULT_TEMPO;
  double mSamplePos = -1.0;
  double mPPQPos = -1.0;
  double mLastBar = -1.0;
  double mCycleStart = -1.0;
  double mCycleEnd = -1.0;

  int mNumerator = 4;
  int mDenominator = 4;

  bool mTransportIsRunning = false;
  bool mTransportLoopEnabled = false;

  ITimeInfo()
  {}
};

/** A struct used for specifying baked-in factory presets */
struct IPreset
{
  bool mInitialized = false;
  char mName[MAX_PRESET_NAME_LEN];

  IByteChunk mChunk;

  IPreset()
  {
    sprintf(mName, "%s", UNUSED_PRESET_NAME);
  }
};
