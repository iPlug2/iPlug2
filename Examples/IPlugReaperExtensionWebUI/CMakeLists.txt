cmake_minimum_required(VERSION 3.14)
project(IPlugReaperExtensionWebUI VERSION 1.0.0)

# REAPER Extensions are macOS and Windows only (no iOS, no Emscripten)
if(IOS OR CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  message(STATUS "IPlugReaperExtensionWebUI is not supported on ${CMAKE_SYSTEM_NAME}. Skipping.")
  return()
endif()

# The WebView editor delegate uses std::filesystem, which requires macOS 10.15+
# (iPlug2.cmake otherwise defaults to 10.13). Set before including iPlug2.cmake.
if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")
endif()

# Discover IPLUG2_DIR if not already set (for independent builds)
if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "iPlug2 root directory")
endif()

# Include iPlug2 CMake configuration
include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

# Include REAPER Extension module
include(${IPLUG2_CMAKE_DIR}/ReaperExt.cmake)

# Include WebView module (WebViewEditorDelegate instead of IGraphics)
include(${IPLUG2_CMAKE_DIR}/WebView.cmake)

# Standard plugin setup
set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PLUG_RESOURCES_DIR ${PROJECT_DIR}/resources)

set(SOURCE_FILES
  IPlugReaperExtensionWebUI.cpp
  IPlugReaperExtensionWebUI.h
  resources/resource.h
  resources/web/index.html
  resources/web/script.js
)

# Add Windows resources
if(WIN32)
  list(APPEND SOURCE_FILES ${PLUG_RESOURCES_DIR}/main.rc)
endif()

# Build as a shared library (MODULE for dynamic loading)
add_library(${PROJECT_NAME} MODULE ${SOURCE_FILES})

# Base configuration - use the WebView UI instead of IGraphics
iplug_add_target(${PROJECT_NAME} PUBLIC
  INCLUDE ${PROJECT_DIR} ${PROJECT_DIR}/resources
  LINK iPlug2::WebView
)

# Configure as REAPER extension (sets up output, deployment, etc.)
iplug_configure_reaperext(${PROJECT_NAME} ${PROJECT_NAME})

# Deploy the WebView UI next to the extension so Release builds can load index.html at runtime.
# (In Debug the extension loads directly from the source tree via __FILE__.) The destination
# subfolder must match SHARED_RESOURCES_SUBPATH in config.h.
if(IPLUG_DEPLOY_PLUGINS)
  iplug_get_default_deploy_path(REAPEREXT)
  if(IPLUG_DEPLOY_PATH_REAPEREXT)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_DIR}/resources/web"
        "${IPLUG_DEPLOY_PATH_REAPEREXT}/IPlugReaperExtensionWebUI"
      COMMENT "Deploying WebView UI to ${IPLUG_DEPLOY_PATH_REAPEREXT}/IPlugReaperExtensionWebUI"
      VERBATIM
    )
  endif()
endif()
