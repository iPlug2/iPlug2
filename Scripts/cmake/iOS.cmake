#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# iOS platform configuration for iPlug2
# Handles SDK selection, deployment targets, and platform-specific settings

# Only proceed if building for iOS
if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
  return()
endif()

# Set IOS variable for use in conditionals throughout the build
set(IOS TRUE)

# iOS platform option: OS (device), SIMULATOR, or VISIONOS
if(NOT DEFINED IPLUG2_IOS_PLATFORM)
  set(IPLUG2_IOS_PLATFORM "OS" CACHE STRING "iOS platform to build for (OS, SIMULATOR, VISIONOS, VISIONOS_SIMULATOR)")
endif()

# Configure SDK based on platform
if(IPLUG2_IOS_PLATFORM STREQUAL "SIMULATOR")
  set(CMAKE_OSX_SYSROOT "iphonesimulator" CACHE STRING "iOS SDK")
  set(IPLUG2_IOS_SDK "iphonesimulator")
elseif(IPLUG2_IOS_PLATFORM STREQUAL "VISIONOS")
  set(CMAKE_OSX_SYSROOT "xros" CACHE STRING "visionOS SDK")
  set(IPLUG2_IOS_SDK "xros")
elseif(IPLUG2_IOS_PLATFORM STREQUAL "VISIONOS_SIMULATOR")
  set(CMAKE_OSX_SYSROOT "xrsimulator" CACHE STRING "visionOS Simulator SDK")
  set(IPLUG2_IOS_SDK "xrsimulator")
else()
  # Default to device SDK
  set(CMAKE_OSX_SYSROOT "iphoneos" CACHE STRING "iOS SDK")
  set(IPLUG2_IOS_SDK "iphoneos")
endif()

# iOS deployment target
if(IPLUG2_IOS_PLATFORM MATCHES "VISIONOS")
  if(NOT DEFINED IPLUG2_XROS_DEPLOYMENT_TARGET)
    set(IPLUG2_XROS_DEPLOYMENT_TARGET "1.0" CACHE STRING "Minimum visionOS version")
  endif()
  set(CMAKE_XCODE_ATTRIBUTE_XROS_DEPLOYMENT_TARGET ${IPLUG2_XROS_DEPLOYMENT_TARGET})
else()
  if(NOT DEFINED IPLUG2_IOS_DEPLOYMENT_TARGET)
    set(IPLUG2_IOS_DEPLOYMENT_TARGET "14" CACHE STRING "Minimum iOS version")
  endif()
  set(CMAKE_OSX_DEPLOYMENT_TARGET ${IPLUG2_IOS_DEPLOYMENT_TARGET})
  set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET ${IPLUG2_IOS_DEPLOYMENT_TARGET})
endif()

# Set architectures based on platform
if(IPLUG2_IOS_PLATFORM STREQUAL "SIMULATOR")
  # Simulator supports both arm64 (Apple Silicon Mac) and x86_64 (Intel Mac)
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "iOS Simulator architectures")
elseif(IPLUG2_IOS_PLATFORM MATCHES "VISIONOS")
  set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "visionOS architectures")
else()
  # Device is arm64 only (since iOS 11)
  set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "iOS device architectures")
endif()

# Xcode-specific settings
if(XCODE)
  # Set SDK root for Xcode
  set(CMAKE_XCODE_ATTRIBUTE_SDKROOT ${CMAKE_OSX_SYSROOT})

  # Targeted device family: 1=iPhone, 2=iPad, 1,2=Universal
  set(CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" CACHE STRING "iOS device family")

  # Skip install for iOS builds (not deploying to system locations)
  set(CMAKE_XCODE_ATTRIBUTE_SKIP_INSTALL "YES")

  # Enable bitcode (optional, deprecated in Xcode 14+)
  set(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE "NO")
endif()

message(STATUS "iPlug2 iOS: Platform=${IPLUG2_IOS_PLATFORM}, SDK=${CMAKE_OSX_SYSROOT}, Deployment=${CMAKE_OSX_DEPLOYMENT_TARGET}")
