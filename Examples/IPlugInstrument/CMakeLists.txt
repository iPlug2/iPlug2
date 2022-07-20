cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

#########
# This is a build file for iPlug2 on Linux.
# It may work on Windows and MacOS, but that isn't the initial goal.
#
# To setup the build (change CMAKE_BUILD_TYPE as desired):
#   cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Debug
# To build the VST2 version:
#   cmake --build build-linux --target IPlugInstrument-vst2
# To build the VST3 version:
#   cmake --build build-linux --target IPlugInstrument-vst3

project(IPlugInstrument VERSION 1.0.0)

set(IPLUG2_DIR ${CMAKE_SOURCE_DIR}/../..)
include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

set(dir "${CMAKE_SOURCE_DIR}")
set(SRC_FILES
  "${dir}/config.h"
  "${dir}/IPlugInstrument.h"
  "${dir}/IPlugInstrument.cpp"
  "${dir}/IPlugInstrument_DSP.h"
)
source_group(TREE ${dir} FILES ${SRC_FILES})
set(RES_FILES
  "${dir}/resources/fonts/Roboto-Regular.ttf"
)
source_group(TREE "${dir}/resources" PREFIX Resources FILES ${RES_FILES})

# Disable compiler-dependent C++ extension features (optional, but recommended for code portability)
set(CMAKE_CXX_EXTENSIONS OFF)

# While not required, creating a base interface for includes and settings seems like a good idea.
add_library(_base INTERFACE)
# iplug_target_add() is a shorthand function for adding sources and include dirs,
# linking libraries, adding resources, setting compile options, etc.
iplug_target_add(_base INTERFACE
  INCLUDE ${dir} ${dir}/resources
  LINK iPlug2_Synth iPlug2_NANOVG iPlug2_GL2)
# Use C++ 14 standard
target_compile_features(_base INTERFACE cxx_std_17)


add_executable(App WIN32 MACOSX_BUNDLE ${SRC_FILES} ${RES_FILES})
iplug_target_add(App PUBLIC LINK _base RESOURCE ${RES_FILES})
# You MUST call iplug_configure_target(<target_name> <app|vst2|vst3|...>) for things to build correctly.
iplug_configure_target(App app)

add_library(VST2 MODULE ${SRC_FILES} ${RES_FILES})
iplug_target_add(VST2 PUBLIC LINK _base RESOURCE ${RES_FILES})
iplug_configure_target(VST2 vst2)

add_library(VST3 MODULE ${SRC_FILES} ${RES_FILES})
iplug_target_add(VST3 PUBLIC LINK _base RESOURCE ${RES_FILES})
iplug_configure_target(VST3 vst3)

if (APPLE)
  add_library(AUv2 MODULE ${SRC_FILES} ${RES_FILES})
  iplug_target_add(AUv2 PUBLIC LINK _base RESOURCE ${RES_FILES})
  iplug_configure_target(AUv2 au2)
endif()

