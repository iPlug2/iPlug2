cmake_minimum_required(VERSION 3.14)
project(iPlug2 VERSION 2.0.0)

# Set IPLUG2_DIR to this directory
set(IPLUG2_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "iPlug2 root directory")

# Include iPlug2 CMake configuration
include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

# Add examples
add_subdirectory(Examples)

# Add tests
add_subdirectory(Tests)
