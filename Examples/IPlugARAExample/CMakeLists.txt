cmake_minimum_required(VERSION 3.19)
project(IPlugARAExample VERSION 1.0.0)

# Discover IPLUG2_DIR if not already set (for independent builds)
if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "iPlug2 root directory")
endif()

# Include iPlug2 CMake configuration
include(${IPLUG2_DIR}/iPlug2.cmake)

find_package(iPlug2 REQUIRED)

# ARA is an extension of VST3, so the VST3_ARA format produces a single .vst3 bundle
# that also exposes the ARA plug-in extension. The ARA SDK (ARA_API + ARA_Library) is
# fetched automatically via FetchContent if not found in Dependencies/IPlug/ARA_SDK.
iplug_add_plugin(${PROJECT_NAME}
  FORMATS VST3_ARA
  SOURCES
    IPlugARAExample.cpp
    IPlugARAExample.h
    IPlugARAExample_ARA.h
    IPlugARAExample_Controls.h
    config.h
  RESOURCES
    resources/fonts/Roboto-Regular.ttf
)

# On Windows loose resource files cannot be loaded from a VST3 bundle,
# so compile the UI font into the module as a Windows resource
if(WIN32 AND TARGET ${PROJECT_NAME}-vst3-ara)
  set(_ara_rc "${CMAKE_CURRENT_SOURCE_DIR}/resources/main.rc")
  target_sources(${PROJECT_NAME}-vst3-ara PRIVATE "${_ara_rc}")
  set_source_files_properties("${_ara_rc}" PROPERTIES
    COMPILE_FLAGS "/I\"${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts\" /I\"${CMAKE_CURRENT_SOURCE_DIR}/resources\""
  )
endif()
