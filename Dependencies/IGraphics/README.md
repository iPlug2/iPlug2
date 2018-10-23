# IGraphics dependencies

This folder contains IGraphics dependencies, which may or may not be needed depending on which backend you want to use. Some  dependencies are included directly in the iPlug 2 repository, but others must be downloaded and built as static libraries. Alternatively prebuilt  static libraries may be downloaded.

## Mac
The build script **build-igraphics-libs-mac.sh** will download and build all the libraries required for IGraphics, and will install them in a unix style hierarchy in the folder **iPlug2/Dependencies/Build/mac**. Build settings defined in **iPlug2/common-mac.xcconfig**  will allow your plug-in project to link to these libraries.

##  Windows
Windows static libraries are built in two stages and source files are located differently. First open a command prompt using git-bash and navigate to **iPlug2/Dependencies/IGraphics**.  Execute the shell script **download-igraphics-libs-win.sh**. This will download the various dependencies and move the source code into the correct folders. Once  that is complete,  launch a regular Windows command prompt and execute the Windows batch script **build-igraphics-libs-win.bat**,  which will compile all the static libraries (for debug/release) configurations and (win32/x64  architectures) using the IGraphicsLibraries visual studio solution. 
