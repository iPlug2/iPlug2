cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
cmake_policy(SET CMP0091 NEW)

project(IPlugEffect VERSION 1.0.0 LANGUAGES C CXX)

# Set iPlug2 directory and include iPlug2 CMake files
set(IPLUG2_DIR ${CMAKE_SOURCE_DIR}/../..)
include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

# Uncomment the following line if you need Skia support
# find_package(iPlug2 REQUIRED COMPONENTS Skia)

# Set project directories and source files
set(PROJECT_DIR "${CMAKE_SOURCE_DIR}")
set(SRC_FILES
  "${PROJECT_DIR}/config.h"
  "${PROJECT_DIR}/IPlugEffect.h"
  "${PROJECT_DIR}/IPlugEffect.cpp"
)
source_group(TREE ${PROJECT_DIR} FILES ${SRC_FILES})

# Set resource files
set(RESOURCES
  "${PROJECT_DIR}/resources/fonts/Roboto-Regular.ttf"
)

if(APPLE)
  # Enable universal binary build for Release builds on macOS
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for macOS" FORCE)
  else()
    set(CMAKE_OSX_ARCHITECTURES "${CMAKE_HOST_SYSTEM_PROCESSOR}" CACHE STRING "Build architectures for macOS" FORCE)
  endif()
  # Set Xcode attributes for universal binary build
  set_target_properties(${TARGET} PROPERTIES XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH[variant=Debug] YES)

  # Set Xcode attributes for code signing
  # set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM 686EDA2T8T) 
  # set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application") 
  # set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual")

endif()

# Create base interface library for common settings
add_library(_base INTERFACE)
iplug_target_add(_base INTERFACE
  INCLUDE ${PROJECT_DIR} ${PROJECT_DIR}/resources
  # Replace with e.g. iPlug2_Skia_GL2 if you need Skia support
  LINK iPlug2_NanoVG_GL2
  FEATURE cxx_std_17
)

# APP target configuration
set(TARGET app)
add_executable(${TARGET} WIN32 MACOSX_BUNDLE ${SRC_FILES})
iplug_target_add(${TARGET} PUBLIC 
  LINK iPlug2_APP _base 
  RESOURCE ${RESOURCES}
)
iplug_configure_target(${TARGET} app)

# CLAP target configuration
set(TARGET clap)
add_library(${TARGET} MODULE ${SRC_FILES})
iplug_target_add(${TARGET} PUBLIC 
  LINK iPlug2_CLAP _base 
  RESOURCE ${RESOURCES}
)
iplug_configure_target(${TARGET} clap)

# VST2 target configuration
set(TARGET vst2)
add_library(${TARGET} MODULE ${SRC_FILES})
iplug_target_add(${TARGET} PUBLIC 
  LINK iPlug2_VST2 _base 
  RESOURCE ${RESOURCES}
)
iplug_configure_target(${TARGET} vst2)

# VST3 target configuration
set(TARGET vst3)
add_library(${TARGET} MODULE ${SRC_FILES})
iplug_target_add(${TARGET} PUBLIC 
  LINK iPlug2_VST3 _base 
  RESOURCE ${RESOURCES}
)
iplug_configure_target(${TARGET} vst3)

# AUv2 target configuration (macOS only)
if(APPLE)
  set(TARGET auv2)
  add_library(${TARGET} MODULE ${SRC_FILES})
  iplug_target_add(${TARGET} PUBLIC 
    LINK iPlug2_AUv2 _base 
    RESOURCE ${RESOURCES}
  )
  iplug_configure_target(${TARGET} auv2)
endif()

# Windows-specific resource configuration
if(WIN32)
  set(CMAKE_RC_FLAGS "/I${PROJECT_DIR}/resources /I${PROJECT_DIR}/resources/fonts /I${PROJECT_DIR}/resources/img ${CMAKE_RC_FLAGS}")
endif()

iplug_target_debug_xcode(vst3 "/Applications/REAPER.app" "")
iplug_target_debug_xcode(vst2 "/Applications/REAPER.app" "")
iplug_target_debug_xcode(clap "/Applications/REAPER.app" "")
iplug_target_debug_xcode(auv2 "/Applications/REAPER.app" "")