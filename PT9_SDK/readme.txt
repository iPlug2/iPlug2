extract PT_90_PlugInSDK.zip here

- run config_SDK_for_Mac.command

make the following mods to compile on 10.68 / Xcode 3.26

update PluginLibrary.xcodeproj so that the base SDK is set to 10.5, compiler is com.apple.compilers.gcc.4_2 and arch is i386

make the following mods to compile on 10.8+ / Xcode 5

make sure you have updated common.xcconfig to use 10.6 SDK and com.apple.compilers.llvm.clang

update PluginLibrary.xcodeproj so that the base SDK is set to 10.6, compiler is com.apple.compilers.llvm.clang and arch is i386

comment out line 31 of /PT9_SDK/AlturaPorts/TDMPlugIns/PlugInLibrary/Utilities/FicBasics.h

// #include <ppc_intrinsics.h>  // needed for __eieio
