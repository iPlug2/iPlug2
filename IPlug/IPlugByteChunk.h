#pragma once

#include "wdlendian.h"

#include "IPlugTypes.h"
#include "IPlugUtilities.h"

template <class T> inline void SWAP(T& a, T& b)
{
  T tmp = a; a = b; b = tmp;
}

/** Manages a block of memory, for plug-in settings store/recall */
class ByteChunk
{
public:
  ByteChunk() {}
  ~ByteChunk() {}
  
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
  
//   inline int PutDoubleArray(const double* data, const int numItems)
//   {
//     Put(&numItems);
//     int n = mBytes.GetSize();
//     mBytes.Resize(n + numItems * sizeof(double));
//     memcpy(mBytes.Get() + n, (BYTE*) data, numItems * sizeof(double));
//     return mBytes.GetSize();
//   }
//   
//   inline int GetDoubleArray(double* data, int startPos)
//   {
//     int len;
//     int dStartPos = Get(&len, startPos);
//     if (dStartPos >= 0)
//     {
//       int dEndPos = dStartPos + (len * sizeof(double));
//       if (dEndPos <= mBytes.GetSize() && len > 0)
//       {
//         memcpy(data, mBytes.Get() + dStartPos, len * sizeof(double));
//       }
//       return dEndPos;
//     }
//     return -1;
//   }
  
  inline int PutBool(bool b)
  {
    int n = mBytes.GetSize();
    mBytes.Resize(n + 1);
    *(mBytes.Get() + n) = (BYTE) (b ? 1 : 0);
    return mBytes.GetSize();
  }
  
  inline int GetBool(bool* pB, int startPos)
  {
    int endPos = startPos + 1;
    if (startPos >= 0 && endPos <= mBytes.GetSize())
    {
      BYTE byt = *(mBytes.Get() + startPos);
      *pB = (byt);
      return endPos;
    }
    return -1;
  }
  
  inline int PutChunk(ByteChunk* pRHS)
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
  
  inline BYTE* GetBytes()
  {
    return mBytes.Get();
  }
  
  inline bool IsEqual(ByteChunk* pRHS)
  {
    return (pRHS && pRHS->Size() == Size() && !memcmp(pRHS->GetBytes(), GetBytes(), Size()));
  }
  
private:
  WDL_TypedBuf<unsigned char> mBytes;
};
