cmake_minimum_required(VERSION 3.14)
project(iPlug2 VERSION 2.0.0)

# Set IPLUG2_DIR to this directory
set(IPLUG2_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "iPlug2 root directory")

# Include iPlug2 CMake configuration
include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

# Options to control building examples and tests
# Default to ON only when building iPlug2 as the top-level project
# (OFF when used via FetchContent or add_subdirectory)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  option(IPLUG2_BUILD_EXAMPLES "Build iPlug2 examples" ON)
  option(IPLUG2_BUILD_TESTS "Build iPlug2 tests" ON)
else()
  option(IPLUG2_BUILD_EXAMPLES "Build iPlug2 examples" OFF)
  option(IPLUG2_BUILD_TESTS "Build iPlug2 tests" OFF)
endif()

if(IPLUG2_BUILD_EXAMPLES)
  add_subdirectory(Examples)
endif()

if(IPLUG2_BUILD_TESTS)
  add_subdirectory(Tests)
endif()
