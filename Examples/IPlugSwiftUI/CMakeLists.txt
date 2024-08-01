cmake_minimum_required(VERSION 3.16)
project(IPlugSwiftUI VERSION 1.0.0 LANGUAGES C CXX OBJC OBJCXX)

# SwiftUI requires Xcode generator (and doesn't support Emscripten)
if(NOT CMAKE_GENERATOR MATCHES "Xcode" OR CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  message(STATUS "IPlugSwiftUI requires Xcode generator for Swift/SwiftUI support. Skipping.")
  return()
endif()

# Enable Swift language
enable_language(Swift)

# Discover IPLUG2_DIR if not already set (for independent builds)
if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "iPlug2 root directory")
endif()

# Include iPlug2 CMake configuration
include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

# Standard plugin setup
set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PLUG_RESOURCES_DIR ${PROJECT_DIR}/resources)

# C++/Objective-C++ source files
set(SOURCE_FILES
  IPlugSwiftUI.h
  IPlugSwiftUI.mm
  IPlugSwiftUI-Shared.h
  resources/resource.h
)

# Swift source files
set(SWIFT_FILES
  UI/BundleLocator.swift
  UI/ContentView.swift
  UI/GridView.swift
  UI/IPlugSwiftUIState.swift
  UI/IPlugSwiftUIViewController.swift
  UI/OscilloscopeView.swift
  UI/Param.swift
  UI/ParamSliderView.swift
  UI/TypeAliases.swift
)

# Metal shader files
set(METAL_SOURCES
  ${PROJECT_DIR}/UI/CRTEffect.metal
  ${PROJECT_DIR}/UI/Oscilloscope.metal
)

# Determine Metal SDK and language standard based on platform
if(IOS)
  if(CMAKE_OSX_SYSROOT MATCHES "iphonesimulator")
    set(METAL_SDK "iphonesimulator")
  else()
    set(METAL_SDK "iphoneos")
  endif()
  # iOS requires ios-metal2.4 for [[stitchable]] attribute
  set(METAL_STD "-std=ios-metal2.4")
else()
  set(METAL_SDK "macosx")
  set(METAL_STD "")
endif()

# Create .air (compiled) files from .metal sources
set(METAL_AIR_FILES)
foreach(metal_source ${METAL_SOURCES})
  get_filename_component(metal_name ${metal_source} NAME_WE)
  set(air_file ${CMAKE_CURRENT_BINARY_DIR}/${metal_name}.air)
  list(APPEND METAL_AIR_FILES ${air_file})
  add_custom_command(
    OUTPUT ${air_file}
    COMMAND xcrun -sdk ${METAL_SDK} metal ${METAL_STD} -c ${metal_source} -o ${air_file}
    DEPENDS ${metal_source}
    COMMENT "Compiling Metal shader ${metal_name} for ${METAL_SDK}"
  )
endforeach()

# Link .air files into default.metallib
set(METALLIB_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/default.metallib)
add_custom_command(
  OUTPUT ${METALLIB_OUTPUT}
  COMMAND xcrun -sdk ${METAL_SDK} metallib ${METAL_AIR_FILES} -o ${METALLIB_OUTPUT}
  DEPENDS ${METAL_AIR_FILES}
  COMMENT "Linking Metal shaders into default.metallib"
)

# Create a target to build the metallib
add_custom_target(${PROJECT_NAME}-metallib DEPENDS ${METALLIB_OUTPUT})

# Helper function to add metallib to a macOS bundle target
function(iplug_add_metallib target)
  add_dependencies(${target} ${PROJECT_NAME}-metallib)
  # Copy metallib to bundle Resources folder as a post-build step (macOS bundle structure)
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${METALLIB_OUTPUT}
      "$<TARGET_BUNDLE_DIR:${target}>/Contents/Resources/default.metallib"
    COMMENT "Copying default.metallib to ${target} bundle"
  )
endfunction()

# Helper function to add metallib to an iOS bundle target (flat bundle structure)
function(iplug_add_metallib_ios target)
  add_dependencies(${target} ${PROJECT_NAME}-metallib)
  # Copy metallib to bundle root (iOS flat bundle structure)
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${METALLIB_OUTPUT}
      "$<TARGET_BUNDLE_DIR:${target}>/default.metallib"
    COMMENT "Copying default.metallib to ${target} iOS bundle"
  )
