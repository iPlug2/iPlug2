# IGraphics dependencies

This folder contains IGraphics dependencies, which may or may not be needed depending on which backend you want to use. Some dependencies (NanoVG, NanoSVG) are included directly in the iPlug 2 repository, but others must be downloaded and built as static libraries. Alternatively a zip file with prebuilt static libraries (and header includes etc) and may be downloaded from the github releases page.

## Mac
The build script **build-igraphics-libs-mac.sh** will build all the libraries required for IGraphics, and will install them in a unix style hierarchy in the folder **iPlug2/Dependencies/Build/mac**. Build settings defined in **iPlug2/common-mac.xcconfig**  will allow your plug-in project to link to these libraries. Libraries are built as universal binaries.

##  Windows
Windows static libraries are built in two stages and source files are located differently. First open a command prompt using git-bash and navigate to **iPlug2/Dependencies/IGraphics**.  Execute the shell script **download-igraphics-libs.sh**. 

```
Oliver Larkin@Oli-PC MINGW64 ~/Dev/iPlug2/Dependencies/IGraphics (master)
$ ./download-igraphics-libs.sh
```

This will download the various dependencies and move the source code into the correct folders. Once  that is complete,  launch a regular Windows command prompt and execute the Windows batch script **build-igraphics-libs-win.bat**,  which will compile all the static libraries (for debug/release) configurations and (win32/x64  architectures) using the IGraphicsLibraries visual studio solution. 
