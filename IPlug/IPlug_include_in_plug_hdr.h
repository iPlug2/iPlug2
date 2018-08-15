/*
 ==============================================================================
 
 This file is part of the iPlug 2 library
 
 Oli Larkin et al. 2018 - https://www.olilarkin.co.uk
 
 iPlug 2 is an open source library subject to commercial or open-source
 licensing.
 
 The code included in this file is provided under the terms of the WDL license
 - https://www.cockos.com/wdl/
 
 ==============================================================================
 */

#pragma once

/** \file IPlug_include_in_plug_hdr.h
    \brief IPlug header include

    Include this file in the main header for your plugin
    A preprocessor macro for a particular API such as VST2_API should be defined at project level
*/

#include <cstdio>
#include "IPlugPlatform.h"
#include "config.h"

#define API_EXT2
#ifdef VST2_API
#ifdef REAPER_PLUGIN
  #define LICE_PROVIDED_BY_APP
  #include "IPlugReaperVST2.h"
  typedef IPlugReaperVST2 IPlug;
#else
  #include "IPlugVST2.h"
  typedef IPlugVST2 IPlug;
#endif
  #define API_EXT "vst"
#elif defined VST3_API
  #include "IPlugVST3.h"
  typedef IPlugVST3 IPlug;
  #define API_EXT "vst3"
#elif defined AU_API
  #include "IPlugAU.h"
  typedef IPlugAU IPlug;
  #define API_EXT "audiounit"
#elif defined AUv3_API
  #include "IPlugAUv3.h"
  typedef IPlugAUv3 IPlug;
  #define API_EXT "app"
  #undef API_EXT2
  #define API_EXT2 ".AUv3"
#elif defined AAX_API
  #include "IPlugAAX.h"
  typedef IPlugAAX IPlug;
  #define API_EXT "aax"
  #define PROTOOLS
#elif defined APP_API
  #include "IPlugAPP.h"
  typedef IPlugAPP IPlug;
  #define API_EXT "app"
#elif defined WAM_API
  #include "IPlugWAM.h"
  typedef IPlugWAM IPlug;
#elif defined WEB_API
  #include "IPlugWeb.h"
  typedef IPlugWeb IPlug;
#elif defined VST3_API
  #include "IPlugVST3.h"
  typedef IPlugVST3 IPlug;
  #define API_EXT "vst3"
#elif defined VST3C_API
  #include "IPlugVST3_Controller.h"
  typedef IPlugVST3Controller IPlug;
  #undef PLUG_CLASS_NAME
  #define PLUG_CLASS_NAME VST3Controller
  #define API_EXT "vst3"
#elif defined VST3P_API
  #include "IPlugVST3_Processor.h"
  typedef IPlugVST3Processor IPlug;
  #define API_EXT "vst3"
#else
  #error "No API defined!"
#endif

#ifdef OS_WIN
  #define EXPORT __declspec(dllexport)
  #define BUNDLE_ID ""
#elif defined OS_MAC || defined OS_IOS
  #define BUNDLE_ID BUNDLE_DOMAIN "." BUNDLE_MFR "." API_EXT "." BUNDLE_NAME API_EXT2
  #define EXPORT __attribute__ ((visibility("default")))
#elif defined OS_LINUX
  //TODO:
#elif defined OS_WEB
  #define BUNDLE_ID ""
#else
  #error "No OS defined!"
#endif

#if defined OS_MAC
  #if defined SWELL_NO_POSTMESSAGE && !defined VST3P_API
    #include <sys/time.h>
    #include <unistd.h>
    #include "swell.h"
    void Sleep(int ms);
    DWORD GetTickCount();
  #endif
#endif

#if !defined NO_IGRAPHICS && !defined VST3P_API
#include "IGraphics_include_in_plug_hdr.h"
#endif
