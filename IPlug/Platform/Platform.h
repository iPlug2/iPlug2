/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for more info.
 
 ==============================================================================
*/


#pragma once
// clang-format off

//---------------------------------------------------------
// Global compiler specific preprocessor definitions
#if defined(_MSC_VER)
	#define PRAGMA(...)       __pragma(__VA_ARGS__)
	#define IPLUG_EXPORT      __declspec(dllexport)
	#define NOINLINE          __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
	#define PRAGMA(...)       _pragma(__VA_ARGS__)
	#define IPLUG_EXPORT      __attribute__((visibility("default")))
	#define NOINLINE          noinline
#endif


//---------------------------------------------------------
// Preprocessor helpers

#define PREPROCESSOR_TOKEN_STRING(expr)         #expr
#define PREPROCESSOR_TOKEN_CONCAT(A, B)         A##B
#define PREPROCESSOR_TOKEN_VARIADIC(...)        __VA_ARGS__
#define PREPROCESSOR_STRING(expr)               PREPROCESSOR_TOKEN_STRING(expr)
#define PREPROCESSOR_CONCAT(A, B)               PREPROCESSOR_TOKEN_CONCAT(A, B)
#define PREPROCESSOR_UNPARENTHESIZE(...)        PREPROCESSOR_TOKEN_VARIADIC __VA_ARGS__

//---------------------------------------------------------
// Only works if [def] is defined as 0 or 1. If [def] is undefined it goes badonkadonk.

#define __PP_INTERNAL_IF_0(expr0, expr1)        expr0
#define __PP_INTERNAL_IF_1(expr0, expr1)        expr1
#define PREPROCESSOR_IF0ELSE(def, expr0, expr1) PREPROCESSOR_TOKEN_CONCAT(__PP_INTERNAL_IF_, def)(expr0, expr1)
#define PREPROCESSOR_IF1ELSE(def, expr1, expr0) PREPROCESSOR_TOKEN_CONCAT(__PP_INTERNAL_IF_, def)(expr0, expr1)
#define PREPROCESSOR_IF0(def, expr)             PREPROCESSOR_TOKEN_CONCAT(__PP_INTERNAL_IF_, def)(expr, )
#define PREPROCESSOR_IF1(def, expr)             PREPROCESSOR_TOKEN_CONCAT(__PP_INTERNAL_IF_, def)(, expr)

//---------------------------------------------------------
// Global preprocessor definitions

#define BEGIN_IPLUG_NAMESPACE                   namespace iplug {
#define BEGIN_IGRAPHICS_NAMESPACE               namespace igraphics {
#define END_IPLUG_NAMESPACE                     }
#define END_IGRAPHICS_NAMESPACE                 }

#define WARNING_MESSAGE(msg)                    PRAGMA(message(__FILE__ "(" PREPROCESSOR_STRING(__LINE__) ") : " "WARNING: " msg))
#define REMINDER_MESSAGE(msg)                   PRAGMA(message(__FILE__ "(" PREPROCESSOR_STRING(__LINE__) "): " msg))
#define DEPRECATED(version, message)            [[deprecated(message)]]
#define NODISCARD                               [[nodiscard]]


// clang-format on

//---------------------------------------------------------
// Detect platform target

#ifdef _WIN32
	#define PLATFORM_WINDOWS 1
	#define OS_WIN  // TODO: remove later
#elif __APPLE__
	#include <TargetConditionals.h>
	#if TARGET_OS_IPHONE
		#define PLATFORM_IOS 1
		#define OS_IOS  // TODO: remove later
	#elif TARGET_OS_MAC
		#define PLATFORM_MAC 1
		#define OS_MAC  // TODO: remove later
	#endif
#elif __gnu_linux__
	#define PLATFORM_LINUX 1
	#define OS_LINUX  // TODO: remove later
#elif EMSCRIPTEN
	#define PLATFORM_WEB 1
	#define OS_WEB  // TODO: remove later
#else
	#error "No OS defined!"
#endif

// Set any undefined platform to 0.
// This will generate a redefinition warning in case of name collition with third-party includes.
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
// TODO: move to precompiled headers

#ifndef _CRT_SECURE_NO_DEPRECATE
	#define _CRT_SECURE_NO_DEPRECATE
#endif

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cctype>
#include <cmath>
#include <codecvt>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>


//---------------------------------------------------------
// Set default types

