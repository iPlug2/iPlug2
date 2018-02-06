#pragma once

/**
 * @file
 * @brief Include to get consistently named preprocessor macros for different platforms and logging functionality
 */

#ifdef _WIN32
  #define OS_WIN
#elif defined __APPLE__
  #include <TargetConditionals.h>
  #if TARGET_OS_MAC
    #define OS_MAC
  #endif
#elif defined __linux || defined __linux__ || defined linux
  #define OS_LINUX
#elif defined EMSCRIPTEN
  #define OS_WEB
#else
  #error "No OS defined!"
#endif

#if defined(_WIN64) || defined(__LP64__)
  #define ARCH_64BIT 
#endif

#include "IPlugLogger.h"

#ifdef NO_PARAMS_MUTEX
#define LOCK_PARAMS_MUTEX
#define LOCK_PARAMS_MUTEX_STATIC
#define ENTER_PARAMS_MUTEX
#define LEAVE_PARAMS_MUTEX
#else
#define LOCK_PARAMS_MUTEX WDL_MutexLock lock(&mParams_mutex)
#define LOCK_PARAMS_MUTEX_STATIC WDL_MutexLock lock(&_this->mParams_mutex)
#define ENTER_PARAMS_MUTEX mParams_mutex.Enter()
#define LEAVE_PARAMS_MUTEX mParams_mutex.Leave()
#endif
