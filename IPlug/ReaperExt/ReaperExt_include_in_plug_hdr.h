/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

/**
 * @file ReaperExt_include_in_plug_hdr.h
 * @brief IPlug ReaperExt header include
 * Include this file in the main header for your reaper extension
*/

#include "config.h"

#include "IPlugPlatform.h"

#if !defined NO_IGRAPHICS
#include "IGraphics_include_in_plug_hdr.h"
#endif

#ifdef OS_WIN
#include <windows.h>
#else
#include "swell.h"
#undef FillRect
#undef DrawText
#endif

#include "ReaperExtBase.h"