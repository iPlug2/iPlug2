#pragma once

#include <cmath>
#include <cstdio>

#include "IPlugConstants.h"
#include "IPlugOSDetect.h"

#ifdef OS_WIN
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#undef WINVER
#define WINVER 0x0501
#pragma warning(disable:4018 4267)	// size_t/signed/unsigned mismatch..
#pragma warning(disable:4800)		// if (pointer) ...
#pragma warning(disable:4805)		// Compare bool and BOOL.
#endif

/**
 * @file
 * Utility functions and macros
 */

#define FREE_NULL(p) {free(p);p=nullptr;}
#define DELETE_NULL(p) {delete(p); p=nullptr;}
#define DELETE_ARRAY(p) {delete[](p); (p)=nullptr;}

/** Clamps the value \p x between \p lo and \p hi
 * @param x Input value
 * @param lo Minimum value to be allowed
 * @param hi Maximum value to be allowed
 * If \p x is outside given range, it will be set to one of the boundaries
*/
#define BOUNDED(x,lo,hi) ((x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))

#define CSTR_NOT_EMPTY(cStr) ((cStr) && (cStr)[0] != '\0')

#define MAKE_QUOTE(str) #str
#define MAKE_STR(str) MAKE_QUOTE(str)

/** @hideinitializer */
#define GET_PARAM_FROM_VARARG(paramType, vp, v) \
{ \
  v = 0.0; \
  switch (paramType) { \
    case IParam::kTypeBool: \
    case IParam::kTypeInt: \
    case IParam::kTypeEnum: { \
      v = (double) va_arg(vp, int); \
      break; \
    } \
    case IParam::kTypeDouble: \
    default: { \
      v = (double) va_arg(vp, double); \
      break; \
    } \
  } \
}

/** @brief Calculates gain from a given dB value
 * @param dB Value in dB
 * @return Gain calculated as an approximation of
 * \f$ 10^{\frac{x}{20}} \f$
 * @see #IAMP_DB
 */
inline double DBToAmp(double dB)
{
  return exp(IAMP_DB * dB);
}

/**
 * @return dB calculated as an approximation of
 * \f$ 20*log_{10}(x) \f$
 * @see #AMP_DB
 */
inline double AmpToDB(double amp)
{
  return AMP_DB * log(fabs(amp));
}

inline void GetVersionParts(int version, int& ver, int& maj, int& min)
{
  ver = (version & 0xFFFF0000) >> 16;
  maj = (version & 0x0000FF00) >> 8;
  min = version & 0x000000FF;
}

inline int GetDecimalVersion(int version)
{
  int ver, rmaj, rmin;
  GetVersionParts(version, ver, rmaj, rmin);
  return 10000 * ver + 100 * rmaj + rmin;
}

inline void GetVersionStr(int version, WDL_String& str)
{
  int ver, rmaj, rmin;
  GetVersionParts(version, ver, rmaj, rmin);
  str.SetFormatted(MAX_VERSION_STR_LEN, "v%d.%d.%d", ver, rmaj, rmin);
}

inline double ToNormalizedParam(double nonNormalizedValue, double min, double max, double shape)
{
  return std::pow((nonNormalizedValue - min) / (max - min), 1.0 / shape);
}

inline double FromNormalizedParam(double normalizedValue, double min, double max, double shape)
{
  return min + std::pow((double) normalizedValue, shape) * (max - min);
}

template <class SRC, class DEST>
void CastCopy(DEST* pDest, SRC* pSrc, int n)
{
  for (int i = 0; i < n; ++i, ++pDest, ++pSrc)
  {
    *pDest = (DEST) *pSrc;
  }
}

#ifndef REMINDER
  #ifdef OS_WIN
    // This enables: #pragma REMINDER("change this line!") with click-through from VC++.
    #define REMINDER(msg) message(__FILE__   "(" MAKE_STR(__LINE__) "): " msg)
  #elif defined __APPLE__
    #define REMINDER(msg) WARNING msg
  #endif
#endif
