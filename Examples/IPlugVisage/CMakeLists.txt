cmake_minimum_required(VERSION 3.24)
project(IPlugVisage VERSION 1.0.0 LANGUAGES C CXX)

if(NOT (WIN32 OR APPLE) OR CMAKE_SYSTEM_NAME MATCHES "^(iOS|visionOS)$")
  message(STATUS "IPlugVisage currently supports macOS and Windows desktop builds. Skipping.")
  return()
endif()

if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "iPlug2 root directory")
endif()

# Visage requires macOS 10.15 or later.
if(APPLE AND (NOT CMAKE_OSX_DEPLOYMENT_TARGET OR CMAKE_OSX_DEPLOYMENT_TARGET VERSION_LESS 10.15))
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")
endif()
include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

include(FetchContent)
set(IPLUG_VISAGE_GIT_TAG "828037000d0893647ab29b66ae9c4a241c90f671" CACHE STRING
  "Visage revision (set to main to follow upstream)")
set(VISAGE_BUILD_EXAMPLES OFF CACHE BOOL "Build Visage standalone examples")
set(VISAGE_BUILD_TESTS OFF CACHE BOOL "Build Visage tests")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
FetchContent_Declare(visage
  GIT_REPOSITORY https://github.com/VitalAudio/visage.git
  GIT_TAG ${IPLUG_VISAGE_GIT_TAG}
)
FetchContent_MakeAvailable(visage)

if(APPLE)
  include(cmake/VisageMacOS.cmake)
endif()

# Embed a font so the editor is self-contained.
add_embedded_resources(IPlugVisageFonts "IPlugVisageFonts.h" "resources::fonts"
  "${IPLUG2_DIR}/Examples/IPlugEffect/resources/fonts/Roboto-Regular.ttf")

set(IPLUG_VISAGE_FORMATS "APP;VST3;CLAP;AU" CACHE STRING "IPlugVisage formats: APP, VST3, CLAP, AU")
set(_visage_supported_formats APP VST3 CLAP AU)
foreach(_format IN LISTS IPLUG_VISAGE_FORMATS)
  if(NOT _format IN_LIST _visage_supported_formats)
    message(FATAL_ERROR "IPlugVisage supports only APP, VST3, CLAP, and AU; got '${_format}'")
  endif()
endforeach()
if(NOT IPLUG_VISAGE_FORMATS)
  message(FATAL_ERROR "IPLUG_VISAGE_FORMATS must contain at least one format")
endif()
iplug_add_plugin(IPlugVisage
  SOURCES IPlugVisage.cpp IPlugVisage.h resources/resource.h
  FORMATS ${IPLUG_VISAGE_FORMATS}
  UI VISAGE
  LINK IPlugVisageFonts
)
