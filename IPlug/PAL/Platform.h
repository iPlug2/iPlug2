/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for more info.
 
 ==============================================================================
*/

#pragma once

// clang-format off

//---------------------------------------------------------
// 
#ifndef PLATFORM_NAME
	#error "PLATFORM_NAME must be defined. Make sure cmake declared this when generating project."
#endif

// Set non-active platforms to 0
#ifndef PLATFORM_WINDOWS
	#define PLATFORM_WINDOWS 0
#endif

#ifndef PLATFORM_IOS
	#define PLATFORM_IOS     0
#endif

#ifndef PLATFORM_MAC
	#define PLATFORM_MAC     0
#endif

#ifndef PLATFORM_LINUX
	#define PLATFORM_LINUX   0
#endif

#ifndef PLATFORM_WEB
	#define PLATFORM_WEB     0
#endif

#if PLATFORM_WINDOWS + PLATFORM_IOS + PLATFORM_MAC + PLATFORM_LINUX + PLATFORM_WEB != 1
	#error "One and only one platform should be active. Check cmake settings."
#endif

//---------------------------------------------------------
// Compiler settings

#include "PlatformCompiler.h"


//---------------------------------------------------------
// Global preprocessor definitions

#define PREPROCESSOR_TOKEN_STRING(expr)         #expr
#define PREPROCESSOR_TOKEN_CONCAT(A, B)         A##B
#define PREPROCESSOR_TOKEN_VARIADIC(...)        __VA_ARGS__
#define PREPROCESSOR_STRING(expr)               PREPROCESSOR_TOKEN_STRING(expr)
#define PREPROCESSOR_CONCAT(A, B)               PREPROCESSOR_TOKEN_CONCAT(A, B)
#define PREPROCESSOR_UNPARENTHESIZE(...)        PREPROCESSOR_TOKEN_VARIADIC __VA_ARGS__

// No quotes in filename
#define PLATFORM_HEADER(filename)               PREPROCESSOR_STRING(PLATFORM_NAME/filename)
#define PLATFORM_PREFIX_HEADER(filename)        PREPROCESSOR_STRING(PREPROCESSOR_CONCAT(PLATFORM_NAME/PLATFORM_NAME, filename))

#define BEGIN_IPLUG_NAMESPACE                   namespace iplug {
#define BEGIN_IGRAPHICS_NAMESPACE               namespace igraphics {
#define END_IPLUG_NAMESPACE                     }
#define END_IGRAPHICS_NAMESPACE                 }

#define WARNING_MESSAGE(msg)                    PRAGMA(message(__FILE__ "(" PREPROCESSOR_STRING(__LINE__) ") : " "WARNING: " msg))
#define REMINDER_MESSAGE(msg)                   PRAGMA(message(__FILE__ "(" PREPROCESSOR_STRING(__LINE__) "): " msg))

#define DEPRECATED(version, message)            [[deprecated(message)]]
#define NODISCARD                               [[nodiscard]]


//---------------------------------------------------------
// STD headers

#include <algorithm>
#include <array>
#include <atomic>
#include <bitset>
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


// clang-format on

//---------------------------------------------------------
// Set default types

namespace iplug::generic
{
	// std::uint*_t and int*_t are optional implementations
	// and may not be available on some compilers.
	struct Types
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
		using utf16  = char16_t;
		using utf32  = char32_t;
		using size_t = std::size_t;

		enum class utf8 : unsigned char
		{
		};
	};
}  // namespace iplug::types

//---------------------------------------------------------
// Include target platform main header file

#include PLATFORM_PREFIX_HEADER(Platform.h)

// NULL definition conformance and overload type safety
#undef NULL
#define NULL nullptr

//---------------------------------------------------------
// Default preprocessor definitions

#ifndef PLATFORM_64BIT
	#error "PLATFORM_64BIT is undefined. Check PlatformCompiler.h"
#endif

#ifndef BEGIN_INCLUDE_DEPENDENCIES
	#define BEGIN_INCLUDE_DEPENDENCIES
#endif

#ifndef END_INCLUDE_DEPENDENCIES
	#define END_INCLUDE_DEPENDENCIES
#endif

#ifndef NOINLINE
	#define NOINLINE
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
	// Check if we have a valid platform struct
	static_assert(std::is_class_v<types::Platform>, "Platform type definition is wrong type.");
	static_assert(std::is_base_of_v<generic::Types, types::Platform>,
				  "Platform type definition structure inheritance failure.");

	using byte   = types::Platform::byte;    // 8-bit unsigned enum class type
	using uint8  = types::Platform::uint8;   // 8-bit unsigned
	using uint16 = types::Platform::uint16;  // 16-bit unsigned
	using uint32 = types::Platform::uint32;  // 32-bit unsigned
	using uint64 = types::Platform::uint64;  // 64-bit unsigned
	using int8   = types::Platform::int8;    // 8-bit signed
	using int16  = types::Platform::int16;   // 16-bit signed
	using int32  = types::Platform::int32;   // 32-bit signed
	using int64  = types::Platform::int64;   // 64-bit signed
	using utf8   = types::Platform::utf8;    // 8-bit unsigned enum class type
	using utf16  = types::Platform::utf16;   // 16-bit unsigned
	using utf32  = types::Platform::utf32;   // 32-bit unsigned
	using size_t = types::Platform::size_t;  // 32-bit or 64-bit unsigned


	//---------------------------------------------------------
	// Type safety checks. Don't want things to go badonkadonk.

	static_assert(sizeof(void*) == (PLATFORM_64BIT + 1) << 2,
				  "ptr size failed. size does not match target architecture (32bit/64bit).");
	static_assert(sizeof(void*) == sizeof(nullptr),
				  "ptr size failed. void* and nullptr should be equal size. If this fails, the world is doomed.");

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
	static_assert(sizeof(size_t) >= 4, "size_t type size failed.");
	static_assert(sizeof(size_t) == sizeof(nullptr), "size_t type size failed.");
	static_assert(sizeof(size_t) == (PLATFORM_64BIT + 1) << 2, "size_t type size failed.");
	static_assert(sizeof(wchar_t) >= 2, "wchar_t type size failed.");

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
	static_assert(size_t(-1) > size_t(0), "size_t type sign test failed. size is signed.");
	static_assert(wchar_t(-1) > wchar_t(0), "wchar_t type sign test failed. wchar is signed.");
}  // namespace iplug
