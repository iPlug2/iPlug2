cmake_minimum_required(VERSION 3.14)
project(IPlugConvoEngine VERSION 1.0.0)

if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "iPlug2 root directory")
endif()

include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

set(CONVOENGINE_DEFINES NO_IGRAPHICS)
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  list(APPEND CONVOENGINE_DEFINES WDL_FFT_REALSIZE=4)
else()
  list(APPEND CONVOENGINE_DEFINES WDL_FFT_REALSIZE=8)
endif()

# NO_IGRAPHICS plugin - no UI, no WAM support
iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    IPlugConvoEngine.cpp
    IPlugConvoEngine.h
    ir.h
    resources/resource.h
    ${IPLUG2_DIR}/WDL/convoengine.cpp
    ${IPLUG2_DIR}/WDL/fft.c
    ${IPLUG2_DIR}/WDL/resample.cpp
  UI NONE
  DEFINES ${CONVOENGINE_DEFINES}
  EXCLUDE_FORMATS WAM WASM_UI
)
