#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# iOS platform configuration for iPlug2
# Handles SDK selection, deployment targets, and platform-specific settings

# Only proceed if building for iOS or visionOS
if(NOT CMAKE_SYSTEM_NAME MATCHES "^(iOS|visionOS)$")
  return()
endif()

# Set IOS variable for use in conditionals throughout the build
set(IOS TRUE)

# Detect visionOS from CMAKE_SYSTEM_NAME (CMake 3.28+)
if(CMAKE_SYSTEM_NAME STREQUAL "visionOS")
  set(VISIONOS TRUE)
endif()

# iOS platform option: OS (device), SIMULATOR, or VISIONOS
# Auto-detect from CMAKE_SYSTEM_NAME for visionOS (CMake 3.28+)
if(NOT DEFINED IPLUG2_IOS_PLATFORM)
  if(VISIONOS)
    # Detect simulator vs device from CMAKE_OSX_SYSROOT if set
    if(CMAKE_OSX_SYSROOT MATCHES "simulator")
      set(IPLUG2_IOS_PLATFORM "VISIONOS_SIMULATOR" CACHE STRING "iOS platform")
    else()
      set(IPLUG2_IOS_PLATFORM "VISIONOS" CACHE STRING "iOS platform")
    endif()
  else()
    set(IPLUG2_IOS_PLATFORM "OS" CACHE STRING "iOS platform to build for (OS, SIMULATOR, VISIONOS, VISIONOS_SIMULATOR)")
  endif()
endif()

# Configure SDK based on platform
# Note: FORCE is required because CMake's iOS toolchain sets CMAKE_OSX_SYSROOT before our code runs
if(IPLUG2_IOS_PLATFORM STREQUAL "SIMULATOR")
  set(CMAKE_OSX_SYSROOT "iphonesimulator" CACHE STRING "iOS SDK" FORCE)
  set(IPLUG2_IOS_SDK "iphonesimulator")
elseif(IPLUG2_IOS_PLATFORM STREQUAL "VISIONOS")
  set(CMAKE_OSX_SYSROOT "xros" CACHE STRING "visionOS SDK" FORCE)
  set(IPLUG2_IOS_SDK "xros")
elseif(IPLUG2_IOS_PLATFORM STREQUAL "VISIONOS_SIMULATOR")
  set(CMAKE_OSX_SYSROOT "xrsimulator" CACHE STRING "visionOS Simulator SDK" FORCE)
  set(IPLUG2_IOS_SDK "xrsimulator")
else()
  # Default to device SDK
  set(CMAKE_OSX_SYSROOT "iphoneos" CACHE STRING "iOS SDK" FORCE)
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
# Note: FORCE is required because CMake's toolchain may set CMAKE_OSX_ARCHITECTURES before our code runs
if(IPLUG2_IOS_PLATFORM STREQUAL "SIMULATOR")
  # iOS Simulator supports both arm64 (Apple Silicon Mac) and x86_64 (Intel Mac)
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "iOS Simulator architectures" FORCE)
elseif(IPLUG2_IOS_PLATFORM STREQUAL "VISIONOS_SIMULATOR")
  # visionOS Simulator is arm64 only (no Intel Macs support visionOS)
  set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "visionOS Simulator architectures" FORCE)
elseif(IPLUG2_IOS_PLATFORM MATCHES "VISIONOS")
  set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "visionOS architectures" FORCE)
else()
  # iOS device is arm64 only (since iOS 11)
  set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "iOS device architectures" FORCE)
endif()

# Set target device family and ibtool flags based on platform
# Device families: 1=iPhone, 2=iPad, 7=visionOS
if(IPLUG2_IOS_PLATFORM MATCHES "VISIONOS")
  set(IPLUG2_TARGETED_DEVICE_FAMILY "7" CACHE STRING "visionOS device family")
  set(IPLUG2_IBTOOL_TARGET_DEVICES "--target-device" "apple-vision")
  set(IPLUG2_DEPLOYMENT_TARGET_DISPLAY ${IPLUG2_XROS_DEPLOYMENT_TARGET})
else()
  set(IPLUG2_TARGETED_DEVICE_FAMILY "1,2" CACHE STRING "iOS device family")
  set(IPLUG2_IBTOOL_TARGET_DEVICES "--target-device" "iphone" "--target-device" "ipad")
  set(IPLUG2_DEPLOYMENT_TARGET_DISPLAY ${IPLUG2_IOS_DEPLOYMENT_TARGET})
endif()

# Xcode-specific settings
if(XCODE)
  # Set SDK root for Xcode
  set(CMAKE_XCODE_ATTRIBUTE_SDKROOT ${CMAKE_OSX_SYSROOT})

  # Set targeted device family
  set(CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY ${IPLUG2_TARGETED_DEVICE_FAMILY})

  # Skip install for iOS builds (not deploying to system locations)
  set(CMAKE_XCODE_ATTRIBUTE_SKIP_INSTALL "YES")

  # Enable bitcode (optional, deprecated in Xcode 14+)
  set(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE "NO")
endif()

message(STATUS "iPlug2 iOS: Platform=${IPLUG2_IOS_PLATFORM}, SDK=${CMAKE_OSX_SYSROOT}, Deployment=${IPLUG2_DEPLOYMENT_TARGET_DISPLAY}")
