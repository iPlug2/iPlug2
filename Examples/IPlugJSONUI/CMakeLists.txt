cmake_minimum_required(VERSION 3.14)
project(IPlugJSONUI VERSION 1.0.0)

# Discover IPLUG2_DIR if not already set (for independent builds)
if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "iPlug2 root directory")
endif()

# Include iPlug2 CMake configuration
include(${IPLUG2_DIR}/iPlug2.cmake)

find_package(iPlug2 REQUIRED)

# Build all formats
iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    IPlugJSONUI.cpp
    IPlugJSONUI.h
    ${IPLUG2_DIR}/IGraphics/Extras/IGraphicsJSON.cpp
  RESOURCES
    resources/fonts/Roboto-Regular.ttf
    resources/ui.json
)

# Add nlohmann json include path to base interface (inherited by all targets)
target_include_directories(_${PROJECT_NAME}-base INTERFACE ${IPLUG2_DIR}/Dependencies/Extras/nlohmann)

# For hot-reload in debug builds, provide path to source resources
target_compile_definitions(_${PROJECT_NAME}-base INTERFACE
  $<$<CONFIG:Debug>:IPLUG_SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}">
)
