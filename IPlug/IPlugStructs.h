/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

/**
 * @file Structures in small classes used throughout the IPlug code base
 * @defgroup IPlugStructs IPlug::Structs
 * Structures in small classes used throughout the IPlug code base
 * @{
 */

#include <algorithm>
#include "wdlstring.h"
#include "ptrlist.h"

#include "IPlugConstants.h"
#include "IPlugPlatform.h"
#include "IPlugMidi.h" // <- Midi related structs in here
#include "IPlugUtilities.h"

BEGIN_IPLUG_NAMESPACE

/** In certain cases we need to queue parameter changes for transferral between threads */
struct ParamTuple
{
  int idx;
  double value;
  
  ParamTuple(int idx = kNoParameter, double value = 0.)
  : idx(idx)
  , value(value)
  {}
};

/** This structure is used when queueing Sysex messages. You may need to set MAX_SYSEX_SIZE to reflect the max sysex payload in bytes */
struct SysExData
{
  SysExData(int offset = 0, int size = 0, const void* pData = 0)
  : mOffset(offset)
  , mSize(size)
  {
    assert(size < MAX_SYSEX_SIZE);
    
    if (pData)
      memcpy(mData, pData, size);
    else
      memset(mData, 0, MAX_SYSEX_SIZE);
  }
  
  int mOffset;
  int mSize;
  uint8_t mData[MAX_SYSEX_SIZE];
};

/** A helper class for IByteChunk and IByteStream that avoids code duplication **/
struct IByteGetter
{
  /** /todo 
   * @param pData /todo
   * @param dataSize /todo
   * @param pBuf /todo
   * @param size /todo
   * @param startPos /todo
   * @return int /todo */
  static inline int GetBytes(const uint8_t* pData, int dataSize, void* pBuf, int size, int startPos)
  {
    int endPos = startPos + size;
    if (startPos >= 0 && endPos <= dataSize)
    {
      memcpy(pBuf, pData + startPos, size);
      return endPos;
    }
    return -1;
  }
  
  /** /todo 
   * @param pData /todo
   * @param dataSize /todo
   * @param str /todo
   * @param startPos /todo
   * @return int /todo  */
  static inline int GetStr(const uint8_t* pData, int dataSize, WDL_String& str, int startPos)
  {
    int len;
    int strStartPos = GetBytes(pData, dataSize, &len, sizeof(len), startPos);
    if (strStartPos >= 0)
    {
      int strEndPos = strStartPos + len;
      if (strEndPos <= dataSize)
      {
        if (len > 0)
          str.Set((char*) (pData + strStartPos), len);
        else
          str.Set("");
      }
      return strEndPos;
    }
    return -1;
  }
};
  
/** Manages a block of memory, for plug-in settings store/recall */
class IByteChunk : private IByteGetter
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
   * @param position The position (in bytes) to start looking
   * @return The IPlug version number, retrieved from the chunk, or 0 if it failed */
  static int GetIPlugVerFromChunk(const IByteChunk& chunk, int& position)
  {
    int magic = 0, ver = 0;
    int magicpos = chunk.Get(&magic, position);
    
    if (magicpos > position && magic == IPLUG_VERSION_MAGIC)
      position = chunk.Get(&ver, magicpos);
    
    return ver;
  }
  
  /** Copies data into the chunk
   * @param pBuf Pointer to the object to copy data from
   * @param size Number of bytes to copy */
  inline int PutBytes(const void* pBuf, int size)
  {
    int n = mBytes.GetSize();
    mBytes.Resize(n + size);
    memcpy(mBytes.Get() + n, pBuf, size);
    return mBytes.GetSize();
  }
  
  /** /todo  
   * @param pBuf /todo
   * @param size /todo
   * @param startPos /todo
   * @return int /todo */
  inline int GetBytes(void* pBuf, int size, int startPos) const
  {
    return IByteGetter::GetBytes(mBytes.Get(), Size(), pBuf, size, startPos);
  }
  
  /** /todo 
   * @tparam T 
   * @param pVal /todo
   * @return int /todo */
  template <class T>
  inline int Put(const T* pVal)
  {
    return PutBytes(pVal, sizeof(T));
  }
  
  /** /todo 
   * @tparam T 
   * @param pVal /todo
   * @param startPos /todo
   * @return int /todo */
  template <class T>
  inline int Get(T* pVal, int startPos) const
  {
    return GetBytes(pVal, sizeof(T), startPos);
  }
  
  /** /todo 
   * @param str /todo
   * @return int /todo */
  inline int PutStr(const char* str)
  {
    int slen = (int) strlen(str);
    Put(&slen);
    return PutBytes(str, slen);
  }
  
  /** /todo 
   * @param str /todo
   * @param startPos /todo
   * @return int /todo */
  inline int GetStr(WDL_String& str, int startPos) const
  {
    return IByteGetter::GetStr(mBytes.Get(), Size(), str, startPos);
  }
  
  /** /todo 
   * @param pRHS /todo
   * @return int /todo */
  inline int PutChunk(const IByteChunk* pRHS)
  {
    return PutBytes(pRHS->GetData(), pRHS->Size());
  }
  
  /** Clears the chunk */
  inline void Clear()
  {
    mBytes.Resize(0);
  }
  
  /** Returns the current size of the chunk
   * @return Current size (in bytes) */
  inline int Size() const
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
  
  /** /todo 
   * @return uint8_t* /todo */
  inline uint8_t* GetData()
  {
    return mBytes.Get();
  }
  
  /** /todo 
   * @return const uint8_t* /todo */
  inline const uint8_t* GetData() const
  {
    return mBytes.Get();
  }
  
  /** /todo 
   * @param otherChunk /todo
   * @return true /todo
   * @return false /todo */
  inline bool IsEqual(IByteChunk& otherChunk) const
  {
    return (otherChunk.Size() == Size() && !memcmp(otherChunk.mBytes.Get(), mBytes.Get(), Size()));
  }
  
