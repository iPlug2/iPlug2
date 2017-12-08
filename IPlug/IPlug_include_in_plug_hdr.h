#pragma once

// Include this file in the main header for your plugin,
// after #defining XXX_API.
#include <cstdio>
#include "IPlugOSDetect.h"
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
#elif defined AAX_API
  #include "IPlugAAX.h"
  typedef IPlugAAX IPlug;
  #define API_EXT "aax"
  #define PROTOOLS
#elif defined SA_API
  #include "IPlugStandalone.h"
  typedef IPlugStandalone IPlug;
  #define API_EXT "standalone"
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
  #define BUNDLE_ID "com." BUNDLE_MFR "." API_EXT "." BUNDLE_NAME
#elif defined OS_LINUX
  //TODO
#endif
