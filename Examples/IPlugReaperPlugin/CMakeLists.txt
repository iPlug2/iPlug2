cmake_minimum_required(VERSION 3.14)
project(IPlugReaperPlugin VERSION 1.0.0)

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
  IPlugReaperPlugin.cpp
  IPlugReaperPlugin.h
  resources/resource.h
)

# REAPER SDK for REAPER-specific API access
set(REAPER_SDK_DIR ${IPLUG_DEPS_DIR}/REAPER_SDK)

# Base library for shared code (includes REAPER SDK path for conditional REAPER API usage)
add_library(_${PROJECT_NAME}-base INTERFACE)
iplug_add_target(_${PROJECT_NAME}-base INTERFACE
  INCLUDE ${PROJECT_DIR} ${PROJECT_DIR}/resources ${REAPER_SDK_DIR} ${IPLUG2_DIR}/WDL/lice
  LINK iPlug2::IPlug
)

# Font resources for bundles
set(FONT_FILES ${PLUG_RESOURCES_DIR}/fonts/Roboto-Regular.ttf)

# Helper function to add resources to plugin bundles
function(iplug_add_plugin_resources target)
  target_sources(${target} PRIVATE ${FONT_FILES})
  set_source_files_properties(${FONT_FILES} PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources
  )
endfunction()

# ============================================================================
# macOS/Windows targets (not iOS, not Emscripten)
# ============================================================================
if(NOT IOS AND NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  # APP Target (Standalone Application)
  set(APP_SOURCES ${SOURCE_FILES})
  if(WIN32)
    list(APPEND APP_SOURCES ${PLUG_RESOURCES_DIR}/main.rc)
    set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} /I\"${PLUG_RESOURCES_DIR}/fonts\" /I\"${PLUG_RESOURCES_DIR}/img\"")
  endif()

  add_executable(${PROJECT_NAME}-app ${APP_SOURCES})
  iplug_add_target(${PROJECT_NAME}-app PUBLIC
    LINK iPlug2::APP ${IGRAPHICS_LIB} _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}-app APP ${PROJECT_NAME})
  iplug_add_plugin_resources(${PROJECT_NAME}-app)

  # VST3 Target
  add_library(${PROJECT_NAME}-vst3 MODULE ${SOURCE_FILES})
  iplug_add_target(${PROJECT_NAME}-vst3 PUBLIC
    LINK iPlug2::VST3 ${IGRAPHICS_LIB} _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}-vst3 VST3 ${PROJECT_NAME})
  iplug_add_plugin_resources(${PROJECT_NAME}-vst3)

  # CLAP Target
  if(IPLUG2_CLAP_SUPPORTED)
    add_library(${PROJECT_NAME}-clap MODULE ${SOURCE_FILES})
    iplug_add_target(${PROJECT_NAME}-clap PUBLIC
      LINK iPlug2::CLAP ${IGRAPHICS_LIB} _${PROJECT_NAME}-base
    )
    iplug_configure_target(${PROJECT_NAME}-clap CLAP ${PROJECT_NAME})
    iplug_add_plugin_resources(${PROJECT_NAME}-clap)
  endif()

  # AAX Target
  if(IPLUG2_AAX_SUPPORTED)
    add_library(${PROJECT_NAME}-aax MODULE ${SOURCE_FILES})
    iplug_add_target(${PROJECT_NAME}-aax PUBLIC
      LINK iPlug2::AAX ${IGRAPHICS_LIB} _${PROJECT_NAME}-base
    )
    iplug_configure_target(${PROJECT_NAME}-aax AAX ${PROJECT_NAME})
    iplug_add_plugin_resources(${PROJECT_NAME}-aax)
  endif()
endif()

# ============================================================================
# AUv2 Target (macOS only)
# ============================================================================
if(APPLE AND NOT IOS)
  add_library(${PROJECT_NAME}-au MODULE ${SOURCE_FILES})
  iplug_add_target(${PROJECT_NAME}-au PUBLIC
    LINK iPlug2::AUv2 ${IGRAPHICS_LIB} _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}-au AUv2 ${PROJECT_NAME})
  iplug_add_plugin_resources(${PROJECT_NAME}-au)
endif()

# ============================================================================
# AUv3 Targets (macOS) - Framework + Appex embedded in APP
# ============================================================================
if(APPLE AND NOT IOS)
  add_library(${PROJECT_NAME}AU-framework SHARED ${SOURCE_FILES})
  iplug_add_target(${PROJECT_NAME}AU-framework PUBLIC
    LINK iPlug2::AUv3 ${IGRAPHICS_LIB} _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}AU-framework AUv3Framework ${PROJECT_NAME})

  iplug_get_auv3appex_source(${PROJECT_NAME} APPEX_SOURCE)
  add_executable(${PROJECT_NAME}AUv3-appex ${APPEX_SOURCE})
  iplug_configure_target(${PROJECT_NAME}AUv3-appex AUv3Appex ${PROJECT_NAME})

  iplug_embed_auv3_in_app(${PROJECT_NAME}-app ${PROJECT_NAME})
endif()

# ============================================================================
# iOS Targets - AUv3 Framework + Appex + Standalone App
# ============================================================================
if(IOS)
  add_library(${PROJECT_NAME}AU-ios-framework SHARED ${SOURCE_FILES})
  iplug_add_target(${PROJECT_NAME}AU-ios-framework PUBLIC
    LINK iPlug2::AUv3iOS ${IGRAPHICS_LIB} _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}AU-ios-framework AUv3iOSFramework ${PROJECT_NAME})

  iplug_get_auv3ios_appex_source(${PROJECT_NAME} IOS_APPEX_SOURCE)
  add_executable(${PROJECT_NAME}AUv3-ios-appex ${IOS_APPEX_SOURCE})
  iplug_configure_target(${PROJECT_NAME}AUv3-ios-appex AUv3iOSAppex ${PROJECT_NAME})

  add_executable(${PROJECT_NAME}-ios-app ${SOURCE_FILES})
  iplug_add_target(${PROJECT_NAME}-ios-app PUBLIC
    LINK iPlug2::AUv3iOS ${IGRAPHICS_LIB} _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}-ios-app IOSApp ${PROJECT_NAME})
  iplug_add_plugin_resources(${PROJECT_NAME}-ios-app)

  iplug_embed_auv3ios_in_app(${PROJECT_NAME}-ios-app ${PROJECT_NAME})
endif()

# ============================================================================
# WAM/Web Targets (Emscripten only)
# ============================================================================
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  add_executable(${PROJECT_NAME}-wam ${SOURCE_FILES})
  iplug_add_target(${PROJECT_NAME}-wam PUBLIC
    LINK iPlug2::WAM _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}-wam WAM ${PROJECT_NAME})

  add_executable(${PROJECT_NAME}-web ${SOURCE_FILES})
  iplug_add_target(${PROJECT_NAME}-web PUBLIC
    LINK iPlug2::Web _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}-web Web ${PROJECT_NAME})

  iplug_build_wam_dist(${PROJECT_NAME}
    WAM_TARGET ${PROJECT_NAME}-wam
    WEB_TARGET ${PROJECT_NAME}-web
    SITE_ORIGIN "/"
  )
endif()
