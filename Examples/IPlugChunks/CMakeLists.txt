cmake_minimum_required(VERSION 3.14)
project(IPlugChunks VERSION 1.0.0)

if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "iPlug2 root directory")
endif()

include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    IPlugChunks.cpp
    IPlugChunks.h
    resources/resource.h
  RESOURCES
    resources/fonts/Roboto-Regular.ttf
)
