/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "IPlugPreprocessor.h"
#include "PlatformCompiler.h"

namespace iplug::generic
{
	enum class utf8 : unsigned char
	{
	};

	// Set default types
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
		using utf8   = ::iplug::generic::utf8;
		using utf16  = char16_t;
		using utf32  = char32_t;
		using size_t = std::size_t;
		using tfloat = IPLUG2_TFLOAT_TYPE;
	};
}  // namespace iplug::generic


// Include target platform main header file
#include PLATFORM_PREFIX_HEADER(Platform.h)

// Must be defined after main platform header
#ifdef _DEBUG
	#define DEBUG_ASSERT(expr) ((expr) ? ((void) 0) : (DEBUGBREAK()))
#else
    #define DEBUG_ASSERT(expr) assert(expr)
#endif

// NULL redefinition for compiler conformance and overload type safety
#undef NULL
#define NULL nullptr

#ifndef PLATFORM_64BIT
	#error "PLATFORM_64BIT is undefined. Check PlatformCompiler.h"
#endif

#ifndef BEGIN_INCLUDE_DEPENDENCIES
	#define BEGIN_INCLUDE_DEPENDENCIES
#endif

#ifndef END_INCLUDE_DEPENDENCIES
	#define END_INCLUDE_DEPENDENCIES
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


//-----------------------------------------------------------------------------
// Link types from target platform to iplug namespace

namespace iplug
{
	// Check if we have a valid platform struct
	static_assert(std::is_class_v<type::Platform>, "Platform type definition is wrong type.");
	static_assert(std::is_base_of_v<generic::Types, type::Platform>,
				  "Platform type definition structure inheritance failure.");

	using byte   = type::Platform::byte;    // 8-bit unsigned enum class type
	using uint8  = type::Platform::uint8;   // 8-bit unsigned
	using uint16 = type::Platform::uint16;  // 16-bit unsigned
	using uint32 = type::Platform::uint32;  // 32-bit unsigned
	using uint64 = type::Platform::uint64;  // 64-bit unsigned
	using int8   = type::Platform::int8;    // 8-bit signed
	using int16  = type::Platform::int16;   // 16-bit signed
	using int32  = type::Platform::int32;   // 32-bit signed
	using int64  = type::Platform::int64;   // 64-bit signed
	using utf8   = type::Platform::utf8;    // 8-bit unsigned enum class type
	using utf16  = type::Platform::utf16;   // 16-bit unsigned
	using utf32  = type::Platform::utf32;   // 32-bit unsigned
	using size_t = type::Platform::size_t;  // 32-bit or 64-bit unsigned
	using tfloat = type::Platform::tfloat;  // defined floating-point type float/double


	//-----------------------------------------------------------------------------
	// Type safety checks. Don't want things to go badonkadonk.

	static_assert(sizeof(void*) == (PLATFORM_64BIT + 1) << 2,
				  "ptr size failed. size does not match target architecture (32bit/64bit).");
	static_assert(sizeof(void*) == sizeof(nullptr),
				  "ptr size failed. void* and nullptr should be equal size. If this fails, the world is doomed.");

	static_assert(std::is_floating_point_v<tfloat> && (std::is_same_v<tfloat, float> || std::is_same_v<tfloat, double>),
				  "tfloat is invalid type. Only float and double are valid");

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


	//-----------------------------------------------------------------------------
	// templates

	// Return number of elements in an array.
	// TODO: temporary, move to template header later
	template <typename T>
	inline constexpr size_t TArrayCount(T&& array)
	{
		return sizeof(uint8(&)[sizeof(array) / sizeof(array[0])]);
	}
}  // namespace iplug


namespace iplug::type
{
	template <class>
	inline constexpr bool Always_false = false;

	template <class T>
	struct InvalidType
	{
		static_assert(Always_false<T>, "Invalid type.");
	};

	// Copy of MSVC internal std::_Is_any_of_v implementation
	template <class T, class... Types>
	inline constexpr bool IsAnyOf = std::disjunction_v<std::is_same<T, Types>...>;

	// No bool or char
	template <class T>
	inline constexpr bool IsMathIntegral = IsAnyOf<std::remove_cv_t<T>,  // remove 'const' and 'volatile' qualifiers
												   short,
												   unsigned short,
												   int,
												   unsigned int,
												   long,
												   unsigned long,
												   long long,
												   unsigned long long>;

	template <class Tx, class Ty>
	inline constexpr bool IsSame = std::is_same_v<std::remove_cv_t<Tx>, Ty>;

	template <class T>
	inline constexpr bool IsIntegral = std::is_integral_v<T>;

	template <class T>
	inline constexpr bool IsFloatingPoint = std::is_floating_point_v<T>;

	template <class T>
	inline constexpr bool IsArithmetic = std::is_arithmetic_v<T>;

	template <class T>
	inline constexpr bool IsSigned = std::is_signed_v<T>;

	template <class T>
	inline constexpr bool IsUnsigned = std::is_unsigned_v<T>;

	template <class T>
	inline constexpr bool IsFloat = IsSame<T, float>;

	template <class T>
	inline constexpr bool IsDouble = IsSame<T, double>;

	// Using modified is_integral_v without bool or char types
	template <class T>
	inline constexpr bool IsMathArithmetic = IsMathIntegral<T> || IsFloatingPoint<T>;

	// clang-format off

	// Returns uint8, uint16, uint32 or uint64 based on the size of T
	template <class T>
	using ConditionalUIntSize = std::conditional_t<sizeof(T) == 1, uint8,
		                        std::conditional_t<sizeof(T) == 2, uint16,
		                        std::conditional_t<sizeof(T) == 4, uint32,
		                        std::conditional_t<sizeof(T) == 8, uint64,
		                        type::InvalidType<T>>>>>;

	// Returns int8, int16, int32 or int64 based on the size of T
	template <class T>
	using ConditionalIntSize =  std::conditional_t<sizeof(T) == 1, int8,
		                        std::conditional_t<sizeof(T) == 2, int16,
		                        std::conditional_t<sizeof(T) == 4, int32,
		                        std::conditional_t<sizeof(T) == 8, int64,
		                        type::InvalidType<T>>>>>;

	// clang-format on
}  // namespace iplug::type


// Temporary
namespace iplug::generic
{
	struct Platform
	{
		inline static constexpr bool IsLittleEndian()
		{
			return (PLATFORM_LITTLE_ENDIAN == 1);
		}
	};
}  // namespace iplug::generic

namespace iplug
{
	struct Platform final : public iplug::generic::Platform
	{
	};
}  // namespace iplug
