/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#pragma once


//-----------------------------------------------------------------------------
// Platform configuration

// currently a copy from WindowsPlatform.h

#error "IOSPlatform.h needs to be configured... Current settings are unconfirmed."

#define PLATFORM_LITTLE_ENDIAN   1
#define PLATFORM_CACHE_LINE_SIZE 64
#define PLATFORM_PTHREADS        0

// https://stackoverflow.com/questions/44140778/resumable-assert-breakpoint-on-ios-like-debugbreak-with-ms-compiler/44142833#44142833
#if defined(__aarch64__)  // ARM64
	#define DEBUGBREAK()                                                         \
		__asm__ __volatile__("   mov    x0, %x0;    \n" /* pid                */ \
							 "   mov    x1, #0x11;  \n" /* SIGSTOP            */ \
							 "   mov    x16, #0x25; \n" /* syscall 37 = kill  */ \
							 "   svc    #0x80       \n" /* software interrupt */ \
							 "   mov    x0, x0      \n" /* nop                */ \
							 ::"r"(getpid())                                     \
							 : "x0", "x1", "x16", "memory")
#else
	#define DEBUGBREAK() __asm__ __volatile__("int $3; mov %eax, %eax")
#endif


//-----------------------------------------------------------------------------
// IOS specific types

namespace iplug::type
{
	struct IOS : iplug::generic::Types
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
	using Platform = IOS;
}  // namespace iplug::type


//-----------------------------------------------------------------------------

#include <CoreFoundation/CoreFoundation.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
