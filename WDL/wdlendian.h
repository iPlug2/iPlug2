/*
  WDL - wdlendian.h
  (c) Theo Niessink 2011
  <http://www.taletn.com/>

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.


  This file provides macros and functions for converting integer and
  floating point data types from native (host) endian to little or big
  endian format, and vice versa.

*/


#ifndef _WDL_ENDIAN_H_
#define _WDL_ENDIAN_H_


#include "wdltypes.h"

typedef union { float  f; unsigned int int32; } WDL_EndianFloat;
typedef union { double f; WDL_UINT64   int64; } WDL_EndianDouble;

#ifdef __cplusplus
	#define WDL_ENDIAN_INLINE inline
#elif defined(_MSC_VER)
	#define WDL_ENDIAN_INLINE __inline
#else
	#define WDL_ENDIAN_INLINE
#endif


// Windows
#ifdef _WIN32
#define WDL_LITTLE_ENDIAN

// Mac OS X
#elif defined(__APPLE__)
#if __LITTLE_ENDIAN__ // Intel
	#define WDL_LITTLE_ENDIAN
#elif __BIG_ENDIAN__ // PowerPC
	#define WDL_BIG_ENDIAN
#else
	#error Unknown endian
#endif

// GNU C (v4.6 or later?)
#elif __GNUC__ && defined(__BYTE_ORDER__)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	#define WDL_LITTLE_ENDIAN
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	#define WDL_BIG_ENDIAN
#else
	#error Unsupported endian
#endif
#if __FLOAT_WORD_ORDER__ != __BYTE_ORDER__
	#error Unsupported float endian
#endif

// GNU C, Intel C++
#elif defined(__i386) || defined(__i386__) || defined(i386)
#define WDL_LITTLE_ENDIAN

// Intel C++
#elif defined(__x86_64) || defined(__x86_64__)
#define WDL_LITTLE_ENDIAN

#else
#error Unknown endian
#endif


// Microsoft C
#ifdef _MSC_VER
#include <intrin.h>
#define WDL_bswap16(x) _byteswap_ushort(x)
#define WDL_bswap32(x) _byteswap_ulong(x)
#define WDL_bswap64(x) _byteswap_uint64(x)

#elif defined(__APPLE__)
  #include "TargetConditionals.h"
  #if defined(TARGET_OS_IPHONE) | defined(TARGET_IPHONE_SIMULATOR)
    #include <libkern/OSByteOrder.h>
    
    // iOS
    #define WDL_bswap16(x) OSSwapInt16(x)
    #define WDL_bswap32(x) OSSwapInt32(x)
    #define WDL_bswap64(x) OSSwapInt64(x)

  // Mac OS X (v10.0 and later)
  #elif defined(TARGET_OS_MAC)
    #include <CoreServices/CoreServices.h>

    #define WDL_bswap16(x) Endian16_Swap(x)
    #define WDL_bswap32(x) Endian32_Swap(x)
    #define WDL_bswap64(x) Endian64_Swap(x) // (Thread-safe on) v10.3 and later (?)
#endif


// GNU C
#elif __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
#define WDL_bswap32(x) __builtin_bswap32(x)
#define WDL_bswap64(x) __builtin_bswap64(x)

// Linux
#elif defined(__linux) || defined(__linux__) || defined(linux)
#include <endian.h>
#define WDL_bswap16(x) bswap16(x)
#define WDL_bswap32(x) bswap32(x)
#define WDL_bswap64(x) bswap64(x)

#endif // WDL_bswapXX

// If none of the supported intrinsics were found, then revert to generic C
// byte swap functions.
#ifndef WDL_bswap16
static WDL_ENDIAN_INLINE unsigned short WDL_bswap16(const unsigned short int16)
{
	return int16 >> 8 |
	       int16 << 8;
}
#endif

