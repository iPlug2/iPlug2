cmake_minimum_required(VERSION 3.14)
project(IPlugCLITest VERSION 1.0.0)

# Discover IPLUG2_DIR if not already set (for independent builds)
if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "iPlug2 root directory")
endif()

# Include iPlug2 CMake configuration
include(${IPLUG2_DIR}/iPlug2.cmake)

find_package(iPlug2 REQUIRED)

# Build only CLI format for this test example
iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    IPlugCLITest.cpp
    IPlugCLITest.h
  FORMATS CLI
  UI IGRAPHICS
)
