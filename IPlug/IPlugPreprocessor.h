/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for more info.
 
 ==============================================================================
*/


#pragma once

// clang-format off

//-----------------------------------------------------------------------------
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


#ifdef PARAMS_MUTEX
	#define ENTER_PARAMS_MUTEX                  mParams_mutex.Enter();        Trace(TRACELOC, "%s", "ENTER_PARAMS_MUTEX");
	#define LEAVE_PARAMS_MUTEX                  mParams_mutex.Leave();        Trace(TRACELOC, "%s", "LEAVE_PARAMS_MUTEX");
	#define ENTER_PARAMS_MUTEX_STATIC           _this->mParams_mutex.Enter(); Trace(TRACELOC, "%s", "ENTER_PARAMS_MUTEX");
	#define LEAVE_PARAMS_MUTEX_STATIC           _this->mParams_mutex.Leave(); Trace(TRACELOC, "%s", "LEAVE_PARAMS_MUTEX");
#else
	#define ENTER_PARAMS_MUTEX
	#define LEAVE_PARAMS_MUTEX
	#define ENTER_PARAMS_MUTEX_STATIC
	#define LEAVE_PARAMS_MUTEX_STATIC
#endif

#ifndef IGRAPHICS_GL
	#if defined IGRAPHICS_GLES2 || IGRAPHICS_GLES3 || IGRAPHICS_GL2 || IGRAPHICS_GL3
		#define IGRAPHICS_GL
	#endif
#endif

// clang-format on
