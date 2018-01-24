#pragma once

/** \file IPlug_include_in_plug_hdr.h
    \brief IPlug header include

    Include this file in the main header for your plugin
    A preprocessor macro for a particular API such as VST_API should be defined at project level
*/

#include <cstdio>
#include "IPlugPlatform.h"
#include "config.h" // This is your plugin's config.h

#ifdef VST_API
  #include "IPlugVST.h"
  typedef IPlugVST IPlug;
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
  #define API_EXT "appex"
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
  typedef IPlugWAM IPlug;
#else
  #error "No API defined!"
#endif

#ifdef OS_WIN
#ifndef NO_IGRAPHICS
  #include "IGraphicsWin.h"
#endif
  #ifdef __MINGW32__
    #define EXPORT __attribute__ ((visibility("default")))
  #else
    #define EXPORT __declspec(dllexport)
  #endif
#elif defined OS_OSX
#ifndef NO_IGRAPHICS
  #include "IGraphicsMac.h"
#endif
  #define EXPORT __attribute__ ((visibility("default")))
  #define BUNDLE_ID BUNDLE_DOMAIN "." BUNDLE_MFR "." API_EXT "." BUNDLE_NAME
#elif defined OS_WEB
#ifndef NO_IGRAPHICS
  #include "IGraphicsWeb.h"
#endif
#elif defined OS_LINUX
  //TODO
#endif
