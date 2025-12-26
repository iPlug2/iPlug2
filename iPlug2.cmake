#  ==============================================================================
#  
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# This file should be included in your main CMakeLists.txt file.

if (APPLE)
  enable_language(OBJC)
  enable_language(OBJCXX)

  # Universal binary support (Intel + Apple Silicon)
  option(IPLUG2_UNIVERSAL "Build universal binaries (arm64 + x86_64)" OFF)
  if(IPLUG2_UNIVERSAL)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures" FORCE)
    # For Xcode generator, also set the Xcode-specific attributes
    set(CMAKE_XCODE_ATTRIBUTE_ARCHS "arm64 x86_64")
    set(CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "NO")
  endif()

  # macOS deployment target
  if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
  endif()

  # Code signing for AUv3 (required for registration)
  set(IPLUG2_DEVELOPMENT_TEAM "" CACHE STRING "Apple Development Team ID for code signing (required for AUv3)")
  if(IPLUG2_DEVELOPMENT_TEAM)
    set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${IPLUG2_DEVELOPMENT_TEAM}")
  endif()
endif()

# Tracer build support - enables TRACER_BUILD preprocessor define for profiling/tracing
option(IPLUG2_TRACER "Enable tracer build (adds TRACER_BUILD define)" OFF)
if(IPLUG2_TRACER)
  add_compile_definitions(TRACER_BUILD)
endif()

set(IPLUG2_CXX_STANDARD 17)

# Set C++ standard globally to ensure PCH and all files compile with C++17
set(CMAKE_CXX_STANDARD ${IPLUG2_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add the iPlug2 cmake module path
set(IPLUG2_CMAKE_DIR ${IPLUG2_DIR}/Scripts/cmake)
list(APPEND CMAKE_MODULE_PATH ${IPLUG2_CMAKE_DIR})

# Make sure MSVC uses static linking
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Generate folders for generators that support it (Visual Studio, Xcode, etc.)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Debug host application for Visual Studio debugging
set(_default_debug_host "")
if(WIN32)
  set(_reaper_path "C:/Program Files/REAPER (x64)/reaper.exe")
  if(EXISTS "${_reaper_path}")
    set(_default_debug_host "${_reaper_path}")
  endif()
endif()
set(IPLUG2_DEBUG_HOST "${_default_debug_host}" CACHE FILEPATH "Host application for debugging plugins (e.g., path to REAPER, Ableton, etc.)")
set(IPLUG2_DEBUG_HOST_ARGS "" CACHE STRING "Command line arguments for the debug host application")