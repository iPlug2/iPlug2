//
//    ██████ ██   ██  ██████   ██████
//   ██      ██   ██ ██    ██ ██            ** Classy Header-Only Classes **
//   ██      ███████ ██    ██ ██
//   ██      ██   ██ ██    ██ ██           https://github.com/Tracktion/choc
//    ██████ ██   ██  ██████   ██████
//
//   CHOC is (C)2022 Tracktion Corporation, and is offered under the terms of the ISC license:
//
//   Permission to use, copy, modify, and/or distribute this software for any purpose with or
//   without fee is hereby granted, provided that the above copyright notice and this permission
//   notice appear in all copies. THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
//   AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
//   CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
//   WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
//   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#ifndef CHOC_PLATFORM_DETECT_HEADER_INCLUDED
#define CHOC_PLATFORM_DETECT_HEADER_INCLUDED

/*
    These conditionals declare the macros
      - CHOC_WINDOWS
      - CHOC_ANDROID
      - CHOC_LINUX
      - CHOC_OSX
      - CHOC_IOS
    ...based on the current operating system.

    It also declares a string literal macro CHOC_OPERATING_SYSTEM_NAME
    which can be used if you need a text description of the OS.
*/
#if defined (_WIN32) || defined (_WIN64)
 #define  CHOC_WINDOWS 1
 #define  CHOC_OPERATING_SYSTEM_NAME   "Windows"
#elif __ANDROID__
 #define  CHOC_ANDROID 1
 #define  CHOC_OPERATING_SYSTEM_NAME   "Android"
#elif defined (LINUX) || defined (__linux__)
 #define  CHOC_LINUX 1
 #define  CHOC_OPERATING_SYSTEM_NAME   "Linux"
#elif __APPLE__
 #define CHOC_APPLE 1
 #include <TargetConditionals.h>
 #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
  #define  CHOC_IOS 1
  #define  CHOC_OPERATING_SYSTEM_NAME   "iOS"
 #else
  #define  CHOC_OSX 1
  #define  CHOC_OPERATING_SYSTEM_NAME   "OSX"
 #endif
#elif defined (__FreeBSD__) || (__OpenBSD__)
 #define  CHOC_BSD 1
 #define  CHOC_OPERATING_SYSTEM_NAME   "BSD"
#elif defined (_POSIX_VERSION)
 #define  CHOC_POSIX 1
 #define  CHOC_OPERATING_SYSTEM_NAME   "Posix"
#elif defined (__EMSCRIPTEN__)
 #define  CHOC_EMSCRIPTEN 1
 #define  CHOC_OPERATING_SYSTEM_NAME   "Emscripten"
#else
 #error "Unknown platform!"
#endif


#endif  // CHOC_PLATFORM_DETECT_HEADER_INCLUDED
