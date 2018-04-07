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
  #include "IPlugVST2.h"
  typedef IPlugVST2 IPlug;
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
#else
  #error "No API defined!"
#endif

#ifdef OS_WIN
  #define EXPORT __declspec(dllexport)
#elif defined OS_MAC
  #define BUNDLE_ID BUNDLE_DOMAIN "." BUNDLE_MFR "." API_EXT "." BUNDLE_NAME API_EXT2
  #define EXPORT __attribute__ ((visibility("default")))
#elif defined OS_LINUX
  //TODO:
#elif defined OS_WEB
  //TODO:
#else
  #error "No OS defined!"
#endif


