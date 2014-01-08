extract AAX_SDK_2p1p0.zip here 

make the following mods to compile on 10.68 / Xcode 3.26

  - set SDKROOT to macosx10.5 for debug and release targets of AAXLibrary static lib
 
  - modify ExamplePlugIns/Common/Mac/CommonDebugSettings.xcconfig and ExamplePlugIns/Common/Mac/CommonReleaseSettings.xcconfig...

  GCC_VERSION = com.apple.compilers.gcc.4_2
  SDKROOT = macosx10.5
  MACOSX_DEPLOYMENT_TARGET = 10.5
  ARCHS = x86_64 i386

make the following mods to compile on 10.8+ / Xcode 5

  - this assumes that you have installed the MacOSX10.6.sdk in /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/

  - set SDKROOT to macosx10.6 for debug and release targets of AAXLibrary static lib
 
  - modify ExamplePlugIns/Common/Mac/CommonDebugSettings.xcconfig and ExamplePlugIns/Common/Mac/CommonReleaseSettings.xcconfig...

  GCC_VERSION = com.apple.compilers.llvm.clang
  SDKROOT = macosx10.6
  MACOSX_DEPLOYMENT_TARGET = 10.5
  ARCHS = x86_64 i386

On Windows you will need to set the Libs/AAXLibrary/WinBuild/AAXLibrary.vcxproj project to link statically to the MSVC2010 runtime library - change to /MT rather than /MD