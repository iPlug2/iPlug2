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

# Add nlohmann json include path to all targets
foreach(target ${PROJECT_NAME}-app ${PROJECT_NAME}-vst3 ${PROJECT_NAME}-clap ${PROJECT_NAME}-au ${PROJECT_NAME}-aax)
  if(TARGET ${target})
    target_include_directories(${target} PRIVATE ${IPLUG2_DIR}/Dependencies/Extras/nlohmann)
  endif()
endforeach()
