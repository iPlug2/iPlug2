# IGraphics dependencies

This folder contains IGraphics dependencies, which may or may not be needed depending on which backend you want to use. Some dependencies are included directly in the iPlug 2 repository, but for e.g. SKIA pre-built libraries can either be [downloaded](https://github.com/iPlug2/iPlug2/tree/master/Dependencies) or you can build them yourself using the scripts here, which can be useful if you need to customize something. 

If you want to build the libraries yourself, you need to do things in the right order.

## All platforms

You need to make sure that you have Python3 installed and that it is callable via `python3`

First open a unix shell prompt (via git-bash on windows) and navigate to **iPlug2/Dependencies/IGraphics**. Execute the shell script **download-igraphics-libs.sh**. e.g.

```
$ cd ~/Dev/iPlug2/Dependencies/IGraphics
$ ./download-igraphics-libs.sh
```

## Mac
The build script **build-igraphics-libs-mac.sh** will build freetype as a static library (only required if you are using IGRAPHICS_FREETYPE with IGRAPHICS_NANOVG) and will install it in a unix style hierarchy in the folder **iPlug2/Dependencies/Build/mac**. Build settings defined in **iPlug2/common-mac.xcconfig**  will allow your plug-in project to link to these libraries. The libraries are built as universal binaries with x86_64 and arm64 architectures.

Next you can execute **build-skia-mac.sh** which will build the various static libraries for skia and place the files in the right locations.

## iOS

Execute **build-skia-ios.sh** with either arm64 or x64 as a single argument, to build for the device or simulator (TODO: what about simulator on ARM?)

## Windows
Launch a regular Windows command prompt (cmd.exe) and execute the Windows batch script **build-igraphics-libs-win.bat**, which will compile all the static libraries (for debug/release) configurations and (win32/x64 architectures) using the IGraphicsLibraries visual studio solution.

You need to make sure that you have LLVM installed and that it is installed in C://ProgramFiles/LLVM

Now launch a shell with git-bash and execute **build-skia-win.sh** which will build the various static libraries for skia and place the files in the right locations. You need to add arguments for configuration and architechture e.g. 

```
$ cd ~/Dev/iPlug2/Dependencies/IGraphics
$ ./build-skia-win.sh Release x64
```

