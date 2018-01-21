#pragma once

#include <algorithm>
#include "wdlstring.h"

#include "IPlugConstants.h"
#include "IPlugOSDetect.h"
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
  
  inline int GetStr(WDL_String* pStr, int startPos)
  {
    int len;
    int strStartPos = Get(&len, startPos);
    if (strStartPos >= 0)
    {
      int strEndPos = strStartPos + len;
      if (strEndPos <= mBytes.GetSize())
      {
        if (len > 0)
          pStr->Set((char*) (mBytes.Get() + strStartPos), len);
        else
          pStr->Set("");
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
  
  inline bool IsEqual(IByteChunk* pRHS)
  {
    return (pRHS && pRHS->Size() == Size() && !memcmp(pRHS->GetBytes(), GetBytes(), Size()));
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
  int plugScChans;
  
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
              bool plugIsInstrument,
              int plugScChans)
              
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
  , plugScChans(plugScChans)
  {};
};

/** Used to store channel i/o count together */
struct ChannelIO
{
  int mIn, mOut;
  ChannelIO(int nIn, int nOut) : mIn(nIn), mOut(nOut) {}
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
