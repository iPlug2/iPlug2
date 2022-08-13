cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
cmake_policy(SET CMP0091 NEW)

#########

# To setup the build (change CMAKE_BUILD_TYPE as desired):
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
# To build the VST2 version:
#   cmake --build build --target IPlugEffect-vst2
# To build the VST3 version:
#   cmake --build build --target IPlugEffect-vst3
# To build the AUv2 version (macOS only):
#   cmake --build build --target IPlugEffect-auv2

project(IPlugEffect VERSION 1.0.0 LANGUAGES C CXX)

set(IPLUG2_DIR ${CMAKE_SOURCE_DIR}/../..)
include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

set(dir "${CMAKE_SOURCE_DIR}")
set(SRC_FILES
  "${dir}/config.h"
  "${dir}/IPlugEffect.h"
  "${dir}/IPlugEffect.cpp"
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
  LINK iPlug2_Synth iPlug2_NANOVG iPlug2_GL2
  FEATURE cxx_std_17)

# For typing convenience the target name is put into a variable.
set(target ${CMAKE_PROJECT_NAME}-app)
add_executable(${target} WIN32 MACOSX_BUNDLE ${SRC_FILES})
iplug_target_add(${target} PUBLIC LINK iPlug2_APP _base RESOURCE ${RES_FILES})
# You MUST call iplug_configure_target(<target_name> <app|vst2|vst3|...>) for things to build correctly.
iplug_configure_target(${target} app)

# set(target ${CMAKE_PROJECT_NAME}-vst2)
# add_library(${target} MODULE ${SRC_FILES})
# iplug_target_add(${target} PUBLIC LINK iPlug2_VST2 _base RESOURCE ${RES_FILES})
# iplug_configure_target(${target} vst2)

set(target ${CMAKE_PROJECT_NAME}-vst3)
add_library(${target} MODULE ${SRC_FILES})
iplug_target_add(${target} PUBLIC LINK iPlug2_VST3 _base RESOURCE ${RES_FILES})
iplug_configure_target(${target} vst3)

if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(target ${CMAKE_PROJECT_NAME}-auv2)
  add_library(${target} MODULE ${SRC_FILES})
  iplug_target_add(${target} PUBLIC LINK iPlug2_AUv2 _base RESOURCE ${RES_FILES})
  iplug_configure_target(${target} auv2)
endif()

if (WIN32)
  set(CMAKE_RC_FLAGS "/I${dir}/resources /I${dir}/resources/fonts /I${dir}/resources/img ${CMAKE_RC_FLAGS}")
endif()
