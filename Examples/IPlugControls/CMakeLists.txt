cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
cmake_policy(SET CMP0091 NEW)

#########
# This is a build file for iPlug2 on Linux.
# It may work on Windows and MacOS, but that isn't the initial goal.
#
# To setup the build (change CMAKE_BUILD_TYPE as desired):
#   cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Debug
# To build the VST2 version:
#   cmake --build build-linux --target IPlugControls-vst2
# To build the VST3 version:
#   cmake --build build-linux --target IPlugControls-vst3

project(IPlugControls VERSION 1.0.0 LANGUAGES C CXX)

set(IPLUG2_DIR ${CMAKE_SOURCE_DIR}/../..)
include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED COMPONENTS APP VST2 VST3 AU)

set(dir "${CMAKE_SOURCE_DIR}")
set(SRC_FILES
  "${dir}/config.h"
  "${dir}/IPlugControls.h"
  "${dir}/IPlugControls.cpp"
)
source_group(TREE ${dir} FILES ${SRC_FILES})
set(RES_FILES
  "${dir}/resources/fonts/Roboto-Regular.ttf"
  "${dir}/resources/fonts/fontaudio.ttf"
  "${dir}/resources/fonts/forkawesome-webfont.ttf"
  "${dir}/resources/img/button@2x.png"
  "${dir}/resources/img/button.png"
  "${dir}/resources/img/font@2x.png"
  "${dir}/resources/img/font.png"
  "${dir}/resources/img/hslider-handle.svg"
  "${dir}/resources/img/hslider-track.svg"
  "${dir}/resources/img/knob@2x.png"
  "${dir}/resources/img/knob.png"
  "${dir}/resources/img/knob-rotate@2x.png"
  "${dir}/resources/img/knob-rotate.png"
  "${dir}/resources/img/slider-handle@2x.png"
  "${dir}/resources/img/slider-handle.png"
  "${dir}/resources/img/slider-handle.svg"
  "${dir}/resources/img/slider-track@2x.png"
  "${dir}/resources/img/slider-track.png"
  "${dir}/resources/img/slider-track.svg"
  "${dir}/resources/img/switch@2x.png"
  "${dir}/resources/img/switch.png"
  "${dir}/resources/img/vector-knob.svg"
)
source_group(TREE "${dir}/resources" PREFIX Resources FILES ${RES_FILES})

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

