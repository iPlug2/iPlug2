/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/
 
#include "config.h"   // This is your plugin's config.h.
#include <AudioUnit.r>

#define UseExtendedThingResource 1

#include <CoreServices/CoreServices.r>

// this is a define used to indicate that a component has no static data that would mean 
// that no more than one instance could be open at a time - never been true for AUs
#ifndef cmpThreadSafeOnMac
#define cmpThreadSafeOnMac  0x10000000
#endif

#undef  TARGET_REZ_MAC_X86
#if defined(__i386__) || defined(i386_YES)
  #define TARGET_REZ_MAC_X86        1
#else
  #define TARGET_REZ_MAC_X86        0
#endif

#undef  TARGET_REZ_MAC_X86_64
#if defined(__x86_64__) || defined(x86_64_YES)
  #define TARGET_REZ_MAC_X86_64     1
#else
  #define TARGET_REZ_MAC_X86_64     0
#endif

#undef  TARGET_REZ_MAC_ARM_64
#if defined(__arm64__) || defined(ARM64_YES)
  #define TARGET_REZ_MAC_ARM_64     1
#else
  #define TARGET_REZ_MAC_ARM_64     0
#endif

#if TARGET_OS_MAC
   #if TARGET_REZ_MAC_X86 && TARGET_REZ_MAC_X86_64
    #define TARGET_REZ_FAT_COMPONENTS_2 1
    #define Target_PlatformType     platformIA32NativeEntryPoint
    #define Target_SecondPlatformType platformX86_64NativeEntryPoint
  #elif TARGET_REZ_MAC_X86
    #define Target_PlatformType     platformIA32NativeEntryPoint
  #elif TARGET_REZ_MAC_X86_64
    #define Target_PlatformType     platformX86_64NativeEntryPoint
  #elif TARGET_REZ_MAC_ARM_64
    #define Target_PlatformType     platformArm64NativeEntryPoint
  #else
    #error you gotta target something
  #endif
  #define Target_CodeResType    'dlle'
  #define TARGET_REZ_USE_DLLE   1
#else
  #error get a real platform type
#endif // not TARGET_OS_MAC

#ifndef TARGET_REZ_FAT_COMPONENTS_2
  #define TARGET_REZ_FAT_COMPONENTS_2   0
#endif

#ifndef TARGET_REZ_FAT_COMPONENTS_4
  #define TARGET_REZ_FAT_COMPONENTS_4   0
#endif

// ----------------

//#ifdef _DEBUG
//  #define PLUG_PUBLIC_NAME PLUG_NAME "_DEBUG"
//#else
#define PLUG_PUBLIC_NAME PLUG_NAME
//#endif

#define RES_ID 1000
#define RES_NAME PLUG_MFR ": " PLUG_PUBLIC_NAME

#if PLUG_TYPE==0
#if PLUG_DOES_MIDI_IN
#define COMP_TYPE kAudioUnitType_MusicEffect
#else
#define COMP_TYPE kAudioUnitType_Effect
#endif
#elif PLUG_TYPE==1
#define COMP_TYPE kAudioUnitType_MusicDevice
#elif PLUG_TYPE==2
#define COMP_TYPE 'aumi'
#endif

resource 'STR ' (RES_ID, purgeable) {
  RES_NAME
};

resource 'STR ' (RES_ID + 1, purgeable) {
  PLUG_PUBLIC_NAME " AU"
};

resource 'dlle' (RES_ID) {
  AUV2_ENTRY_STR
};

resource 'thng' (RES_ID, RES_NAME) {
  COMP_TYPE,
  PLUG_UNIQUE_ID,
  PLUG_MFR_ID,
  0, 0, 0, 0,               //  no 68K
  'STR ', RES_ID,
  'STR ', RES_ID + 1,
  0,  0,      // icon 
  PLUG_VERSION_HEX,
  componentHasMultiplePlatforms | componentDoAutoVersion,
  0,
  {
    cmpThreadSafeOnMac, 
    Target_CodeResType, RES_ID,
    Target_PlatformType,
#if TARGET_REZ_FAT_COMPONENTS_2 || TARGET_REZ_FAT_COMPONENTS_4
    cmpThreadSafeOnMac, 
    Target_CodeResType, RES_ID,
    Target_SecondPlatformType,
#endif
  }
};

#undef RES_ID
