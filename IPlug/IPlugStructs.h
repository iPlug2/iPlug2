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

/** A helper class for IByteChunk and IByteStream that avoids code duplication */
struct IByteGetter
{
  /** Copy raw bytes from a byte array, returning the new position for subsequent calls
   * @param pSrc The source buffer
   * @param dstSize The size of the source data in bytes
   * @param pDst The destination buffer
   * @param nBytesToCopy The number of bytes to copy from pSrc
   * @param startPos The starting position in bytes in pSrc
   * @return int The end position in bytes after the copy, or -1 if the copy would have copied more data than in the src buffer  */
  static inline int GetBytes(const uint8_t* pSrc, int srcSize, void* pDst, int nBytesToCopy, int startPos)
  {
    int endPos = startPos + nBytesToCopy;
    if (startPos >= 0 && endPos <= srcSize)
    {
      memcpy(pDst, pSrc + startPos, nBytesToCopy);
      return endPos;
    }
    return -1;
  }
  
  /** Get a string from a byte array, to a WDL_String, returning the new position for subsequent calls
   * @param pSrc The source buffer
   * @param dstSize The size of the source data in bytes
   * @param str WDL_String to fill with the extracted string
   * @param startPos The starting position in bytes in pSrc
   * @return int The end position in bytes after the copy, or -1 if the copy would have copied more data than in the src buffer  */
  static inline int GetStr(const uint8_t* pSrc, int srcSize, WDL_String& str, int startPos)
  {
    int len;
    int strStartPos = GetBytes(pSrc, srcSize, &len, sizeof(len), startPos);
    if (strStartPos >= 0)
    {
      int strEndPos = strStartPos + len;
      if (strEndPos <= srcSize)
      {
        if (len > 0)
          str.Set((char*) (pSrc + strStartPos), len);
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
  
  /** Copies data into the chunk, placing it at the end, resizing if nessecary
   * @param pSrc Pointer to the data to copy
   * @param nBytesToCopy Number of bytes to copy
   * @return int The size of the chunk after insertion  */
  inline int PutBytes(const void* pSrc, int nBytesToCopy)
  {
    int n = mBytes.GetSize();
    mBytes.Resize(n + nBytesToCopy);
    memcpy(mBytes.Get() + n, pSrc, nBytesToCopy);
    return mBytes.GetSize();
  }
  
  /** Copy raw bytes from the IByteChunk, returning the new position for subsequent calls
   * @param pDst The destination buffer
   * @param nBytesToCopy The number of bytes to copy from the chunk
   * @param startPos The starting position in bytes in the chunk
   * @return int The end position in the chunk (in bytes) after the copy, or -1 if the copy would have copied more data than in the chunk  */
  inline int GetBytes(void* pDst, int nBytesToCopy, int startPos) const
  {
    return IByteGetter::GetBytes(mBytes.Get(), Size(), pDst, nBytesToCopy, startPos);
  }
  
  /** Copies arbitary typed data into the IByteChunk
   * @tparam T The type of data to be stored
   * @param pVal Ptr to the data to be stored
   * @return int The size of the chunk after insertion  */
  template <class T>
  inline int Put(const T* pVal)
  {
    return PutBytes(pVal, sizeof(T));
  }
  
  /** Get arbitary typed data from the IByteChunk
   * @tparam T The type of data to be extracted
   * @param pDst Ptr to the destination where the data will be extracted
   * @param startPos The starting position in bytes in the chunk
   * @return int The end position in the chunk (in bytes) after the copy, or -1 if the copy would have copied  more data than in the chunk  */
  template <class T>
  inline int Get(T* pDst, int startPos) const
  {
    return GetBytes(pDst, sizeof(T), startPos);
  }
  
  /** Put a string into the IByteChunk
   * @param str CString to insert into the chunk
   * @return int The size of the chunk after insertion  */
  inline int PutStr(const char* str)
  {
    int slen = (int) strlen(str);
    Put(&slen);
    return PutBytes(str, slen);
  }
  
  /** Get a string from the IByteChunk
   * @param str WDL_String to fill
   * @param startPos The starting position in bytes in the chunk
   * @return int The end position in the chunk (in bytes) after the copy, or -1 if the copy would have copied  more data than in the chunk  */
  inline int GetStr(WDL_String& str, int startPos) const
  {
    return IByteGetter::GetStr(mBytes.Get(), Size(), str, startPos);
  }
  
  /** Put another IByteChunk into this one
   * @param pRHS Ptr to the IByteChunk to copy in
   * @return int The size of the chunk after insertion  */
  inline int PutChunk(const IByteChunk* pRHS)
  {
    return PutBytes(pRHS->GetData(), pRHS->Size());
  }
  
  /** Clears the chunk (resizes to 0) */
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
  
  /** Resizes the chunk
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
  
  /** Gets a ptr to the chunk data
   * @return uint8_t* Ptr to the chunk data */
  inline uint8_t* GetData()
  {
    return mBytes.Get();
  }
  
  /** Gets a const ptr to the chunk data
   * @return const uint8_t* const Ptr to the chunk data */
  inline const uint8_t* GetData() const
  {
    return mBytes.Get();
  }
  
  /** Compares the size & values of the data of another chunk with this one
   * @param otherChunk The chunk to compare with
   * @return \c true if the chunks are equal */
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
  
  /** Copy raw bytes from the stream, returning the new position for subsequent calls
   * @param pDst The destination buffer
   * @param nBytesToCopy The number of bytes to copy from the stream
   * @param startPos The starting position in bytes in the stream
   * @return int The end position in the stream (in bytes) after the copy, or -1 if the copy would have copied more data than in the stream  */
  inline int GetBytes(void* pDst, int nBytesToCopy, int startPos) const
  {
    return IByteGetter::GetBytes(mBytes, Size(), pDst, nBytesToCopy, startPos);
  }
  
  /** Get arbitary typed data from the stream
   * @tparam T The type of data to be extracted
   * @param pDst Ptr to the destination where the data will be extracted
   * @param startPos The starting position in bytes in the stream
   * @return int The end position in the stream (in bytes) after the copy, or -1 if the copy would have copied  more data than in the stream  */
  template <class T>
  inline int Get(T* pDst, int startPos) const
  {
    return GetBytes(pDst, sizeof(T), startPos);
  }
  
  /** Get a string from the stream
   * @param str WDL_String to fill
   * @param startPos The starting position in bytes in the stream
   * @return int The end position in the stream (in bytes) after the copy, or -1 if the copy would have copied  more data than in the stream  */
  inline int GetStr(WDL_String& str, int startPos) const
  {
    return IByteGetter::GetStr(mBytes, Size(), str, startPos);
  }
  
  /** Returns the  size of the stream
   * @return size (in bytes) */
  inline int Size() const
  {
    return mSize;
  }
  
  /** Compares the size & values of the data of another stream with this one
   * @param otherChunk The stream to compare with
   * @return \c true if the streams are equal */
  inline bool IsEqual(IByteStream& otherStream) const
  {
    return (otherStream.Size() == Size() && !memcmp(otherStream.mBytes, mBytes, Size()));
  }
  
  /** Gets a const ptr to the stream data
   * @return uint8_t* const ptr to the stream data */
  inline const uint8_t* GetData()
  {
    return mBytes;
  }
  
private:
  const uint8_t* mBytes;
  int mSize;
};

/** Helper class to maintain a read position whilst extracting data from an IByteChunk  */
class IByteChunkReader
{
public:
  IByteChunkReader(const IByteChunk& chunk, int startPos = 0)
  : mChunk(chunk)
  , mPos(startPos)
  {
  }
  
  /** Copy \c nBytesToCopy bytes from the managed IByteChunk into \c pBuf .
   * @param pBuf Destination buffer
   * @param nBytesToCopy Number of bytes to copy
   * @return Next read position in the IByteChunk */
  inline int GetBytes(void* pBuf, int nBytesToCopy)
  {
    mPos = mChunk.GetBytes(pBuf, nBytesToCopy, mPos);
    return mPos;
  }
  
  /** Copy arbitary typed data out of the managed IByteChunk at the current position and update the position
   * @tparam T type of the variable to get
   * @param pDst Pointer to the destination where the value will be stored
   * @return int Next read position in the IByteChunk */
  template <class T>
  inline int Get(T* pDst)
  {
    mPos = mChunk.Get(pDst, mPos);
    return mPos;
  }
  
  /** Retrieve a string from the managed IByteChunk and put it in \c str .
   * @param str Destination for the string
   * @return int Next read position in the IByteChunk */
  inline int GetStr(WDL_String& str)
  {
    mPos = mChunk.GetStr(str, mPos);
    return mPos;
  }
  
  /** Return the current position in the managed IByteChunk
   * @return The current position in the IByteChunk */
  inline int Tell() const
  {
    return mPos;
  }

  /** Set the current position in the managed IByteChunk.
   * @param pos The new IByteChunk position */
  inline void Seek(int pos)
  {
    mPos = pos;
  }

private:
  const IByteChunk& mChunk;
  int mPos;
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
  int plugMinWidth;
  int plugMaxWidth;
  int plugMinHeight;
  int plugMaxHeight;
  bool plugHostResize;
  const char* bundleID;
  const char* appGroupID;
  
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
         bool plugHostResize,
         int plugMinWidth,
         int plugMaxWidth,
         int plugMinHeight,
         int plugMaxHeight,
         const char* bundleID,
         const char* appGroupID)
              
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
  , plugMinWidth(plugMinWidth)
  , plugMaxWidth(plugMaxWidth)
  , plugMinHeight(plugMinHeight)
  , plugMaxHeight(plugMaxHeight)
  , plugHostResize(plugHostResize)
  , bundleID(bundleID)
  , appGroupID(appGroupID)
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
  WDL_String mLabel;
};

/** Used to manage information about a bus such as whether it's an input or output, channel count */
class IBusInfo
{
public:
  IBusInfo(ERoute direction, int nchans = 0)
  : mDirection(direction)
  , mNChans(nchans)
  {
  }
  
  int NChans() const { return mNChans; }

  ERoute GetDirection() const { return mDirection; }

private:
  ERoute mDirection;
  int mNChans;
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
  
  /** \todo 
   * @param direction \todo
   * @param NChans \todo
   * @param label \todo */
  void AddBusInfo(ERoute direction, int NChans)
  {
    mBusInfo[direction].Add(new IBusInfo(direction, NChans));
  }
  
  /** \todo
   * @param direction \todo
   * @param index \todo
   * @return IBusInfo* \todo */
  const IBusInfo* GetBusInfo(ERoute direction, int index) const
  {
    assert(index >= 0 && index < mBusInfo[direction].GetSize());
    return mBusInfo[direction].Get(index);
  }
  
  /** \todo 
   * @param direction \todo
   * @param index \todo
   * @return int \todo */
  int NChansOnBusSAFE(ERoute direction, int index) const
  {
    int NChans = 0;
    
    if(index >= 0 && index < mBusInfo[direction].GetSize())
      NChans = mBusInfo[direction].Get(index)->NChans();

    return NChans;
  }
  
  /** \todo  
   * @param direction \todo
   * @return int \todo */
  int NBuses(ERoute direction) const
  {
    return mBusInfo[direction].GetSize();
  }
  
  /** Get the total number of channels across all direction buses for this IOConfig
   * @param direction \todo
   * @return int \todo */
  int GetTotalNChannels(ERoute direction) const
  {
    int total = 0;
    
    for(int i = 0; i < mBusInfo[direction].GetSize(); i++)
      total += mBusInfo[direction].Get(i)->NChans();
    
    return total;
  }
  
  /** \todo  
   * @param direction \todo
   * @return true \todo
   * @return false \todo */
  bool ContainsWildcard(ERoute direction) const
  {
    for(auto i = 0; i < mBusInfo[direction].GetSize(); i++)
    {
      if(mBusInfo[direction].Get(i)->NChans() < 0)
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
    snprintf(mName, MAX_PRESET_NAME_LEN, "%s", UNUSED_PRESET_NAME);
  }
};

/** Used for key press info, such as ASCII representation, virtual key (mapped to win32 codes) and modifiers */
struct IKeyPress
{
  int VK; // Windows VK_XXX
  char utf8[5] = { 0 }; // UTF8 key
  bool S, C, A; // SHIFT / CTRL(WIN) or CMD (MAC) / ALT

  /** IKeyPress Constructor
   * @param _utf8 UTF8 key
   * @param vk Windows Virtual Key
   * @param s Is SHIFT modifier pressed
   * @param c Is CTRL/CMD modifier pressed
   * @param a Is ALT modifier pressed */
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