endfunction()

# CocoaEditorDelegate sources (required for SwiftUI integration)
set(COCOA_EDITOR_SRC
  ${IPLUG2_DIR}/IPlug/Extras/Cocoa/IPlugCocoaEditorDelegate.mm
  ${IPLUG2_DIR}/IPlug/Extras/Cocoa/IPlugCocoaViewController.mm
)

# Base library for shared code
add_library(_${PROJECT_NAME}-base INTERFACE)
iplug_add_target(_${PROJECT_NAME}-base INTERFACE
  INCLUDE ${PROJECT_DIR} ${PROJECT_DIR}/resources
          ${IPLUG2_DIR}/IPlug/Extras/Cocoa
  SOURCE ${COCOA_EDITOR_SRC}
  LINK iPlug2::IPlug
  DEFINE NO_IGRAPHICS COCOA_EDITOR_DELEGATE
)

# ============================================================================
# macOS targets (not iOS)
# ============================================================================
if(NOT IOS)
  # ============================================================================
  # APP Target (Standalone Application) - with SwiftUI
  # ============================================================================
  add_executable(${PROJECT_NAME}-app ${SOURCE_FILES} ${SWIFT_FILES})
  iplug_add_target(${PROJECT_NAME}-app PUBLIC
    LINK iPlug2::APP _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}-app APP ${PROJECT_NAME})

  # Swift configuration
  set_target_properties(${PROJECT_NAME}-app PROPERTIES
    XCODE_ATTRIBUTE_SWIFT_VERSION 5.0
    XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${PROJECT_DIR}/IPlugSwiftUI-Bridging-Header.h"
    XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES YES
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.AcmeInc.app.${PROJECT_NAME}"
    XCODE_ATTRIBUTE_APPLICATION_EXTENSION_API_ONLY NO
    XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks"
    MACOSX_DEPLOYMENT_TARGET 15.0
  )

  # Link Metal frameworks (SwiftUI linked automatically via Swift imports)
  target_link_libraries(${PROJECT_NAME}-app PUBLIC
    "-framework Metal"
    "-framework MetalKit"
    "-framework Accelerate"
  )

  # Add compiled Metal shaders
  iplug_add_metallib(${PROJECT_NAME}-app)

  # ============================================================================
  # VST3 Target - with SwiftUI
  # ============================================================================
  add_library(${PROJECT_NAME}-vst3 MODULE ${SOURCE_FILES} ${SWIFT_FILES})
  iplug_add_target(${PROJECT_NAME}-vst3 PUBLIC
    LINK iPlug2::VST3 _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}-vst3 VST3 ${PROJECT_NAME})

  set_target_properties(${PROJECT_NAME}-vst3 PROPERTIES
    XCODE_ATTRIBUTE_SWIFT_VERSION 5.0
    XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${PROJECT_DIR}/IPlugSwiftUI-Bridging-Header.h"
    XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES YES
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.AcmeInc.vst3.${PROJECT_NAME}"
    MACOSX_DEPLOYMENT_TARGET 15.0
  )

  target_link_libraries(${PROJECT_NAME}-vst3 PUBLIC
    "-framework Metal"
    "-framework MetalKit"
    "-framework Accelerate"
  )

  # Add compiled Metal shaders
  iplug_add_metallib(${PROJECT_NAME}-vst3)

  # ============================================================================
  # CLAP Target - with SwiftUI
  # ============================================================================
  add_library(${PROJECT_NAME}-clap MODULE ${SOURCE_FILES} ${SWIFT_FILES})
  iplug_add_target(${PROJECT_NAME}-clap PUBLIC
    LINK iPlug2::CLAP _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}-clap CLAP ${PROJECT_NAME})

  set_target_properties(${PROJECT_NAME}-clap PROPERTIES
    XCODE_ATTRIBUTE_SWIFT_VERSION 5.0
    XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${PROJECT_DIR}/IPlugSwiftUI-Bridging-Header.h"
    XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES YES
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.AcmeInc.clap.${PROJECT_NAME}"
    MACOSX_DEPLOYMENT_TARGET 15.0
  )

  target_link_libraries(${PROJECT_NAME}-clap PUBLIC
    "-framework Metal"
    "-framework MetalKit"
    "-framework Accelerate"
  )

  # Add compiled Metal shaders
  iplug_add_metallib(${PROJECT_NAME}-clap)

  # ============================================================================
  # AUv2 Target (macOS only) - with SwiftUI
  # ============================================================================
  add_library(${PROJECT_NAME}-au MODULE ${SOURCE_FILES} ${SWIFT_FILES})
  iplug_add_target(${PROJECT_NAME}-au PUBLIC
    LINK iPlug2::AUv2 _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}-au AUv2 ${PROJECT_NAME})

  set_target_properties(${PROJECT_NAME}-au PROPERTIES
    XCODE_ATTRIBUTE_SWIFT_VERSION 5.0
    XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${PROJECT_DIR}/IPlugSwiftUI-Bridging-Header.h"
    XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES YES
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.AcmeInc.audiounit.${PROJECT_NAME}"
    MACOSX_DEPLOYMENT_TARGET 15.0
  )

  target_link_libraries(${PROJECT_NAME}-au PUBLIC
    "-framework Metal"
    "-framework MetalKit"
    "-framework Accelerate"
  )

  # Add compiled Metal shaders
  iplug_add_metallib(${PROJECT_NAME}-au)
