extract PT_90_PlugInSDK.zip here

- run config_SDK_for_Mac.command

- if you want to compile with Xcode3 on Snow Leopard...

update PluginLibrary.xcodeproj so that the base SDK is set to 10.5, compiler is com.apple.compilers.gcc.4_2 and arch is i386

- if you want to compile with Xcode4 on Lion...

make sure you have updated common.xcconfig to use 10.6 SDK and com.apple.compilers.llvmgcc42

update PluginLibrary.xcodeproj so that the base SDK is set to 10.6, compiler is com.apple.compilers.llvmgcc42 and arch is i386

comment out line 31 of /PT9_SDK/AlturaPorts/TDMPlugIns/PlugInLibrary/Utilities/FicBasics.h

// #include <ppc_intrinsics.h>  // needed for __eieio