#ifndef WDL_bswap32
static WDL_ENDIAN_INLINE unsigned int WDL_bswap32(const unsigned int int32)
{
	return int32 >> 24 |
	       int32 >> 8  & 0x0000FF00 |
	       int32 << 8  & 0x00FF0000 |
	       int32 << 24;
}
#endif

#ifndef WDL_bswap64
static WDL_ENDIAN_INLINE WDL_UINT64 WDL_bswap64(const WDL_UINT64 int64)
{
	return int64 >> 56 |
	       int64 >> 40 & 0x000000000000FF00ULL |
	       int64 >> 24 & 0x0000000000FF0000ULL |
	       int64 >> 8  & 0x00000000FF000000ULL |
	       int64 << 8  & 0x000000FF00000000ULL |
	       int64 << 24 & 0x0000FF0000000000ULL |
	       int64 << 40 & 0x00FF000000000000ULL |
	       int64 << 56;
}
#endif


// Int types

#if defined(WDL_LITTLE_ENDIAN)

	#define WDL_bswap16_if_le(x) WDL_bswap16(x)
	#define WDL_bswap32_if_le(x) WDL_bswap32(x)
	#define WDL_bswap64_if_le(x) WDL_bswap64(x)
	#define WDL_bswap16_if_be(x) ((unsigned short)(x))
	#define WDL_bswap32_if_be(x) ((unsigned int)(x))
	#define WDL_bswap64_if_be(x) ((WDL_UINT64)(x))

	// Wrappers that convert a variable in-place, or generate no code if
	// conversion is not necessary. Beware to only feed variables to these
	// macros, so no fancy things like WDL_BSWAP32_IF_LE(x + y) or
	// WDL_BSWAP32_IF_LE(x++). Note that these macros only change the type
	// to unsigned if conversion is actually necessary.
	#define WDL_BSWAP16_IF_LE(x) (x = WDL_bswap16(x))
	#define WDL_BSWAP32_IF_LE(x) (x = WDL_bswap32(x))
	#define WDL_BSWAP64_IF_LE(x) (x = WDL_bswap64(x))
	#define WDL_BSWAP16_IF_BE(x) ((void)0)
	#define WDL_BSWAP32_IF_BE(x) ((void)0)
	#define WDL_BSWAP64_IF_BE(x) ((void)0)

#else // #elif defined(WDL_BIG_ENDIAN)

	#define WDL_bswap16_if_be(x) WDL_bswap16(x)
	#define WDL_bswap32_if_be(x) WDL_bswap32(x)
	#define WDL_bswap64_if_be(x) WDL_bswap64(x)
	#define WDL_bswap16_if_le(x) ((unsigned short)(x))
	#define WDL_bswap32_if_le(x) ((unsigned int)(x))
	#define WDL_bswap64_if_le(x) ((WDL_UINT64)(x))

	// In-place wrappers (see notes above)
	#define WDL_BSWAP16_IF_BE(x) (x = WDL_bswap16(x))
	#define WDL_BSWAP32_IF_BE(x) (x = WDL_bswap32(x))
	#define WDL_BSWAP64_IF_BE(x) (x = WDL_bswap64(x))
	#define WDL_BSWAP16_IF_LE(x) ((void)0)
	#define WDL_BSWAP32_IF_LE(x) ((void)0)
	#define WDL_BSWAP64_IF_LE(x) ((void)0)

#endif // WDL_bswapXX_if_YY


// C++ auto-typed wrappers

#ifdef __cplusplus

static WDL_ENDIAN_INLINE unsigned short WDL_bswap_if_le(unsigned short int16) { return WDL_bswap16_if_le(int16); }
static WDL_ENDIAN_INLINE signed   short WDL_bswap_if_le(signed   short int16) { return WDL_bswap16_if_le(int16); }
static WDL_ENDIAN_INLINE unsigned int   WDL_bswap_if_le(unsigned int   int32) { return WDL_bswap32_if_le(int32); }
static WDL_ENDIAN_INLINE signed   int   WDL_bswap_if_le(signed   int   int32) { return WDL_bswap32_if_le(int32); }
static WDL_ENDIAN_INLINE WDL_UINT64     WDL_bswap_if_le(WDL_UINT64     int64) { return WDL_bswap64_if_le(int64); }
static WDL_ENDIAN_INLINE WDL_INT64      WDL_bswap_if_le(WDL_INT64      int64) { return WDL_bswap64_if_le(int64); }

