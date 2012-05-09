#ifndef __DIGICODE_H__
#define __DIGICODE_H__

// This header and associated .cpp files are a simple way of including the digidesign plugin framework files
// thanks to Julian Storer who does it like this in juce, hope it's not a violation of GPL

#include "IPlugOSDetect.h"

#if RTAS_API
  #ifdef OS_WIN

    #define kCompileAsCodeResource    0
    #define kBuildStandAlone          0
    #define kNoDSP                    0
    #define kNoDAE                    0
    #define kNoSDS                    0
    #define kNoViews                  0
    #define kUseDSPCodeDecode         0

    #define WINDOWS_VERSION           1
    #define PLUGIN_SDK_BUILD          1
    #define PLUGIN_SDK_DIRECTMIDI     1

    #include "ForcedInclude.h"

  #else

    #define kCompileAsCodeResource    0
    #define kNoDSP                    1
    #define kNoDAE                    0
    #define kNoSDS                    0
    #define kNoViews                  0
    #define kUseDSPCodeDecode         0

    #define MAC_VERSION               1
    #define PLUGIN_SDK_BUILD          1
    #define PLUGIN_SDK_DIRECTMIDI     1
    #define DIGI_PASCAL

    #include "MacAlwaysInclude.h"

  #endif
#endif

#endif   // __DIGICODE_H__
