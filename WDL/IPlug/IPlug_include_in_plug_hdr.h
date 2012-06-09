#ifndef _IPLUG_INCLUDE_HDR_
#define _IPLUG_INCLUDE_HDR_

// Include this file in the main header for your plugin,
// after #defining either VST_API or AU_API.
#include <stdio.h>
#include "IPlugOSDetect.h"
#include "resource.h" // This is your plugin's resource.h

#ifdef VST_API
  #include "IPlugVST.h"
  typedef IPlugVST IPlug;
  #define API_EXT "vst"
#elif VST3_API
  #include "IPlugVST3.h"
  typedef IPlugVST3 IPlug;
  #define API_EXT "vst3"
#elif defined AU_API
  #include "IPlugAU.h"
  typedef IPlugAU IPlug;
  #define API_EXT "audiounit"
#elif RTAS_API
  #include "IPlugRTAS.h"
  typedef IPlugRTAS IPlug;
  #define API_EXT "rtas"
  #define PROTOOLS
#elif AAX_API
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
  #include "IGraphicsWin.h"
  #ifdef __MINGW32__
    #define EXPORT __attribute__ ((visibility("default")))
  #else
    #define EXPORT __declspec(dllexport)
  #endif
#elif defined OS_OSX
  #include "IGraphicsMac.h"
  #define EXPORT __attribute__ ((visibility("default")))
  #define BUNDLE_ID "com." BUNDLE_MFR "." API_EXT "." BUNDLE_NAME
#elif defined OS_IOS
  #define EXPORT __attribute__ ((visibility("default")))
  #define BUNDLE_ID "com." BUNDLE_MFR "." API_EXT "." BUNDLE_NAME
#elif defined OS_LINUX
  //TODO
#endif

#endif // _IPLUG_INCLUDE_HDR_
