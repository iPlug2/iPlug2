/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for more info.
 
 ==============================================================================
*/


#pragma once
// clang-format off

// Preprocessor helpers
#define PREPROCESSOR_TOKEN_STRING(String)       #String
#define PREPROCESSOR_TOKEN_CONCAT(A, B)         A##B
#define PREPROCESSOR_TOKEN_VARIADIC(...)        __VA_ARGS__
#define PREPROCESSOR_STRING(String)             PREPROCESSOR_TOKEN_STRING(String)
#define PREPROCESSOR_CONCAT(A, B)               PREPROCESSOR_TOKEN_CONCAT(A, B)
#define PREPROCESSOR_UNPARENTHESIZE(...)        PREPROCESSOR_TOKEN_VARIADIC __VA_ARGS__

// Only works if [Def] is defined as 0 or 1. If [Def] is undefined it goes badonkadonk.
#define __PP_INTERNAL_IF_0(Expr0, Expr1)        Expr0
#define __PP_INTERNAL_IF_1(Expr0, Expr1)        Expr1
#define PREPROCESSOR_IF0ELSE(Def, Expr0, Expr1) PREPROCESSOR_TOKEN_CONCAT(__PP_INTERNAL_IF_, Def)(Expr0, Expr1)
#define PREPROCESSOR_IF1ELSE(Def, Expr1, Expr0) PREPROCESSOR_TOKEN_CONCAT(__PP_INTERNAL_IF_, Def)(Expr0, Expr1)
#define PREPROCESSOR_IF0(Def, Expr)             PREPROCESSOR_TOKEN_CONCAT(__PP_INTERNAL_IF_, Def)(Expr, )
#define PREPROCESSOR_IF1(Def, Expr)             PREPROCESSOR_TOKEN_CONCAT(__PP_INTERNAL_IF_, Def)(, Expr)

#define DEPRECATED(Version, Message)            [[deprecated(Message)]]

/*
	#define UE_CHECK_IMPL(expr) \
		{ \
			if(!(expr)) \
			{ \
				struct Impl \
				{ \
					static void FORCENOINLINE UE_DEBUG_SECTION ExecCheckImplInternal() \
					{ \
						FDebug::CheckVerifyFailed(#expr, __FILE__, __LINE__, TEXT("")); \
					} \
				}; \
				Impl::ExecCheckImplInternal(); \
				PLATFORM_BREAK(); \
				CA_ASSUME(false); \
			} \
		}
*/


// Preprocessor definitions
#define BEGIN_IPLUG_NAMESPACE                   namespace iplug {
#define BEGIN_IGRAPHICS_NAMESPACE               namespace igraphics {
#define END_IPLUG_NAMESPACE                     }
#define END_IGRAPHICS_NAMESPACE                 }

// clang-format on


//---------------------------------------------------------

// Silence C++17 codecvt deprecation messages
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING


//---------------------------------------------------------

#ifdef _WIN32
	#define PLATFORM_WINDOWS 1
	#define OS_WIN  // TODO: remove later
#elif defined __APPLE__
	#include <TargetConditionals.h>
	#if TARGET_OS_IPHONE
		#define PLATFORM_IOS 1
		#define OS_IOS  // TODO: remove later
	#elif TARGET_OS_MAC
		#define PLATFORM_MAC 1
		#define OS_MAC  // TODO: remove later
	#endif
#elif defined __linux || defined __linux__ || defined linux
	#define PLATFORM_LINUX 1
	#define OS_LINUX  // TODO: remove later
#elif defined EMSCRIPTEN
	#define PLATFORM_WEB 1
	#define OS_WEB  // TODO: remove later
#else
	#error "No OS defined!"
#endif

#if !PLATFORM_WINDOWS
	#define PLATFORM_WINDOWS 0
#endif
#if !PLATFORM_IOS
	#define PLATFORM_IOS 0
#endif
#if !PLATFORM_MAC
	#define PLATFORM_MAC 0
#endif
#if !PLATFORM_LINUX
	#define PLATFORM_LINUX 0
#endif
#if !PLATFORM_WEB
	#define PLATFORM_WEB 0
#endif


//---------------------------------------------------------
// STL headers
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdlib>

#include <cstdlib>
#include <string>
#include <vector>
#include <limits>
#include <memory>


namespace iplug::Types
{
	struct Generic
	{
		typedef unsigned char      uint8;
		typedef unsigned short     uint16;
		typedef unsigned int       uint32;
		typedef unsigned long long uint64;
		typedef char               int8;
		typedef short              int16;
		typedef int                int32;
		typedef long long          int64;
		typedef unsigned char      utf8;
		typedef char16_t           utf16;
		typedef char32_t           utf32;
		typedef wchar_t            wchar;
		typedef unsigned __int64   size;
	};
}  // namespace iplug::Types

//---------------------------------------------------------

