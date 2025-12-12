cmake_minimum_required(VERSION 3.14)
project(IPlugWebUI VERSION 1.0.0)

# Discover IPLUG2_DIR if not already set (for independent builds)
if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "iPlug2 root directory")
endif()

# Include iPlug2 CMake configuration
include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

# Include WebView module for WebViewEditorDelegate
include(${IPLUG2_CMAKE_DIR}/WebView.cmake)

# Standard plugin setup
set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PLUG_RESOURCES_DIR ${PROJECT_DIR}/resources)

set(SOURCE_FILES
  IPlugWebUI.cpp
  IPlugWebUI.h
  resources/resource.h
)

# Base library for shared code
add_library(_${PROJECT_NAME}-base INTERFACE)
iplug_add_target(_${PROJECT_NAME}-base INTERFACE
  INCLUDE ${PROJECT_DIR} ${PROJECT_DIR}/resources
  LINK iPlug2::IPlug
)

# Helper function to add web resources to plugin bundles
function(iplug_add_web_resources target)
  file(GLOB WEB_RESOURCES
    "${PLUG_RESOURCES_DIR}/web/*.html"
    "${PLUG_RESOURCES_DIR}/web/*.js"
    "${PLUG_RESOURCES_DIR}/web/*.css"
  )
  target_sources(${target} PRIVATE ${WEB_RESOURCES})
  foreach(resource ${WEB_RESOURCES})
    set_source_files_properties(${resource} PROPERTIES
      MACOSX_PACKAGE_LOCATION Resources/web
    )
  endforeach()
endfunction()

# ============================================================================
# macOS/Windows targets (not iOS, not Emscripten)
# Note: IPlugWebUI uses WebView UI, not IGraphics - WAM/Web targets are not supported
# ============================================================================
if(NOT IOS AND NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  # ============================================================================
  # APP Target (Standalone Application) - with WebView UI
  # ============================================================================
  set(APP_SOURCES ${SOURCE_FILES})

  # Add Windows resource file for dialog/menu resources
  if(WIN32)
    list(APPEND APP_SOURCES ${PLUG_RESOURCES_DIR}/main.rc)
  endif()

  add_executable(${PROJECT_NAME}-app ${APP_SOURCES})
  iplug_add_target(${PROJECT_NAME}-app PUBLIC
    LINK iPlug2::APP iPlug2::WebView _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}-app APP ${PROJECT_NAME})
  iplug_add_web_resources(${PROJECT_NAME}-app)

  # ============================================================================
  # VST3 Target - with WebView UI
  # ============================================================================
  add_library(${PROJECT_NAME}-vst3 MODULE ${SOURCE_FILES})
  iplug_add_target(${PROJECT_NAME}-vst3 PUBLIC
    LINK iPlug2::VST3 iPlug2::WebView _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}-vst3 VST3 ${PROJECT_NAME})
  iplug_add_web_resources(${PROJECT_NAME}-vst3)

  # ============================================================================
  # CLAP Target - with WebView UI
  # ============================================================================
  add_library(${PROJECT_NAME}-clap MODULE ${SOURCE_FILES})
  iplug_add_target(${PROJECT_NAME}-clap PUBLIC
    LINK iPlug2::CLAP iPlug2::WebView _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}-clap CLAP ${PROJECT_NAME})
  iplug_add_web_resources(${PROJECT_NAME}-clap)
endif()

# ============================================================================
# AUv2 Target (macOS only) - with WebView UI
# ============================================================================
if(APPLE AND NOT IOS)
  add_library(${PROJECT_NAME}-au MODULE ${SOURCE_FILES})
  iplug_add_target(${PROJECT_NAME}-au PUBLIC
    LINK iPlug2::AUv2 iPlug2::WebView _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}-au AUv2 ${PROJECT_NAME})
  iplug_add_web_resources(${PROJECT_NAME}-au)
endif()

# ============================================================================
# AUv3 Targets (macOS) - Framework + Appex embedded in APP
# ============================================================================
if(APPLE AND NOT IOS)
  # Framework containing AUv3 plugin code
  add_library(${PROJECT_NAME}AU-framework SHARED ${SOURCE_FILES})
  iplug_add_target(${PROJECT_NAME}AU-framework PUBLIC
    LINK iPlug2::AUv3 iPlug2::WebView _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}AU-framework AUv3Framework ${PROJECT_NAME})
  iplug_add_web_resources(${PROJECT_NAME}AU-framework)

  # App Extension (appex) - executable using NSExtensionMain entry point
  iplug_get_auv3appex_source(${PROJECT_NAME} APPEX_SOURCE)
  add_executable(${PROJECT_NAME}AUv3-appex ${APPEX_SOURCE})
  iplug_configure_target(${PROJECT_NAME}AUv3-appex AUv3Appex ${PROJECT_NAME})

  # Embed AUv3 appex in the existing APP target
  iplug_embed_auv3_in_app(${PROJECT_NAME}-app ${PROJECT_NAME})
endif()

# ============================================================================
# iOS Targets - AUv3 Framework + Appex + Standalone App (WebView UI)
# ============================================================================
if(IOS)
  # iOS AUv3 Framework containing plugin code
  add_library(${PROJECT_NAME}AU-ios-framework SHARED ${SOURCE_FILES})
  iplug_add_target(${PROJECT_NAME}AU-ios-framework PUBLIC
    LINK iPlug2::AUv3iOS iPlug2::WebView _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}AU-ios-framework AUv3iOSFramework ${PROJECT_NAME})
  iplug_add_web_resources(${PROJECT_NAME}AU-ios-framework)

  # iOS AUv3 App Extension (appex)
  iplug_get_auv3ios_appex_source(${PROJECT_NAME} IOS_APPEX_SOURCE)
  add_executable(${PROJECT_NAME}AUv3-ios-appex ${IOS_APPEX_SOURCE})
  iplug_configure_target(${PROJECT_NAME}AUv3-ios-appex AUv3iOSAppex ${PROJECT_NAME})

  # iOS Standalone App that hosts the AUv3 for testing
  add_executable(${PROJECT_NAME}-ios-app ${SOURCE_FILES})
  iplug_add_target(${PROJECT_NAME}-ios-app PUBLIC
    LINK iPlug2::AUv3iOS iPlug2::WebView _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}-ios-app IOSApp ${PROJECT_NAME})
  iplug_add_web_resources(${PROJECT_NAME}-ios-app)

  # Embed AUv3 appex and framework in the iOS app
  iplug_embed_auv3ios_in_app(${PROJECT_NAME}-ios-app ${PROJECT_NAME})
endif()
