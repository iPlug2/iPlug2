#include "resource.h"   // This is your plugin's resource.h.
#include <AudioUnit/AudioUnit.r>

#define UseExtendedThingResource 1

#include <CoreServices/CoreServices.r>

// this is a define used to indicate that a component has no static data that would mean 
// that no more than one instance could be open at a time - never been true for AUs
#ifndef cmpThreadSafeOnMac
#define cmpThreadSafeOnMac  0x10000000
#endif

#undef  TARGET_REZ_MAC_PPC
#if defined(__ppc__) || defined(ppc_YES)
  #define TARGET_REZ_MAC_PPC        1
#else
  #define TARGET_REZ_MAC_PPC        0
#endif

#undef  TARGET_REZ_MAC_X86
#if defined(__i386__) || defined(i386_YES)
  #define TARGET_REZ_MAC_X86        1
#else
  #define TARGET_REZ_MAC_X86        0
#endif

#undef  TARGET_REZ_MAC_PPC64
#if defined(__ppc64__) || defined(ppc64_YES)
  #define TARGET_REZ_MAC_PPC64      1
#else
  #define TARGET_REZ_MAC_PPC64      0
#endif

#undef  TARGET_REZ_MAC_X86_64
#if defined(__x86_64__) || defined(x86_64_YES)
  #define TARGET_REZ_MAC_X86_64     1
#else
  #define TARGET_REZ_MAC_X86_64     0
#endif

#if TARGET_OS_MAC
  #if TARGET_REZ_MAC_PPC && TARGET_REZ_MAC_X86 && TARGET_REZ_MAC_X86_64 && TARGET_REZ_MAC_PPC64
    #define TARGET_REZ_FAT_COMPONENTS_4 1
    #define Target_PlatformType     platformPowerPCNativeEntryPoint
    #define Target_SecondPlatformType platformIA32NativeEntryPoint
    #define Target_ThirdPlatformType  platformX86_64NativeEntryPoint
    #define Target_FourthPlatformType platformPowerPC64NativeEntryPoint
  #elif TARGET_REZ_MAC_PPC && TARGET_REZ_MAC_X86
    #define TARGET_REZ_FAT_COMPONENTS_2 1
    #define Target_PlatformType     platformPowerPCNativeEntryPoint
    #define Target_SecondPlatformType platformIA32NativeEntryPoint
  #elif TARGET_REZ_MAC_X86 && TARGET_REZ_MAC_X86_64
    #define TARGET_REZ_FAT_COMPONENTS_2 1
    #define Target_PlatformType     platformIA32NativeEntryPoint
    #define Target_SecondPlatformType platformX86_64NativeEntryPoint
  #elif TARGET_REZ_MAC_PPC && TARGET_REZ_MAC_PPC64
    #define TARGET_REZ_FAT_COMPONENTS_2 1
    #define Target_PlatformType     platformPowerPCNativeEntryPoint
    #define Target_SecondPlatformType platformPowerPC64NativeEntryPoint
  #elif TARGET_REZ_MAC_X86_64 && TARGET_REZ_MAC_PPC64
    #define TARGET_REZ_FAT_COMPONENTS_2 1
    #define Target_PlatformType     platformX86_64NativeEntryPoint
    #define Target_SecondPlatformType platformPowerPC64NativeEntryPoint
  #elif TARGET_REZ_MAC_X86
    #define Target_PlatformType     platformIA32NativeEntryPoint
  #elif TARGET_REZ_MAC_X86_64
    #define Target_PlatformType     platformX86_64NativeEntryPoint
  #elif TARGET_REZ_MAC_PPC64
    #define Target_PlatformType     platformPowerPC64NativeEntryPoint
  #elif TARGET_REZ_MAC_PPC
    #define Target_PlatformType     platformPowerPCNativeEntryPoint
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

resource 'STR ' (RES_ID, purgeable) {
  RES_NAME
};

resource 'STR ' (RES_ID + 1, purgeable) {
  PLUG_PUBLIC_NAME " AU"
};

resource 'dlle' (RES_ID) {
  PLUG_ENTRY_STR
};

resource 'thng' (RES_ID, RES_NAME) {
#if PLUG_IS_INST
kAudioUnitType_MusicDevice,
#elif PLUG_DOES_MIDI
kAudioUnitType_MusicEffect,
#else
kAudioUnitType_Effect,
#endif
  PLUG_UNIQUE_ID,
  PLUG_MFR_ID,
  0, 0, 0, 0,               //  no 68K
  'STR ', RES_ID,
  'STR ', RES_ID + 1,
  0,  0,      // icon 
  PLUG_VER,
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
#if TARGET_REZ_FAT_COMPONENTS_4
    cmpThreadSafeOnMac,
    Target_CodeResType, RES_ID,
    Target_ThirdPlatformType,
    cmpThreadSafeOnMac,
    Target_CodeResType, RES_ID,
    Target_FourthPlatformType,
#endif
  }
};

#undef RES_ID

#if TARGET_REZ_MAC_PPC || TARGET_REZ_MAC_X86

#define RES_ID 2000
#undef RES_NAME
#define RES_NAME PLUG_MFR ": " PLUG_PUBLIC_NAME " Carbon View"

resource 'STR ' (RES_ID, purgeable) {
  RES_NAME
};

resource 'STR ' (RES_ID + 1, purgeable) {
  PLUG_PUBLIC_NAME " AU Carbon View"
};

resource 'dlle' (RES_ID) {
  PLUG_VIEW_ENTRY_STR
};

resource 'thng' (RES_ID, RES_NAME) {
  kAudioUnitCarbonViewComponentType,
  PLUG_UNIQUE_ID,
  PLUG_MFR_ID,
  0, 0, 0, 0,               //  no 68K
  'STR ', RES_ID,
  'STR ', RES_ID + 1,
  0,  0,      // icon 
  PLUG_VER,
  componentHasMultiplePlatforms | componentDoAutoVersion,
  0,
  {
    cmpThreadSafeOnMac, 
    Target_CodeResType, RES_ID,
    Target_PlatformType,
#if TARGET_REZ_FAT_COMPONENTS_4 || (TARGET_REZ_FAT_COMPONENTS_2 && TARGET_REZ_MAC_PPC && TARGET_REZ_MAC_X86)
    cmpThreadSafeOnMac, 
    Target_CodeResType, RES_ID,
    Target_SecondPlatformType,
#endif
  }
};

#undef RES_ID

#endif // TARGET_REZ_MAC_PPC || TARGET_REZ_MAC_X86
