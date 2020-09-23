/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/


#pragma once

namespace iplug
{
	enum class EIntrinsic : uint32
	{
		MMX    = 1 << 0,
		SSE    = 1 << 1,
		SSE2   = 1 << 2,
		SSE3   = 1 << 3,
		SSSE3  = 1 << 4,
		SSE41  = 1 << 5,
		SSE42  = 1 << 6,
		AVX    = 1 << 7,
		AVX2   = 1 << 8,
		FMA3   = 1 << 9,
		FMA4   = 1 << 10,
		AVX512 = 1 << 11,
		KNC    = 1 << 12,
		AMX    = 1 << 13,
		SVML   = 1 << 14,
	};
}  // namespace iplug

namespace iplug::generic
{
	struct GenericSystem
	{
		// Returns cache line size used when compiling as an integer
		static inline constexpr int CacheLineSize()
		{
			return static_cast<int>(ECacheLineSize::Native);
		}

		//! @retval #EEndian Current runtime endianness
		static inline const EEndian GetEndianness()
		{
			static volatile const union
			{
				uint16 i;
				uint8 c[2];
			} u = {0x0001};
			return u.c[0] == 0x01 ? EEndian::Little : EEndian::Big;
		}
	};
}  // namespace iplug::generic


#if __has_include(PLATFORM_HEADER(System.h))
	#include PLATFORM_HEADER(System.h)
#else
namespace iplug
{
	using System = iplug::generic::GenericSystem;
}
#endif