endif()

# ============================================================================
# iOS Targets - AUv3 Framework + Appex + Standalone App (SwiftUI)
# ============================================================================
if(IOS)
  # iOS AUv3 Framework containing plugin code
  add_library(${PROJECT_NAME}AU-ios-framework SHARED ${SOURCE_FILES} ${SWIFT_FILES})
  iplug_add_target(${PROJECT_NAME}AU-ios-framework PUBLIC
    LINK iPlug2::AUv3iOS _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}AU-ios-framework AUv3iOSFramework ${PROJECT_NAME})

  set_target_properties(${PROJECT_NAME}AU-ios-framework PROPERTIES
    XCODE_ATTRIBUTE_SWIFT_VERSION 5.0
    XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${PROJECT_DIR}/IPlugSwiftUI-Bridging-Header.h"
    XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES YES
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.AcmeInc.app.${PROJECT_NAME}.AUv3Framework"
    XCODE_ATTRIBUTE_APPLICATION_EXTENSION_API_ONLY YES
  )

  target_link_libraries(${PROJECT_NAME}AU-ios-framework PUBLIC
    "-framework Metal"
    "-framework MetalKit"
    "-framework Accelerate"
  )

  # Add compiled Metal shaders to framework
  iplug_add_metallib_ios(${PROJECT_NAME}AU-ios-framework)

  # iOS AUv3 App Extension (appex)
  iplug_get_auv3ios_appex_source(${PROJECT_NAME} IOS_APPEX_SOURCE)
  add_executable(${PROJECT_NAME}AUv3-ios-appex ${IOS_APPEX_SOURCE})
  iplug_configure_target(${PROJECT_NAME}AUv3-ios-appex AUv3iOSAppex ${PROJECT_NAME})

  # iOS Standalone App that hosts the AUv3 for testing
  add_executable(${PROJECT_NAME}-ios-app ${SOURCE_FILES} ${SWIFT_FILES})
  iplug_add_target(${PROJECT_NAME}-ios-app PUBLIC
    LINK iPlug2::AUv3iOS _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}-ios-app IOSApp ${PROJECT_NAME})

  set_target_properties(${PROJECT_NAME}-ios-app PROPERTIES
    XCODE_ATTRIBUTE_SWIFT_VERSION 5.0
    XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${PROJECT_DIR}/IPlugSwiftUI-Bridging-Header.h"
    XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES YES
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.AcmeInc.app.${PROJECT_NAME}"
    XCODE_ATTRIBUTE_APPLICATION_EXTENSION_API_ONLY NO
    XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks"
  )

  target_link_libraries(${PROJECT_NAME}-ios-app PUBLIC
    "-framework Metal"
    "-framework MetalKit"
    "-framework Accelerate"
  )

  # Add compiled Metal shaders to app
  iplug_add_metallib_ios(${PROJECT_NAME}-ios-app)

  # Embed AUv3 appex and framework in the iOS app
  iplug_embed_auv3ios_in_app(${PROJECT_NAME}-ios-app ${PROJECT_NAME})
endif()
