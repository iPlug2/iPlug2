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

/** Used to store channel i/o count together per bus */
struct ChannelIO
{
  WDL_TypedBuf<int> mInputBuses;
  WDL_TypedBuf<int> mOutputBuses;
  
  void AddInputBus(int NChans) { mInputBuses.Add(NChans); }
  void AddOutputBus(int NChans) { mOutputBuses.Add(NChans); }
  
  int NChansOnInputBus(int busIdx)
  {
    assert(busIdx < mInputBuses.GetSize());
    return mInputBuses.Get()[busIdx];
  }

  int NChansOnOutputBus(int busIdx)
  {
    assert(busIdx < mOutputBuses.GetSize());
    return mOutputBuses.Get()[busIdx];
  }
  
  int GetTotalNInputChannels() const
  {
    int total = 0;
    
    for(int i = 0; i < mInputBuses.GetSize(); i++)
      total += mInputBuses.Get()[i];
    
    return total;
  }
  
  int GetTotalNOutputChannels() const
  {
    int total = 0;
    
    for(int i = 0; i < mOutputBuses.GetSize(); i++)
      total += mOutputBuses.Get()[i];
    
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
