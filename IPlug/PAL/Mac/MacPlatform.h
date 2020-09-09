/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#pragma once


//---------------------------------------------------------
// Platform configuration

// currently a copy from WindowsPlatform.h

#error "MacPlatform.h needs to be configured... Current settings are unconfirmed."

#define PLATFORM_LITTLE_ENDIAN   1
#define PLATFORM_CACHE_LINE_SIZE 64
#define PLATFORM_CACHE_ALIGN     __attribute__((aligned(PLATFORM_CACHE_LINE_SIZE)))
#define DEBUGBREAK()             __debugbreak()
#define PLATFORM_PTHREADS        0


//---------------------------------------------------------
// Mac specific types

namespace iplug::types
{
	struct MacOS : iplug::generic::Types
	{
		using uint8  = std::uint8_t;
		using uint16 = std::uint16_t;
		using uint32 = std::uint32_t;
		using uint64 = std::uint64_t;
		using int8   = std::int8_t;
		using int16  = std::int16_t;
		using int32  = std::int32_t;
		using int64  = std::int64_t;
	};
	using Platform = MacOS;
}  // namespace iplug::types

