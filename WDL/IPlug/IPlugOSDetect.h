#ifndef _IPLUG_OSDETECT_H_
#define _IPLUG_OSDETECT_H_

#ifdef _WIN32
  #define OS_WIN
#elif defined __APPLE__
  #include "TargetConditionals.h"
  #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
    #define OS_IOS
  #elif TARGET_OS_MAC
    #define OS_OSX
  #endif
#elif defined __linux || defined __linux__ || defined linux
  #define OS_LINUX
#else
  #error "No OS defined!"
#endif

#if defined(_WIN64) || defined(__LP64__)
  #define ARCH_64BIT 
#endif

#endif // _IPLUG_OSDETECT_H_