private:
  WDL_TypedBuf<uint8_t> mBytes;
};

/** Manages a non-owned block of memory, for receiving arbitrary message byte streams */
class IByteStream : private IByteGetter
{
public:
  IByteStream(const void *pData, int dataSize) : mBytes(reinterpret_cast<const uint8_t *>(pData)), mSize(dataSize) {}
  ~IByteStream() {}
  
  /** /todo 
   * @param pBuf /todo
   * @param size /todo
   * @param startPos /todo
   * @return int /todo */
  inline int GetBytes(void* pBuf, int size, int startPos) const
  {
    return IByteGetter::GetBytes(mBytes, Size(), pBuf, size, startPos);
  }
  
  /** /todo 
   * @tparam T 
   * @param pVal /todo
   * @param startPos /todo
   * @return int /todo */
  template <class T>
  inline int Get(T* pVal, int startPos) const
  {
    return GetBytes(pVal, sizeof(T), startPos);
  }
  
  /** /todo  
   * @param str /todo
   * @param startPos /todo
   * @return int /todo */
  inline int GetStr(WDL_String& str, int startPos) const
  {
    return IByteGetter::GetStr(mBytes, Size(), str, startPos);
  }
  
  /** Returns the  size of the chunk
   * @return  size (in bytes) */
  inline int Size() const
  {
    return mSize;
  }
  
  /** /todo  
   * @param otherStream /todo
   * @return true /todo
   * @return false /todo */
  inline bool IsEqual(IByteStream& otherStream) const
  {
    return (otherStream.Size() == Size() && !memcmp(otherStream.mBytes, mBytes, Size()));
  }
  
  /** /todo  
   * @return const uint8_t* /todo */
  inline const uint8_t* GetData()
  {
    return mBytes;
  }
  
private:
  const uint8_t* mBytes;
  int mSize;
};

/** Helper struct to set compile time options to an API class constructor  */
struct Config
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
  bool plugDoesMidiIn;
  bool plugDoesMidiOut;
  bool plugDoesMPE;
  bool plugDoesChunks;
  int plugType;
  bool plugHasUI;
  int plugWidth;
  int plugHeight;
  const char* bundleID;
  
  Config(int nParams,
              int nPresets,
              const char* channelIOStr,
              const char* pluginName,
              const char* productName,
              const char* mfrName,
              int vendorVersion,
              int uniqueID,
              int mfrID,
              int latency,
              bool plugDoesMidiIn,
              bool plugDoesMidiOut,
              bool plugDoesMPE,
              bool plugDoesChunks,
              int plugType,
              bool plugHasUI,
              int plugWidth,
              int plugHeight,
              const char* bundleID)
              
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
  , plugDoesMidiIn(plugDoesMidiIn)
  , plugDoesMidiOut(plugDoesMidiOut)
  , plugDoesMPE(plugDoesMPE)
  , plugDoesChunks(plugDoesChunks)
  , plugType(plugType)
  , plugHasUI(plugHasUI)
  , plugWidth(plugWidth)
  , plugHeight(plugHeight)
  , bundleID(bundleID)
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

/** Used to manage information about a bus such as whether it's an input or output, channel count and if it has a label */
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
  
  /** /todo 
   * @param direction /todo
   * @param NChans /todo
   * @param label /todo */
  void AddBusInfo(ERoute direction, int NChans, const char* label = "")
  {
    mBusInfo[direction].Add(new IBusInfo(direction, NChans, label));
  }
  
  /** /todo
   * @param direction /todo
   * @param index /todo
   * @return IBusInfo* /todo */
  IBusInfo* GetBusInfo(ERoute direction, int index)
  {
    assert(index >= 0 && index < mBusInfo[direction].GetSize());
    return mBusInfo[direction].Get(index);
  }
  
  /** /todo 
   * @param direction /todo
   * @param index /todo
   * @return int /todo */
  int NChansOnBusSAFE(ERoute direction, int index)
  {
    int NChans = 0;
    
    if(index >= 0 && index < mBusInfo[direction].GetSize())
      NChans = mBusInfo[direction].Get(index)->mNChans;

    return NChans;
  }
  
  /** /todo  
   * @param direction /todo
   * @return int /todo */
  int NBuses(ERoute direction)
  {
    return mBusInfo[direction].GetSize();
  }
  
  /** Get the total number of channels across all direction buses for this IOConfig
   * @param direction /todo
   * @return int /todo */
  int GetTotalNChannels(ERoute direction) const
  {
    int total = 0;
    
    for(int i = 0; i < mBusInfo[direction].GetSize(); i++)
      total += mBusInfo[direction].Get(i)->mNChans;
    
    return total;
  }
  
  /** /todo  
   * @param direction /todo
   * @return true /todo
   * @return false /todo */
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

/** Used for key press info, such as ASCII representation, virtual key (mapped to win32 codes) and modifiers */
struct IKeyPress
{
  int VK; // Windows VK_XXX
  char utf8[5] = { 0 }; // UTF8 key
  bool S, C, A; // SHIFT / CTRL(WIN) or CMD (MAC) / ALT

  /** /todo
   * @param _utf8 /todo
   * @param vk /todo
   * @param s /todo
   * @param c /todo
   * @param a /todo */
  IKeyPress(const char* _utf8, int vk, bool s = false, bool c = false, bool a = false)
    : VK(vk)
    , S(s), C(c), A(a)
  {
    strcpy(utf8, _utf8);
  }

  void DBGPrint() const { DBGMSG("VK: %i\n", VK); }
};

END_IPLUG_NAMESPACE

/**@}*/
