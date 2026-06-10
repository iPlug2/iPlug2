cmake_minimum_required(VERSION 3.14)
project(IPlugReaperExtension VERSION 1.0.0)

# REAPER Extensions are macOS and Windows only (no iOS, no Emscripten)
if(IOS OR CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  message(STATUS "IPlugReaperExtension is not supported on ${CMAKE_SYSTEM_NAME}. Skipping.")
  return()
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

# Standard plugin setup
set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PLUG_RESOURCES_DIR ${PROJECT_DIR}/resources)

set(SOURCE_FILES
  IPlugReaperExtension.cpp
  IPlugReaperExtension.h
  resources/resource.h
  resources/roboto.cpp
  resources/roboto.hpp
)

# Add Windows resources
if(WIN32)
  list(APPEND SOURCE_FILES ${PLUG_RESOURCES_DIR}/main.rc)
endif()

# Build as a shared library (MODULE for dynamic loading)
add_library(${PROJECT_NAME} MODULE ${SOURCE_FILES})

# Base configuration
iplug_add_target(${PROJECT_NAME} PUBLIC
  INCLUDE ${PROJECT_DIR} ${PROJECT_DIR}/resources
  LINK ${IGRAPHICS_LIB}
)

# Configure as REAPER extension (sets up output, deployment, etc.)
iplug_configure_reaperext(${PROJECT_NAME} ${PROJECT_NAME})
