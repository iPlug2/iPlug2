cmake_minimum_required(VERSION 3.16)
project(IPlugCocoaUI VERSION 1.0.0 LANGUAGES C CXX OBJC OBJCXX)

# Cocoa UI requires Xcode generator for Swift support (and doesn't support Emscripten)
if(NOT CMAKE_GENERATOR MATCHES "Xcode" OR CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  message(STATUS "IPlugCocoaUI requires Xcode generator for Swift support. Skipping.")
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
  IPlugCocoaUI.h
  IPlugCocoaUI.mm
  IPlugCocoaUI-Shared.h
  resources/resource.h
)

# Swift source files
set(SWIFT_FILES
  UI/IPlugCocoaUIViewController.swift
  UI/TypeAliases.swift
)

# CocoaEditorDelegate sources (required for native Cocoa UI integration)
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
  # APP Target (Standalone Application) - with Cocoa UI
  # ============================================================================
  add_executable(${PROJECT_NAME}-app ${SOURCE_FILES} ${SWIFT_FILES})
  iplug_add_target(${PROJECT_NAME}-app PUBLIC
    LINK iPlug2::APP _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}-app APP ${PROJECT_NAME})

  # Swift configuration
  set_target_properties(${PROJECT_NAME}-app PROPERTIES
    XCODE_ATTRIBUTE_SWIFT_VERSION 5.0
    XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${PROJECT_DIR}/IPlugCocoaUI-Bridging-Header.h"
    XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES YES
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.AcmeInc.app.${PROJECT_NAME}"
    XCODE_ATTRIBUTE_APPLICATION_EXTENSION_API_ONLY NO
    XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks"
  )

  # Link MetalKit for bridging header (used in TypeAliases)
  target_link_libraries(${PROJECT_NAME}-app PUBLIC
    "-framework MetalKit"
  )

  # ============================================================================
  # VST3 Target - with Cocoa UI
  # ============================================================================
  add_library(${PROJECT_NAME}-vst3 MODULE ${SOURCE_FILES} ${SWIFT_FILES})
  iplug_add_target(${PROJECT_NAME}-vst3 PUBLIC
    LINK iPlug2::VST3 _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}-vst3 VST3 ${PROJECT_NAME})

  set_target_properties(${PROJECT_NAME}-vst3 PROPERTIES
    XCODE_ATTRIBUTE_SWIFT_VERSION 5.0
    XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${PROJECT_DIR}/IPlugCocoaUI-Bridging-Header.h"
    XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES YES
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.AcmeInc.vst3.${PROJECT_NAME}"
  )

  target_link_libraries(${PROJECT_NAME}-vst3 PUBLIC
    "-framework MetalKit"
  )

  # ============================================================================
  # CLAP Target - with Cocoa UI
  # ============================================================================
  add_library(${PROJECT_NAME}-clap MODULE ${SOURCE_FILES} ${SWIFT_FILES})
  iplug_add_target(${PROJECT_NAME}-clap PUBLIC
    LINK iPlug2::CLAP _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}-clap CLAP ${PROJECT_NAME})

  set_target_properties(${PROJECT_NAME}-clap PROPERTIES
    XCODE_ATTRIBUTE_SWIFT_VERSION 5.0
    XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${PROJECT_DIR}/IPlugCocoaUI-Bridging-Header.h"
    XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES YES
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.AcmeInc.clap.${PROJECT_NAME}"
  )

  target_link_libraries(${PROJECT_NAME}-clap PUBLIC
    "-framework MetalKit"
  )

  # ============================================================================
  # AUv2 Target (macOS only) - with Cocoa UI
  # ============================================================================
  add_library(${PROJECT_NAME}-au MODULE ${SOURCE_FILES} ${SWIFT_FILES})
  iplug_add_target(${PROJECT_NAME}-au PUBLIC
    LINK iPlug2::AUv2 _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}-au AUv2 ${PROJECT_NAME})

  set_target_properties(${PROJECT_NAME}-au PROPERTIES
    XCODE_ATTRIBUTE_SWIFT_VERSION 5.0
    XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${PROJECT_DIR}/IPlugCocoaUI-Bridging-Header.h"
    XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES YES
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.AcmeInc.audiounit.${PROJECT_NAME}"
  )

  target_link_libraries(${PROJECT_NAME}-au PUBLIC
    "-framework MetalKit"
  )

  # ============================================================================
  # AAX Target (macOS only) - with Cocoa UI
  # ============================================================================
  add_library(${PROJECT_NAME}-aax MODULE ${SOURCE_FILES} ${SWIFT_FILES})
  iplug_add_target(${PROJECT_NAME}-aax PUBLIC
    LINK iPlug2::AAX _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}-aax AAX ${PROJECT_NAME})

  set_target_properties(${PROJECT_NAME}-aax PROPERTIES
    XCODE_ATTRIBUTE_SWIFT_VERSION 5.0
    XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${PROJECT_DIR}/IPlugCocoaUI-Bridging-Header.h"
    XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES YES
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.AcmeInc.aax.${PROJECT_NAME}"
  )

  target_link_libraries(${PROJECT_NAME}-aax PUBLIC
    "-framework MetalKit"
  )

  # ============================================================================
  # AUv3 Targets (macOS) - Framework + Appex embedded in APP
  # ============================================================================
  add_library(${PROJECT_NAME}AU-framework SHARED ${SOURCE_FILES} ${SWIFT_FILES})
  iplug_add_target(${PROJECT_NAME}AU-framework PUBLIC
    LINK iPlug2::AUv3 _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}AU-framework AUv3Framework ${PROJECT_NAME})

  set_target_properties(${PROJECT_NAME}AU-framework PROPERTIES
    XCODE_ATTRIBUTE_SWIFT_VERSION 5.0
    XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${PROJECT_DIR}/IPlugCocoaUI-Bridging-Header.h"
    XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES YES
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.AcmeInc.app.${PROJECT_NAME}.AUv3Framework"
    XCODE_ATTRIBUTE_APPLICATION_EXTENSION_API_ONLY YES
  )

  target_link_libraries(${PROJECT_NAME}AU-framework PUBLIC
    "-framework MetalKit"
  )

  iplug_get_auv3appex_source(${PROJECT_NAME} APPEX_SOURCE)
  add_executable(${PROJECT_NAME}AUv3-appex ${APPEX_SOURCE})
  iplug_configure_target(${PROJECT_NAME}AUv3-appex AUv3Appex ${PROJECT_NAME})

  iplug_embed_auv3_in_app(${PROJECT_NAME}-app ${PROJECT_NAME})
endif()

# ============================================================================
# iOS Targets - AUv3 Framework + Appex + Standalone App (Cocoa UI)
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
    XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${PROJECT_DIR}/IPlugCocoaUI-Bridging-Header.h"
    XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES YES
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.AcmeInc.app.${PROJECT_NAME}.AUv3Framework"
    XCODE_ATTRIBUTE_APPLICATION_EXTENSION_API_ONLY YES
  )

  target_link_libraries(${PROJECT_NAME}AU-ios-framework PUBLIC
    "-framework MetalKit"
  )

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
    XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${PROJECT_DIR}/IPlugCocoaUI-Bridging-Header.h"
    XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES YES
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.AcmeInc.app.${PROJECT_NAME}"
    XCODE_ATTRIBUTE_APPLICATION_EXTENSION_API_ONLY NO
    XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks"
  )

  target_link_libraries(${PROJECT_NAME}-ios-app PUBLIC
    "-framework MetalKit"
  )

  # Embed AUv3 appex and framework in the iOS app
  iplug_embed_auv3ios_in_app(${PROJECT_NAME}-ios-app ${PROJECT_NAME})
endif()