static WDL_ENDIAN_INLINE unsigned short WDL_bswap_if_be(unsigned short int16) { return WDL_bswap16_if_be(int16); }
static WDL_ENDIAN_INLINE signed   short WDL_bswap_if_be(signed   short int16) { return WDL_bswap16_if_be(int16); }
static WDL_ENDIAN_INLINE unsigned int   WDL_bswap_if_be(unsigned int   int32) { return WDL_bswap32_if_be(int32); }
static WDL_ENDIAN_INLINE signed   int   WDL_bswap_if_be(signed   int   int32) { return WDL_bswap32_if_be(int32); }
static WDL_ENDIAN_INLINE WDL_UINT64     WDL_bswap_if_be(WDL_UINT64     int64) { return WDL_bswap64_if_be(int64); }
static WDL_ENDIAN_INLINE WDL_INT64      WDL_bswap_if_be(WDL_INT64      int64) { return WDL_bswap64_if_be(int64); }

// Auto-typed in-place wrappers (see notes above)
#ifdef WDL_LITTLE_ENDIAN
	#define WDL_BSWAP_IF_LE(x) (x = WDL_bswap_if_le(x))
	#define WDL_BSWAP_IF_BE(x) ((void)0)
#else // #elif defined(WDL_BIG_ENDIAN)
	#define WDL_BSWAP_IF_BE(x) (x = WDL_bswap_if_be(x))
	#define WDL_BSWAP_IF_LE(x) ((void)0)
#endif


// Map floating point types to int types.

#if defined(WDL_LITTLE_ENDIAN)
	#define __WDL_bswapf_if_a WDL_bswapf_if_le
	#define __WDL_bswapf_if_b WDL_bswapf_if_be
#else // #elif defined(WDL_BIG_ENDIAN)
	#define __WDL_bswapf_if_a WDL_bswapf_if_be
	#define __WDL_bswapf_if_b WDL_bswapf_if_le
#endif

static WDL_ENDIAN_INLINE unsigned int __WDL_bswapf_if_a(const float        f)     { return WDL_bswap32(*(const unsigned int*)&f); }
static WDL_ENDIAN_INLINE WDL_UINT64   __WDL_bswapf_if_a(const double       f)     { return WDL_bswap64(*(const WDL_UINT64  *)&f); }

static WDL_ENDIAN_INLINE float        __WDL_bswapf_if_a(const unsigned int int32)
{
	const unsigned int i = WDL_bswap32(int32);
	return *(const float*)&i;
}

static WDL_ENDIAN_INLINE double       __WDL_bswapf_if_a(const WDL_UINT64   int64)
{
	const WDL_UINT64 i = WDL_bswap64(int64);
	return *(const double*)&i;
}

static WDL_ENDIAN_INLINE unsigned int __WDL_bswapf_if_b(const float        f)     { return *(const unsigned int*)&f; }
static WDL_ENDIAN_INLINE WDL_UINT64   __WDL_bswapf_if_b(const double       f)     { return *(const WDL_UINT64  *)&f; }
static WDL_ENDIAN_INLINE float        __WDL_bswapf_if_b(const unsigned int int32) { return *(const float       *)&int32; }
static WDL_ENDIAN_INLINE double       __WDL_bswapf_if_b(const WDL_UINT64   int64) { return *(const double      *)&int64; }

#undef __WDL_bswapf_if_a
#undef __WDL_bswapf_if_b

#endif // __cplusplus


#endif // _WDL_ENDIAN_H_
