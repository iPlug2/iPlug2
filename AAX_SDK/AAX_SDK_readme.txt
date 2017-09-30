extract AAX_SDK_2p3p0.zip here 

  - this assumes that you have installed the MacOSX10.X.sdk in /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/

  - set SDKROOT to macosx10.X for debug and release targets of AAXLibrary static lib
 
  - modify ExamplePlugIns/Common/Mac/CommonDebugSettings.xcconfig and ExamplePlugIns/Common/Mac/CommonReleaseSettings.xcconfig...

  GCC_VERSION = com.apple.compilers.llvm.clang
  SDKROOT = macosx10.X
  MACOSX_DEPLOYMENT_TARGET = 10.X
  ARCHS = x86_64 i386

On Windows you will need to set the Libs/AAXLibrary/WinBuild/AAXLibrary.vcxproj project to link statically to the MSVC runtime library - change to /MT rather than /MD