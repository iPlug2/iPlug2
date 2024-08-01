cmake_minimum_required(VERSION 3.14)
project(IPlugConvoEngine VERSION 1.0.0)

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

set(SOURCE_FILES
  IPlugConvoEngine.cpp
  IPlugConvoEngine.h
  ir.h
  resources/resource.h
  ${IPLUG2_DIR}/WDL/convoengine.cpp
  ${IPLUG2_DIR}/WDL/fft.c
)

# Base library for shared code
add_library(_${PROJECT_NAME}-base INTERFACE)
iplug_add_target(_${PROJECT_NAME}-base INTERFACE
  INCLUDE ${PROJECT_DIR} ${PROJECT_DIR}/resources
  LINK iPlug2::IPlug
  DEFINE WDL_FFT_REALSIZE=8
)

# ============================================================================
# macOS/Windows targets (not iOS, not Emscripten)
# Note: This is a NO_IGRAPHICS plugin, so WAM/Web targets are not supported
# ============================================================================
if(NOT IOS AND NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  # ============================================================================
  # APP Target (Standalone Application) - NO UI
  # ============================================================================
  set(APP_SOURCES ${SOURCE_FILES})

  # Add Windows resource file for dialog/menu resources
  if(WIN32)
    list(APPEND APP_SOURCES ${PLUG_RESOURCES_DIR}/main.rc)
  endif()

  add_executable(${PROJECT_NAME}-app ${APP_SOURCES})
  iplug_add_target(${PROJECT_NAME}-app PUBLIC
    LINK iPlug2::APP _${PROJECT_NAME}-base
    DEFINE NO_IGRAPHICS
  )
  iplug_configure_target(${PROJECT_NAME}-app APP ${PROJECT_NAME})

  # ============================================================================
  # VST3 Target - NO UI
  # ============================================================================
  add_library(${PROJECT_NAME}-vst3 MODULE ${SOURCE_FILES})
  iplug_add_target(${PROJECT_NAME}-vst3 PUBLIC
    LINK iPlug2::VST3 _${PROJECT_NAME}-base
    DEFINE NO_IGRAPHICS
  )
  iplug_configure_target(${PROJECT_NAME}-vst3 VST3 ${PROJECT_NAME})

  # ============================================================================
  # CLAP Target - NO UI
  # ============================================================================
  add_library(${PROJECT_NAME}-clap MODULE ${SOURCE_FILES})
  iplug_add_target(${PROJECT_NAME}-clap PUBLIC
    LINK iPlug2::CLAP _${PROJECT_NAME}-base
    DEFINE NO_IGRAPHICS
  )
  iplug_configure_target(${PROJECT_NAME}-clap CLAP ${PROJECT_NAME})
endif()

# ============================================================================
# AUv2 Target (macOS only) - NO UI
# ============================================================================
if(APPLE AND NOT IOS)
  add_library(${PROJECT_NAME}-au MODULE ${SOURCE_FILES})
  iplug_add_target(${PROJECT_NAME}-au PUBLIC
    LINK iPlug2::AUv2 _${PROJECT_NAME}-base
    DEFINE NO_IGRAPHICS
  )
  iplug_configure_target(${PROJECT_NAME}-au AUv2 ${PROJECT_NAME})
endif()

# ============================================================================
# iOS Targets - AUv3 Framework + Appex + Standalone App (NO UI)
# ============================================================================
if(IOS)
  # iOS AUv3 Framework containing plugin code
  add_library(${PROJECT_NAME}AU-ios-framework SHARED ${SOURCE_FILES})
  iplug_add_target(${PROJECT_NAME}AU-ios-framework PUBLIC
    LINK iPlug2::AUv3iOS _${PROJECT_NAME}-base
    DEFINE NO_IGRAPHICS
  )
  iplug_configure_target(${PROJECT_NAME}AU-ios-framework AUv3iOSFramework ${PROJECT_NAME})

  # iOS AUv3 App Extension (appex)
  iplug_get_auv3ios_appex_source(${PROJECT_NAME} IOS_APPEX_SOURCE)
  add_executable(${PROJECT_NAME}AUv3-ios-appex ${IOS_APPEX_SOURCE})
  iplug_configure_target(${PROJECT_NAME}AUv3-ios-appex AUv3iOSAppex ${PROJECT_NAME})

  # iOS Standalone App that hosts the AUv3 for testing
  add_executable(${PROJECT_NAME}-ios-app ${SOURCE_FILES})
  iplug_add_target(${PROJECT_NAME}-ios-app PUBLIC
    LINK iPlug2::AUv3iOS _${PROJECT_NAME}-base
    DEFINE NO_IGRAPHICS
  )
  iplug_configure_target(${PROJECT_NAME}-ios-app IOSApp ${PROJECT_NAME})

  # Embed AUv3 appex and framework in the iOS app
  iplug_embed_auv3ios_in_app(${PROJECT_NAME}-ios-app ${PROJECT_NAME})
endif()