namespace iplug::Types
{
	// std::uint*_t and int*_t is optional compiler implementation
	struct Generic
	{
		using byte   = std::byte;
		using uint8  = unsigned char;
		using uint16 = unsigned short;
		using uint32 = unsigned int;
		using uint64 = unsigned long long;
		using int8   = char;
		using int16  = short;
		using int32  = int;
		using int64  = long long;
		using utf8   = unsigned char;
		using utf16  = char16_t;
		using utf32  = char32_t;
		using wchar  = wchar_t;
	};
}  // namespace iplug::Types


//---------------------------------------------------------
// Include target platform header
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
// Default platform/compiler dependent preprocessor definitions

#ifndef BEGIN_INCLUDE_DEPENDENCIES
	#define BEGIN_INCLUDE_DEPENDENCIES
#endif

#ifndef END_INCLUDE_DEPENDENCIES
	#define END_INCLUDE_DEPENDENCIES
#endif

#ifndef NOINLINE
	#define NOINLINE
#endif


//---------------------------------------------------------
// Defaults for undefined platform definitions

#ifndef PLATFORM_64BIT
	#error "PLATFORM_64BIT is undefined. Set value of 0 or 1 in corresponding platform section"
#endif

#ifndef IPLUG_API
	#define IPLUG_API
#endif

#ifndef PLATFORM_LITTLE_ENDIAN
	#define PLATFORM_LITTLE_ENDIAN 0
#endif

#if defined(PLATFORM_CACHE_LINE_SIZE) && !defined(PLATFORM_CACHE_ALIGN)
	#undef PLATFORM_CACHE_LINE_SIZE
#elif !defined(PLATFORM_CACHE_LINE_SIZE) && defined(PLATFORM_CACHE_ALIGN)
	#define PLATFORM_CACHE_LINE_SIZE 64
#endif

#ifndef PLATFORM_PTHREADS
	#define PLATFORM_PTHREADS 0
#endif


//---------------------------------------------------------
// Link types from target platform to iplug namespace and perform basic tests

namespace iplug
{
	// 8-bit unsigned enum class
	using byte = Types::Platform::byte;

	// 8-bit unsigned
	using uint8 = Types::Platform::uint8;

	// 16-bit unsigned
	using uint16 = Types::Platform::uint16;

	// 32-bit unsigned
	using uint32 = Types::Platform::uint32;

	// 64-bit unsigned
	using uint64 = Types::Platform::uint64;

	// 8-bit signed
	using int8 = Types::Platform::int8;

	// 16-bit signed
	using int16 = Types::Platform::int16;

	// 32-bit signed
	using int32 = Types::Platform::int32;

	// 64-bit signed
	using int64 = Types::Platform::int64;

	// 8-bit unsigned
	using utf8 = Types::Platform::utf8;

	// 16-bit unsigned
	using utf16 = Types::Platform::utf16;

	// 32-bit unsigned
	using utf32 = Types::Platform::utf32;

	// 16-bit or 32-bit unsigned (undefined width)
	using wchar = Types::Platform::wchar;


	//---------------------------------------------------------
	// Type safety checks. Don't want things to go badonkadonk.

	static_assert(sizeof(uint8) == 1, "uint8 type size failed.");
	static_assert(sizeof(uint16) == 2, "uint16 type size failed.");
	static_assert(sizeof(uint32) == 4, "uint32 type size failed.");
	static_assert(sizeof(uint64) == 8, "uint64 type size failed.");
	static_assert(sizeof(int8) == 1, "int8 type size failed.");
	static_assert(sizeof(int16) == 2, "int16 type size failed.");
	static_assert(sizeof(int32) == 4, "int32 type size failed.");
	static_assert(sizeof(int64) == 8, "int64 type size failed.");
	static_assert(sizeof(utf8) == 1, "utf8 type size failed.");
	static_assert(sizeof(utf16) == 2, "utf16 type size failed.");
	static_assert(sizeof(utf32) == 4, "utf32 type size failed.");
	static_assert(sizeof(wchar) >= 2, "wchar type size failed.");
	static_assert(sizeof(size_t) >= 4, "size_t type size failed.");
	static_assert(sizeof(size_t) == sizeof(nullptr), "size_t type size failed.");

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
	static_assert(size_t(-1) > size_t(0), "size_t type sign test failed. size is signed.");
}  // namespace iplug
