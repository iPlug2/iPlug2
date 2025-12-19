cmake_minimum_required(VERSION 3.14)
project(IPlugSvelteUI VERSION 1.0.0)

if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "iPlug2 root directory")
endif()

include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

# Glob web resources (Svelte build outputs have hashed filenames)
file(GLOB WEB_RESOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/resources/web/*.html"
  "${CMAKE_CURRENT_SOURCE_DIR}/resources/web/assets/*"
)

iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    IPlugSvelteUI.cpp
    IPlugSvelteUI.h
    resources/resource.h
  WEB_RESOURCES ${WEB_RESOURCES}
  UI WEBVIEW
  EXCLUDE_FORMATS WAM
)