// Include correct platform header file
#if PLATFORM_WINDOWS
	#include "Windows/WindowsPlatform.h"
#elif PLATFORM_IOS
	#include "IOS/IOSPlatform.h"
#elif PLATFORM_MAC
	#include "Mac/MacPlatform.h"
#elif PLATFORM_LINUX
	#include "Linux/LinuxPlatform.h"
#elif PLATFORM_WEB
	#include "WEB/WEBPlatform.h"
#else
	#error "No platform target defined"
#endif


//---------------------------------------------------------
// Default values

#ifndef PLATFORM_64BIT
	#error "PLATFORM_64BIT is undefined. Set value of 0 or 1 in corresponding platform section"
#endif

#ifndef PLATFORM_LITTLE_ENDIAN
	#define PLATFORM_LITTLE_ENDIAN 0
#endif

#ifndef PLATFORM_CACHE_LINE_SIZE
	#define PLATFORM_CACHE_LINE_SIZE 64
#endif

#ifndef noinline
	#define noinline
#endif


namespace iplug
{
	const struct aaffq
	{
		const bool b64Bit        = (PLATFORM_64BIT);
		const bool bLittleEndian = (PLATFORM_LITTLE_ENDIAN);
	};
}  // namespace iplug


//---------------------------------------------------------

typedef iplug::Types::PlatformTypes::uint8  uint8;
typedef iplug::Types::PlatformTypes::uint16 uint16;
typedef iplug::Types::PlatformTypes::uint32 uint32;
typedef iplug::Types::PlatformTypes::uint64 uint64;
typedef iplug::Types::PlatformTypes::int8   int8;
typedef iplug::Types::PlatformTypes::int16  int16;
typedef iplug::Types::PlatformTypes::int32  int32;
typedef iplug::Types::PlatformTypes::int64  int64;
typedef iplug::Types::PlatformTypes::utf8   utf8;
typedef iplug::Types::PlatformTypes::utf16  utf16;
typedef iplug::Types::PlatformTypes::utf32  utf32;
typedef iplug::Types::PlatformTypes::wchar  wchar;
typedef iplug::Types::PlatformTypes::size   size;

//---------------------------------------------------------
// Type safety checks

namespace iplug::Types::Tests
{
	static_assert(1 == sizeof(char) && sizeof(char) <= sizeof(short) && sizeof(short) <= sizeof(int) &&
					  sizeof(int) <= sizeof(long) && sizeof(long) <= sizeof(long long),
				  "Type sanity check failed. This should never happen.");

	static_assert(sizeof(uint8) == 1, "uint8 type size failed.");
	static_assert(sizeof(uint16) == 2, "uint16 type size failed.");
	static_assert(sizeof(uint32) == 4, "uint32 type size failed.");
	static_assert(sizeof(uint64) == 8, "uint64 type size failed.");
	static_assert(sizeof(int8) == 1, "int8 type size failed.");
	static_assert(sizeof(int16) == 2, "int16 type size failed.");
	static_assert(sizeof(int32) == 4, "int32 type size failed.");
	static_assert(sizeof(int64) == 8, "int64 type size failed.");
	static_assert(sizeof(utf8) == 1, "utf8 type size failed.");
	static_assert(sizeof(utf16) == 2, "char16 type size failed.");
	static_assert(sizeof(utf32) == 4, "char32 type size failed.");
	static_assert(sizeof(wchar) == sizeof(utf16) || sizeof(wchar) == sizeof(utf32), "wchar type size failed.");
	static_assert(sizeof(size) == sizeof(nullptr), "size type size failed.");

	static_assert(uint8(-1) > uint8(0), "uint8 type sign test failed. uint8 is signed.");
	static_assert(uint16(-1) > uint16(0), "uint16 type sign test failed. uint16 is signed.");
	static_assert(uint32(-1) > uint32(0), "uint32 type sign test failed. uint32 is signed.");
	static_assert(uint64(-1) > uint64(0), "uint64 type sign test failed. uint64 is signed.");
	static_assert(int8(-1) < int8(0), "int8 type sign test failed. int8 is unsigned.");
	static_assert(int16(-1) < int16(0), "int16 type sign test failed. int16 is unsigned.");
	static_assert(int32(-1) < int32(0), "int32 type sign test failed. int32 is unsigned.");
	static_assert(int64(-1) < int64(0), "int64 type sign test failed. int64 is unsigned.");
	static_assert(utf8(-1) > utf8(0), "utf8 type sign test failed. utf8 is signed.");
	static_assert(utf16(-1) > utf16(0), "utf16 type sign test failed. utf16 is signed.");
	static_assert(utf32(-1) > utf32(0), "utf32 type sign test failed. utf32 is signed.");
	static_assert(wchar(-1) > wchar(0), "wchar type sign test failed. wchar is signed.");
	static_assert(size(-1) > size(0), "size type sign test failed. size is signed.");

}  // namespace iplug::Types::Tests

