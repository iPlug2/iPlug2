#pragma once

#include <algorithm>
#include "wdlstring.h"

#include "IPlugConstants.h"
#include "IPlugPlatform.h"
#include "IPlugMidi.h" // <- Midi related structs in here

/** Manages a block of memory, for plug-in settings store/recall */
class IByteChunk
{
public:
  IByteChunk() {}
  ~IByteChunk() {}
  
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
  
  inline int GetBytes(void* pBuf, int size, int startPos)
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
  
  template <class T> inline int Get(T* pVal, int startPos)
  {
    return GetBytes(pVal, sizeof(T), startPos);
  }
  
  inline int PutStr(const char* str)
  {
    int slen = (int) strlen(str);
    Put(&slen);
    return PutBytes(str, slen);
  }
  
  inline int GetStr(WDL_String& str, int startPos)
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
  
  inline int PutBool(bool b)
  {
    int n = mBytes.GetSize();
    mBytes.Resize(n + 1);
    *(mBytes.Get() + n) = (uint8_t) (b ? 1 : 0);
    return mBytes.GetSize();
  }
  
  inline int GetBool(bool* pB, int startPos)
  {
    int endPos = startPos + 1;
    if (startPos >= 0 && endPos <= mBytes.GetSize())
    {
      uint8_t byt = *(mBytes.Get() + startPos);
      *pB = (byt);
      return endPos;
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
  
  /** Resizes the chunk
   * @param newSize Desired size (in bytes)
   * @todo Check this
   * @return Old size (in bytes)
   */
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
  
  inline uint8_t* GetBytes()
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
  const char* effectName;
  const char* productName;
  const char* mfrName;
  int vendorVersion;
  int uniqueID;
  int mfrID;
  int latency;
  bool plugDoesMidi;
  bool plugDoesChunks;
  bool plugIsInstrument;
  
  IPlugConfig(int nParams,
              int nPresets,
              const char* channelIOStr,
              const char* effectName,
              const char* productName,
              const char* mfrName,
              int vendorVersion,
              int uniqueID,
              int mfrID,
              int latency,
              bool plugDoesMidi,
              bool plugDoesChunks,
              bool plugIsInstrument)
              
  : nParams(nParams)
  , nPresets(nPresets)
  , channelIOStr(channelIOStr)
  , effectName(effectName)
  , productName(productName)
  , mfrName(mfrName)
  , vendorVersion(vendorVersion)
  , uniqueID(uniqueID)
  , mfrID(mfrID)
  , latency(latency)
  , plugDoesMidi(plugDoesMidi)
  , plugDoesChunks(plugDoesChunks)
  , plugIsInstrument(plugIsInstrument)
  {};
};

/**
 Used to manage scratch buffers for each channel of I/O, which may involve converting from single to double precision
 */
template<class TIN  = float, class TOUT = double>
struct ChannelData
{
  bool mConnected = false;
  TOUT** mData = nullptr; // If this is for an input channel, points into IPlugBase::mInData, if it's for an output channel points into IPlugBase::mOutData
  TIN* mIncomingData = nullptr;
  WDL_TypedBuf<TOUT> mScratchBuf;
  WDL_String mLabel = WDL_String("");
};

struct BusInfo
{
  int mNChans;
  WDL_String mLabel;
  
  BusInfo(int nchans = 0, const char* label = "")
  : mNChans(nchans)
  {
    mLabel.Set(label);
  }
};

/** An IOConfig is used to store bus info for each input/output configuration defined in the channel io string */
struct IOConfig
{
  WDL_PtrList<BusInfo> mInputBusInfo; // A particular valid io config may have multiple input buses
  WDL_PtrList<BusInfo> mOutputBusInfo; // or multiple output busses
  
  ~IOConfig()
  {
    mInputBusInfo.Empty(true);
    mOutputBusInfo.Empty(true);
  }
  
  void AddInputBusInfo(int NChans, const char* label = "input")
  {
    mInputBusInfo.Add(new BusInfo(NChans, label));
  }
  
  void AddOutputBusInfo(int NChans, const char* label = "output")
  {
    mOutputBusInfo.Add(new BusInfo(NChans, label));
  }

  BusInfo* GetInputBusInfo(int index)
  {
    assert(index >= 0 && index < mInputBusInfo.GetSize());

    return mInputBusInfo.Get(index);
  }
  
  BusInfo* GetOutputBusInfo(int index)
  {
    assert(index >= 0 && index < mOutputBusInfo.GetSize());
  
    return mOutputBusInfo.Get(index);
  }
  
  int NChansOnInputBusSAFE(int index)
  {
    if(index >= 0 && index < mInputBusInfo.GetSize())
      return mInputBusInfo.Get(index)->mNChans;
    else
      return 0;
  }
  
  int NChansOnOutputBusSAFE(int index)
  {
    if(index >= 0 && index < mOutputBusInfo.GetSize())
      return mOutputBusInfo.Get(index)->mNChans;
    else
      return 0;
  }
  
  int NInputBuses() { return mInputBusInfo.GetSize(); }
  int NOutputBuses() { return mOutputBusInfo.GetSize(); }

  /** Get the total number of input channels across all input buses for this IOConfig */
  int GetTotalNInputChannels() const
  {
    int total = 0;
    
    for(int i = 0; i < mInputBusInfo.GetSize(); i++)
      total += mInputBusInfo.Get(i)->mNChans;
    
    return total;
  }
  
  /** Get the total number of output channels across all output buses for this IOConfig */
  int GetTotalNOutputChannels() const
  {
    int total = 0;
    
    for(int i = 0; i < mOutputBusInfo.GetSize(); i++)
      total += mOutputBusInfo.Get(i)->mNChans;
    
    return total;
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
