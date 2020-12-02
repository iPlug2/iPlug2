# CMake and iPlug2

This documentation should help those trying to build iPlug2 projects using
CMake. iPlug2 is a complex project and building it correctly on all supported
platforms, with different compilers, IDEs, and plugin types complicates things
even more.


## Loading iPlug2's CMake scripts
Loading iPlug2's CMake code is fairly easy, just a few lines.

```
set(IPLUG2_DIR path/to/iPlug2)
include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED COMPONENTS APP VST3 NanoVG)
```

The first line isn't strictly necessary, but it allows you to change the location
of the iPlug2 directory without changing your code by adding
`-DIPLUG2_DIR=/my/real/path/to/iPlug2` on the cmake command line. The `include`
tells CMake where to find the rest of the iPlug2 CMake scripts and sets some
other variables. Finally `find_package` actually loads the build scripts, and
requests support for building standalone applications and VST3 plugins using
the NanoVG graphics backend.

## Adding a Target
Building an application with iPlug2 isn't to different from normal CMake.
First add source files and resource files.
```
set(SRC_FILES
  config.h
  IPlugInstrument.h
  IPlugInstrument.cpp
  IPlugInstrument_DSP.h)
set(RES_FILES
  resources/fonts/Roboto-Regular.ttf)
```

Create a target using those files, notice that both source files *and* resources are included as "source files". iPlug2 copies any files listed as resource files to the correct locations, but many platforms also need them to be listed as source files.
```
add_executable(App WIN32 MACOSX_BUNDLE ${SRC_FILES} ${RES_FILES})
```

The next step is to set properties and link libraries. iPlug2 has a convenient function for this named `iplug_target_add(...)`. This example links `iPlug2_NANOVG` and `iPlug2_GL2` to use the NanoVG graphics backend with OpenGL 2. It also links `iPlug2_Synth` to include code from `iPlug2/IPlug/Extras/Synth`.
```
iplug_target_add(App PUBLIC
  INCLUDE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/resources
  LINK iPlug2_Synth iPlug2_NANOVG iPlug2_GL2
  RESOURCE ${RES_FILES}
)
```
The above code is equivalent to
```
target_include_directories(App PUBLIC ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/resources)
target_link_libraries(App PUBLIC iPlug2_Synth iPlug2_NANOVG iPlug2_GL)
set_property(TARGET ${target} APPEND PROPERTY RESOURCE ${RES_FILES})
```
Finally call `iplug_configure_target(...)` to make sure it builds correctly (this is required for all plugins/apps/etc.).
```
iplug_configure_target(App app)
```

Below is the code for the VST3 version. `SRC_FILES` and `RES_FILES` don't need to be re-defined. 
```
add_library(VST3 MODULE ${SRC_FILES} ${RES_FILES})
iplug_target_add(VST3 PUBLIC
  INCLUDE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/resources
  LINK iPlug2_Synth iPlug2_NANOVG iPlug2_GL2
  RESOURCE ${RES_FILES}
)
iplug_configure_target(VST3 vst3)
```

## Components
iPlug2 supports many different configurations and a single project won't require
all of them. To help keep things organized only components that are listed
in `find_package` will be loaded (with some exceptions). 

List of components:
* AAX - Support for Avid AAX plugins (WIP)
* APP - Support for standalone applications
* AU - Support for Audio Unit v2 and Audio Unit v3 plugins (ignored on non-Apple platforms) (WIP)
* LV2 - Support for LV2 plugins (WIP)
* VST2 - Support for VST2 plugins
* VST3 - Support for VST3 plugins
* WEB - Support for Web Audio Modules (WIP)
* NanoVG - NanoVG graphics backend (Not yet a component)
* Skia - Skia graphics backend



## CMake Variables
This is a list of common variables you might want to set when building with
CMake. To set one use `-DMY_VARIABLE=value` on the CMake command line, or
change the value using the CMake GUI.

* `CMAKE_BUILD_TYPE=<Debug|Release>`: Useful for Makefile and Ninja generators
  otherwise CMake will default to not including debug data.
* `VST2_SDK=<path>`: Path to the VST2_SDK directory (it should be named VST2_SDK).
  Defaults to `iPlug2/Dependencies/IPlug/VST2_SDK`.
* `VST3_SDK=<path>`: Path to the VST3_SDK directory (it should be named VST3_SDK).
  `iPlug2/Dependencies/IPlug/VST3_SDK`
* `VST2_INSTALL_PATH=<path>`: Path to install VST2 plugins. The default is almost
  always correct, but you might have an unusual setup.
* `VST3_INSTALL_PATH=<path>`: Path to install VST3 plugins. The default is almost
  always correct, but you might have an unusual setup.
* `PLUG_NAME=<name>`: Name of the plugin (e.g. IPlugInstrument). This defaults
  to the CMake project name.

