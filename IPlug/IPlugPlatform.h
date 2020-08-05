/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for more info.
 
 ==============================================================================
*/

/**
 * @file
 * @brief Core include file. Should be included first
 */

#pragma once


//---------------------------------------------------------
// Configuration flags


// Disable version checking when compiling
#ifndef IPLUG_SKIP_CPP_VERSION_CHECK
	#define IPLUG_SKIP_CPP_VERSION_CHECK 0
#endif

// Bypass inclusion of system headers like windows.h
#ifndef IPLUG_DONT_INCLUDE_PLATFORM_HEADER
	#define IPLUG_DONT_INCLUDE_PLATFORM_HEADER 0
#endif

// Enables extra compiler warnings
#ifndef IPLUG_EXTENDED_COMPILER_WARNINGS
	#define IPLUG_EXTENDED_COMPILER_WARNINGS 1
#endif

#include "Platform/Platform.h"


#ifdef PARAMS_MUTEX
	#define ENTER_PARAMS_MUTEX \
		mParams_mutex.Enter(); \
		Trace(TRACELOC, "%s", "ENTER_PARAMS_MUTEX");
	#define LEAVE_PARAMS_MUTEX \
		mParams_mutex.Leave(); \
		Trace(TRACELOC, "%s", "LEAVE_PARAMS_MUTEX");
	#define ENTER_PARAMS_MUTEX_STATIC \
		_this->mParams_mutex.Enter(); \
		Trace(TRACELOC, "%s", "ENTER_PARAMS_MUTEX");
	#define LEAVE_PARAMS_MUTEX_STATIC \
		_this->mParams_mutex.Leave(); \
		Trace(TRACELOC, "%s", "LEAVE_PARAMS_MUTEX");
#else
	#define ENTER_PARAMS_MUTEX
	#define LEAVE_PARAMS_MUTEX
	#define ENTER_PARAMS_MUTEX_STATIC
	#define LEAVE_PARAMS_MUTEX_STATIC
#endif


#if defined IGRAPHICS_GLES2 || IGRAPHICS_GLES3 || IGRAPHICS_GL2 || IGRAPHICS_GL3
	#define IGRAPHICS_GL
#endif
