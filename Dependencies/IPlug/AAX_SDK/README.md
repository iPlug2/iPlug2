extract **AAX_SDK_2p3p2.zip** here 

# macOS

  - You need to modify the AAXlibrary project in order to compile with the same SDK as your plug-ins

  - set SDKROOT to macosx10.x for debug and release targets of AAXLibrary static lib
 
  - modify **ExamplePlugIns/Common/Mac/CommonDebugSettings.xcconfig** and **ExamplePlugIns/Common/Mac/CommonReleaseSettings.xcconfig**...

  GCC_VERSION = com.apple.compilers.llvm.clang  
  SDKROOT = macosx10.X  
  MACOSX_DEPLOYMENT_TARGET = 10.X  
  ARCHS = x86_64  

# Windows

On Windows you will need to 

  - Open **msvc/AAX_SDK.sln** in Visual Studio 2019. You should be prompted to retarget the solution, agree to that to upgrade the compiler and VS projects for VS2019
  
  - Right click the "AAXLibrary" project in the solution explorer and select "Properties". Navigate to **Configurations Properties -> C/C++ -> Code Generation -> Runtime Library** . For the Configurations/Platform Debug/x64 this should be set to **Multi-threaded Debug (/MTd)**, change it and click apply. For the Release/x64 platform it should be set to **Multi-threaded (/MT)**, change it and click apply.

  - Now build both Debug and Release versions of the AAXLibrary project.

