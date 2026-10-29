#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# ARA SDK configuration for iPlug2
#
# ARA is an extension of the VST3 plug-in format, so iPlug2::ARA is linked alongside
# iPlug2::VST3 (see VST3ARA.cmake / the VST3_ARA format in iplug_add_plugin()).
#
# The ARA SDK is located as follows:
# 1. ARA_SDK_DIR cache variable / Dependencies/IPlug/ARA_SDK - a directory containing
#    ARA_API and ARA_Library subdirectories (e.g. a clone of Celemony/ARA_SDK with submodules)
# 2. Otherwise the minimal subset (ARA_API + ARA_Library, no examples) is fetched via
#    FetchContent, pinned to the ARA SDK release tag below.

include(${CMAKE_CURRENT_LIST_DIR}/IPlug.cmake)

set(IPLUG2_ARA_SDK_GIT_TAG "releases/2.3.0" CACHE STRING "Git tag used when fetching the ARA SDK")

if(NOT TARGET iPlug2::ARA)
  set(ARA_SDK_DIR "${IPLUG_DEPS_DIR}/ARA_SDK" CACHE PATH "Path to a directory containing ARA_API and ARA_Library")

  set(_ara_supported TRUE)
  set(_ara_include_dir "")

  # ARA VST3 integration requires the VST3 SDK headers
  if(NOT IPLUG2_VST3_SUPPORTED)
    message(STATUS "ARA requires the VST3 SDK, which was not found. ARA targets will not be available.")
    set(_ara_supported FALSE)
  elseif(EXISTS "${ARA_SDK_DIR}/ARA_API" AND EXISTS "${ARA_SDK_DIR}/ARA_Library")
    message(STATUS "Using ARA SDK at ${ARA_SDK_DIR}")
    set(_ara_include_dir "${ARA_SDK_DIR}")
  elseif(CMAKE_VERSION VERSION_LESS 3.19)
    message(STATUS "Fetching the ARA SDK requires CMake 3.19+. ARA targets will not be available.")
    set(_ara_supported FALSE)
  else()
    include(FetchContent)

    if(FETCHCONTENT_BASE_DIR)
      set(_ara_fetch_root "${FETCHCONTENT_BASE_DIR}/ARA")
    else()
      set(_ara_fetch_root "${CMAKE_BINARY_DIR}/_deps/ARA")
    endif()

    message(STATUS "ARA SDK not found at ${ARA_SDK_DIR} - fetching ARA_API and ARA_Library (${IPLUG2_ARA_SDK_GIT_TAG}) into ${_ara_fetch_root}")

    # SOURCE_SUBDIR points at a non-existent directory so that FetchContent_MakeAvailable()
    # populates the sources without calling add_subdirectory() - the required ARA sources
    # are compiled directly into targets that link iPlug2::ARA, like the VST3 SDK sources
    FetchContent_Declare(ara_api
      GIT_REPOSITORY https://github.com/Celemony/ARA_API.git
      GIT_TAG ${IPLUG2_ARA_SDK_GIT_TAG}
      GIT_SHALLOW TRUE
      SOURCE_DIR "${_ara_fetch_root}/ARA_API"
      SOURCE_SUBDIR "iplug2-no-add-subdirectory"
    )
    FetchContent_Declare(ara_library
      GIT_REPOSITORY https://github.com/Celemony/ARA_Library.git
      GIT_TAG ${IPLUG2_ARA_SDK_GIT_TAG}
      GIT_SHALLOW TRUE
      SOURCE_DIR "${_ara_fetch_root}/ARA_Library"
      SOURCE_SUBDIR "iplug2-no-add-subdirectory"
    )
    FetchContent_MakeAvailable(ara_api ara_library)

    set(_ara_include_dir "${_ara_fetch_root}")
  endif()

  if(NOT _ara_supported)
    set(IPLUG2_ARA_SUPPORTED FALSE CACHE INTERNAL "ARA SDK available")
    # Create a dummy iPlug2::ARA target so projects can link to it without errors
    add_library(iPlug2::ARA INTERFACE IMPORTED)
  else()
    set(IPLUG2_ARA_SUPPORTED TRUE CACHE INTERNAL "ARA SDK available")
    set(IPLUG2_ARA_INCLUDE_DIR "${_ara_include_dir}" CACHE INTERNAL "Parent directory of ARA_API and ARA_Library")

    add_library(iPlug2::ARA INTERFACE IMPORTED)

    # Minimal ARA_Library plug-in side sources - everything else is header-only
    set(ARA_SDK_SRC
      ${_ara_include_dir}/ARA_Library/PlugIn/ARAPlug.cpp
      ${_ara_include_dir}/ARA_Library/Dispatch/ARAPlugInDispatch.cpp
      ${_ara_include_dir}/ARA_Library/Debug/ARADebug.c
      ${_ara_include_dir}/ARA_Library/Utilities/ARAChannelFormat.cpp
      ${_ara_include_dir}/ARA_Library/Utilities/ARAPitchInterpretation.cpp
    )

    target_sources(iPlug2::ARA INTERFACE ${ARA_SDK_SRC})

    target_include_directories(iPlug2::ARA INTERFACE
      ${_ara_include_dir}
      ${IPLUG_DIR}/ARA
    )

    target_compile_definitions(iPlug2::ARA INTERFACE
      ARA_API
    )

    option(IPLUG2_ARA_VALIDATE_API_CALLS "Enable the ARA SDK's API call validation asserts in debug builds" ON)

    # Log every host API entry, model object lifetime change and model edit (os_log on macOS,
    # OutputDebugString on Windows) - useful to see what a host does before it trips an assert
    option(IPLUG2_ARA_DEBUG_LOG "Log ARA host API calls and model object lifetimes in debug builds" OFF)

    if(NOT IPLUG2_ARA_VALIDATE_API_CALLS)
      target_compile_definitions(iPlug2::ARA INTERFACE ARA_VALIDATE_API_CALLS=0)
    endif()

    if(IPLUG2_ARA_DEBUG_LOG)
      target_compile_definitions(iPlug2::ARA INTERFACE
        $<$<CONFIG:Debug>:ARA_ENABLE_HOST_ENTRY_LOG=1>
        $<$<CONFIG:Debug>:ARA_ENABLE_OBJECT_LIFETIME_LOG=1>
        $<$<CONFIG:Debug>:ARA_ENABLE_MODEL_EDIT_LOG=1>
      )
    endif()

    target_link_libraries(iPlug2::ARA INTERFACE iPlug2::IPlug)
  endif()
endif()
