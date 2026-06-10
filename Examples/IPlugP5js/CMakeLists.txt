cmake_minimum_required(VERSION 3.14)
project(IPlugP5js VERSION 1.0.0)

if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "iPlug2 root directory")
endif()

include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    IPlugP5js.cpp
    IPlugP5js.h
    resources/resource.h
  WEB_RESOURCES
    resources/web/index.html
    resources/web/p5.min.js
    resources/web/script.js
    resources/web/sketch.js
    resources/web/uniforms.frag
    resources/web/uniforms.vert
  UI WEBVIEW
  EXCLUDE_FORMATS WAM
)
