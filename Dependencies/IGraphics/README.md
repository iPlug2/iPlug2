# IGraphics dependencies

This folder contains IGraphics dependencies, which may or may not be needed depending on which backend you want to use. Some dependencies (NanoVG, NanoSVG) are included directly in the iPlug2 repository, but others must be downloaded or built locally as static libraries. 

## Downloading prebuilt libraries

A zip file with prebuilt static libraries, including igraphics libraries (and header includes etc) and may be downloaded for each platform from the github releases page https://github.com/iPlug2/iPlug2/releases.

The contents of the zip files IPLUG2_DEPS_***.zip should be extracted to iPlug2/Dependencies/Build

You can download the zip file for your platform by running the shell script **download-prebuilt-libs.sh**, (located in the folder above this one) from a posix terminal (macOS terminal or windows git bash/mingw terminal) 

## Building libraries locally

To build libraries locally, you must first download the tarballs for each library. To do this execute the shell script **download-libs-src.sh** from a posix terminal. 

### Mac
Execute the build script **build-libs-mac.sh**, which will build all the libraries required for IGraphics on macOS, and will install them in a unix style hierarchy in the folder **iPlug2/Dependencies/Build/mac**. Build settings defined in **iPlug2/common-mac.xcconfig**  will allow your plug-in project to link to these libraries.

###  Windows
Execute the Windows batch script **build-libs-win.bat** from a regular Windows command prompt (cmd.exe). This will compile all the static libraries (for debug/release) configurations and (win32/x64 architectures) using the IGraphicsLibraries visual studio solution. 
