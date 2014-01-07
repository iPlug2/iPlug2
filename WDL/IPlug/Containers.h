#ifndef _CONTAINERS_
#define _CONTAINERS_

#ifdef WIN32
  #undef _WIN32_WINNT
  #define _WIN32_WINNT 0x0501
  #undef WINVER
  #define WINVER 0x0501
  #pragma warning(disable:4018 4267)	// size_t/signed/unsigned mismatch..
  #pragma warning(disable:4800)		// if (pointer) ...
  #pragma warning(disable:4805)		// Compare bool and BOOL.
#endif

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "../mutex.h"
#include "../wdlstring.h"
#include "../ptrlist.h"
#include "../wdlendian.h"

#define FREE_NULL(p) {free(p);p=0;}
#define DELETE_NULL(p) {delete(p); p=0;}
#define DELETE_ARRAY(p) {delete[](p); (p)=0;}
#define IPMIN(x,y) ((x)<(y)?(x):(y))
#define IPMAX(x,y) ((x)<(y)?(y):(x))
#define BOUNDED(x,lo,hi) ((x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))
#define CSTR_NOT_EMPTY(cStr) ((cStr) && (cStr)[0] != '\0')

#define MAKE_QUOTE(str) #str
#define MAKE_STR(str) MAKE_QUOTE(str)

#define PI 3.141592653589793238
#define AMP_DB 8.685889638065036553
#define IAMP_DB 0.11512925464970

inline double DBToAmp(double dB)
{
  return exp(IAMP_DB * dB);
}

inline double AmpToDB(double amp)
{
  return AMP_DB * log(fabs(amp));
}

#ifndef REMINDER
  #if defined WIN32
  // This enables: #pragma REMINDER("change this line!") with click-through from VC++.
    #define REMINDER(msg) message(__FILE__   "(" MAKE_STR(__LINE__) "): " msg)
  #elif defined __APPLE__
    #define REMINDER(msg) WARNING msg
  #endif
#endif

template <class T> inline void SWAP(T& a, T& b)
{
  T tmp = a; a = b; b = tmp;
}

typedef unsigned char BYTE;
class ByteChunk
{
public:
  ByteChunk() {}
  ~ByteChunk() {}

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

// Handle endian conversion for integer and floating point data types.
// Data is always stored in the chunk in little endian format, so nothing needs
//  changing on Intel x86 platforms.

#ifdef WDL_BIG_ENDIAN
  inline int Put(const unsigned short* pVal)
  {
    unsigned short i = WDL_bswap16_if_be(*pVal);
    return PutBytes(&i, 2);
  }

  inline int Get(unsigned short* pVal, int startPos)
  {
    startPos = GetBytes(pVal, 2, startPos);
    WDL_BSWAP16_IF_BE(*pVal);
    return startPos;
  }

  inline int Put(const unsigned int* pVal)
  {
    unsigned int i = WDL_bswap32_if_be(*pVal);
    return PutBytes(&i, 4);
  }

  inline int Get(unsigned int* pVal, int startPos)
  {
    startPos = GetBytes(pVal, 4, startPos);
    WDL_BSWAP32_IF_BE(*pVal);
    return startPos;
  }

  inline int Put(const WDL_UINT64* pVal)
  {
    WDL_UINT64 i = WDL_bswap64_if_be(*pVal);
    return PutBytes(&i, 8);
  }

  inline int Get(WDL_UINT64* pVal, int startPos)
  {
    startPos = GetBytes(pVal, 8, startPos);
    WDL_BSWAP64_IF_BE(*pVal);
    return startPos;
  }

  // Signed
  inline int Put(const short*     pVal) { return Put((const unsigned short*) pVal); }
  inline int Put(const int*       pVal) { return Put((const unsigned int*)   pVal); }
  inline int Put(const WDL_INT64* pVal) { return Put((const WDL_UINT64*)     pVal); }

  inline int Get(short*     pVal, int startPos) { return Get((unsigned short*) pVal, startPos); }
  inline int Get(int*       pVal, int startPos) { return Get((unsigned int*)   pVal, startPos); }
  inline int Get(WDL_INT64* pVal, int startPos) { return Get((WDL_UINT64*)     pVal, startPos); }

  // Floats
  inline int Put(const float* pVal)
  {
    unsigned int i = WDL_bswapf_if_be(*pVal);
    return PutBytes(&i, 4);
  }

  inline int Get(float* pVal, int startPos)
  {
    unsigned int i;
    startPos = GetBytes(&i, 4, startPos);
    *pVal = WDL_bswapf_if_be(i);
    return startPos;
  }

  inline int Put(const double* pVal)
  {
    WDL_UINT64 i = WDL_bswapf_if_be(*pVal);
    return PutBytes(&i, 8);
  }

  inline int Get(double* pVal, int startPos)
  {
    WDL_UINT64 i;
    startPos = GetBytes(&i, 8, startPos);
    *pVal = WDL_bswapf_if_be(i);
    return startPos;
  }

#endif // WDL_BIG_ENDIAN

  inline int PutStr(const char* str)
  {
    int slen = strlen(str);
    #ifdef WDL_BIG_ENDIAN
    { const unsigned int i = WDL_bswap32_if_be(slen); Put(&i); }
    #else
    Put(&slen);
    #endif
    return PutBytes(str, slen);
  }

  inline int GetStr(WDL_String* pStr, int startPos)
  {
    int len;
    int strStartPos = Get(&len, startPos);
    if (strStartPos >= 0)
    {
      WDL_BSWAP32_IF_BE(len);
      int strEndPos = strStartPos + len;
      if (strEndPos <= mBytes.GetSize() && len > 0)
      {
        pStr->Set((char*) (mBytes.Get() + strStartPos), len);
      }
      return strEndPos;
    }
    return -1;
  }

  inline int PutDoubleArray(const double* data, const int numItems)
  {
    Put(&numItems);
    int n = mBytes.GetSize();
    mBytes.Resize(n + numItems * sizeof(double));
    memcpy(mBytes.Get() + n, (BYTE*) data, numItems * sizeof(double));
    return mBytes.GetSize();
  }

  inline int GetDoubleArray(double* data, int startPos)
  {
    int len;
    int dStartPos = Get(&len, startPos);
    if (dStartPos >= 0)
    {
      int dEndPos = dStartPos + len;
      if (dEndPos <= mBytes.GetSize() && len > 0)
      {
        memcpy(data, mBytes.Get() + dStartPos, len * sizeof(double));
      }
      return dEndPos;
    }
    return -1;
  }

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

  inline void Clear()
  {
    mBytes.Resize(0);
  }

  inline int Size()
  {
    return mBytes.GetSize();
  }

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

#endif
