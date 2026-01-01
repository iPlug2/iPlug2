# IPlugMinimal - Minimal iPlug2 example with CMake-generated resources
#
# This example demonstrates how to create an iPlug2 plugin project with
# no pre-made resource files. All plist, main.rc, XIB, and resource.h
# files are generated at CMake configure time from config.h.
#
# Project structure:
#   IPlugMinimal/
#     CMakeLists.txt    - This file
#     config.h          - Plugin configuration (all metadata)
#     IPlugMinimal.h    - Plugin header
#     IPlugMinimal.cpp  - Plugin implementation
#     (no resources/ folder needed!)

cmake_minimum_required(VERSION 3.16)
project(IPlugMinimal VERSION 1.0.0)

# Discover IPLUG2_DIR if not already set (for independent builds)
if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "iPlug2 root directory")
endif()

# Include iPlug2 CMake configuration (enables OBJC/OBJCXX languages, sets C++ standard)
include(${IPLUG2_DIR}/iPlug2.cmake)

find_package(iPlug2 REQUIRED)

# Generate all resource files from config.h
# This creates plist files, main.rc, resource.h, and XIB in the build directory
iplug_generate_resources(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR})

# Create the plugin with APP, VST3, and AU formats
# No RESOURCES argument needed - everything is generated!
iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    IPlugMinimal.cpp
    IPlugMinimal.h
  FORMATS APP VST3 AU
)
