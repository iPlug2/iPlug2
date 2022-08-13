cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
cmake_policy(SET CMP0091 NEW)

#########


# To setup the build (change CMAKE_BUILD_TYPE as desired):
#   mkdir build-mac
#   cd build-mac
#   cmake .. -DCMAKE_BUILD_TYPE=Debug
# To build the APP version:
#   cmake --build . --target app -j
# To build the VST3 version:
#   cmake --build . --target vst3 -j
# To build the AUv2 version
#   cmake --build . --target auv2 -j

project(IPlugEffect VERSION 1.0.0 LANGUAGES C CXX)

set(IPLUG2_DIR ${CMAKE_SOURCE_DIR}/../..)
include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2)
#find_package(iPlug2 REQUIRED COMPONENTS Skia)

set(PROJECT_DIR "${CMAKE_SOURCE_DIR}")
set(SRC_FILES
  "${PROJECT_DIR}/config.h"
  "${PROJECT_DIR}/IPlugEffect.h"
  "${PROJECT_DIR}/IPlugEffect.cpp"
)
source_group(TREE ${PROJECT_DIR} FILES ${SRC_FILES})

set(RESOURCES
  "${PROJECT_DIR}/resources/fonts/Roboto-Regular.ttf"
)

# While not required, creating a base interface for includes and settings seems like a good idea.
add_library(_base INTERFACE)
# iplug_target_add() is a shorthand function for adding sources and include directories,
# linking libraries, adding resources, setting compile options, etc.
iplug_target_add(_base INTERFACE
  INCLUDE ${PROJECT_DIR} ${PROJECT_DIR}/resources
  LINK iPlug2_NanoVG_GL2
  FEATURE cxx_std_17)

# For typing convenience the TARGET name is put into a variable.
set(TARGET app)
add_executable(${TARGET} WIN32 MACOSX_BUNDLE ${SRC_FILES})
iplug_target_add(${TARGET} PUBLIC LINK iPlug2_APP _base RESOURCE ${RESOURCES})
iplug_configure_target(${TARGET} app)

#set(TARGET vst2)
#add_library(${TARGET} MODULE ${SRC_FILES})
#iplug_target_add(${TARGET} PUBLIC LINK iPlug2_VST2 _base RESOURCE ${RESOURCES})
#iplug_configure_target(${TARGET} vst2)

set(TARGET vst3)
add_library(${TARGET} MODULE ${SRC_FILES})
iplug_target_add(${TARGET} PUBLIC LINK iPlug2_VST3 _base RESOURCE ${RESOURCES})
iplug_configure_target(${TARGET} vst3)

if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(TARGET auv2)
  add_library(${TARGET} MODULE ${SRC_FILES})
  iplug_target_add(${TARGET} PUBLIC LINK iPlug2_AUv2 _base RESOURCE ${RESOURCES})
  iplug_configure_target(${TARGET} auv2)
endif()

if (WIN32)
  set(CMAKE_RC_FLAGS "/I${PROJECT_DIR}/resources /I${PROJECT_DIR}/resources/fonts /I${PROJECT_DIR}/resources/img ${CMAKE_RC_FLAGS}")
endif()
