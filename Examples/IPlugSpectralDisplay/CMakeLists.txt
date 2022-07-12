cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
cmake_policy(SET CMP0091 NEW)

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

project(IPlugSpectralDisplay VERSION 1.0.0 LANGUAGES C CXX)

set(IPLUG2_DIR ${CMAKE_SOURCE_DIR}/../..)
include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

include(${IPLUG2_DIR}/Scripts/cmake/WDL.cmake)

set(dir "${CMAKE_SOURCE_DIR}")
set(SRC_FILES
  "${dir}/config.h"
  "${dir}/IPlugSpectralDisplay.h"
  "${dir}/IPlugSpectralDisplay.cpp"
)
source_group(TREE ${dir} FILES ${SRC_FILES})

set(RES_FILES
  "${dir}/resources/fonts/Roboto-Regular.ttf"
)

# While not required, creating a base interface for includes and settings seems like a good idea.
add_library(_base INTERFACE)
# iplug_target_add() is a shorthand function for adding sources and include dirs,
# linking libraries, adding resources, setting compile options, etc.
iplug_target_add(_base INTERFACE
  INCLUDE ${dir} ${dir}/resources
  LINK iPlug2_Synth iPlug2_NANOVG iPlug2_GL2)

# For whatever reason setting CXX_STANDARD doesn't seem to work properly, so set it explicitly.
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(_base INTERFACE "$<$<COMPILE_LANGUAGE:CXX>:/std:c++14>")
else()
  target_compile_options(_base INTERFACE "$<$<COMPILE_LANGUAGE:CXX>:-std=c++14>")
endif()

# For typing convenience the target name is put into a variable.
set(tgt ${CMAKE_PROJECT_NAME}-app)
add_executable(${tgt} WIN32 MACOSX_BUNDLE ${SRC_FILES})
iplug_target_add(${tgt} PUBLIC LINK iPlug2_APP _base _wdl RESOURCE ${RES_FILES})
# You MUST call iplug_configure_target(<target_name> <app|vst2|vst3|...>) for things to build correctly.
iplug_configure_target(${tgt} app)

set(tgt ${CMAKE_PROJECT_NAME}-vst2)
add_library(${tgt} MODULE ${SRC_FILES})
iplug_target_add(${tgt} PUBLIC LINK iPlug2_VST2 _base _wdl RESOURCE ${RES_FILES})
iplug_configure_target(${tgt} vst2)

set(tgt ${CMAKE_PROJECT_NAME}-vst3)
add_library(${tgt} MODULE ${SRC_FILES})
iplug_target_add(${tgt} PUBLIC LINK iPlug2_VST3 _base _wdl RESOURCE ${RES_FILES})
iplug_configure_target(${tgt} vst3)

