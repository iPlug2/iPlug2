#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# CLAP target configuration for iPlug2

include(${CMAKE_CURRENT_LIST_DIR}/IPlug.cmake)

if(NOT TARGET iPlug2::CLAP)
  # Option to auto-fetch CLAP SDK if not found
  option(IPLUG2_FETCH_CLAP_SDK "Automatically fetch CLAP SDK if not found" OFF)

  # Define SDK paths (can be overridden via cache for FetchContent usage)
  set(IPLUG2_CLAP_SDK_PATH "${IPLUG_DEPS_DIR}/CLAP_SDK" CACHE PATH "Path to CLAP SDK")
  set(IPLUG2_CLAP_HELPERS_PATH "${IPLUG_DEPS_DIR}/CLAP_HELPERS" CACHE PATH "Path to CLAP helpers")
  set(CLAP_SDK_DIR ${IPLUG2_CLAP_SDK_PATH})
  set(CLAP_HELPERS_DIR ${IPLUG2_CLAP_HELPERS_PATH})

  # Check if CLAP SDK exists, optionally fetch it
  if(NOT EXISTS ${CLAP_SDK_DIR}/include)
    if(IPLUG2_FETCH_CLAP_SDK)
      message(STATUS "CLAP SDK not found, fetching from GitHub...")
      include(FetchContent)
      FetchContent_Declare(clap
        GIT_REPOSITORY https://github.com/free-audio/clap.git
        GIT_TAG 1.2.2
        GIT_SHALLOW TRUE
      )
      FetchContent_MakeAvailable(clap)
      set(CLAP_SDK_DIR ${clap_SOURCE_DIR})
      set(IPLUG2_CLAP_SDK_PATH ${CLAP_SDK_DIR} CACHE PATH "Path to CLAP SDK" FORCE)
    else()
      message(STATUS "CLAP SDK not found at ${CLAP_SDK_DIR}. CLAP targets will not be available.")
      message(STATUS "  Set IPLUG2_FETCH_CLAP_SDK=ON to automatically download, or")
      message(STATUS "  Set IPLUG2_CLAP_SDK_PATH to point to an existing CLAP SDK.")
      set(IPLUG2_CLAP_SUPPORTED FALSE CACHE INTERNAL "CLAP SDK available")
      # Create a dummy iPlug2::CLAP target so projects can link to it without errors
      add_library(iPlug2::CLAP INTERFACE IMPORTED)
      # Define stub function that excludes the target from default build
      function(iplug_configure_clap target project_name)
        message(STATUS "Skipping CLAP target '${target}' - CLAP SDK not available")
        set_target_properties(${target} PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endfunction()
      return()
    endif()
  endif()

  # Check if CLAP helpers exists, optionally fetch it
  if(NOT EXISTS ${CLAP_HELPERS_DIR}/include)
    if(IPLUG2_FETCH_CLAP_SDK)
      message(STATUS "CLAP helpers not found, fetching from GitHub...")
      include(FetchContent)
      FetchContent_Declare(clap_helpers
        GIT_REPOSITORY https://github.com/free-audio/clap-helpers.git
        GIT_TAG main
        GIT_SHALLOW TRUE
      )
      FetchContent_MakeAvailable(clap_helpers)
      set(CLAP_HELPERS_DIR ${clap_helpers_SOURCE_DIR})
      set(IPLUG2_CLAP_HELPERS_PATH ${CLAP_HELPERS_DIR} CACHE PATH "Path to CLAP helpers" FORCE)
    else()
      message(STATUS "CLAP helpers not found at ${CLAP_HELPERS_DIR}. CLAP targets will not be available.")
      message(STATUS "  Set IPLUG2_FETCH_CLAP_SDK=ON to automatically download, or")
      message(STATUS "  Set IPLUG2_CLAP_HELPERS_PATH to point to existing CLAP helpers.")
      set(IPLUG2_CLAP_SUPPORTED FALSE CACHE INTERNAL "CLAP SDK available")
      add_library(iPlug2::CLAP INTERFACE IMPORTED)
      function(iplug_configure_clap target project_name)
        message(STATUS "Skipping CLAP target '${target}' - CLAP helpers not available")
        set_target_properties(${target} PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endfunction()
      return()
    endif()
  endif()

  set(IPLUG2_CLAP_SUPPORTED TRUE CACHE INTERNAL "CLAP SDK available")

  add_library(iPlug2::CLAP INTERFACE IMPORTED)

  # iPlug2 CLAP wrapper sources
  set(CLAP_IPLUG_SRC
    ${IPLUG_DIR}/CLAP/IPlugCLAP.cpp
  )

  target_sources(iPlug2::CLAP INTERFACE ${CLAP_IPLUG_SRC})

  target_include_directories(iPlug2::CLAP INTERFACE
    ${IPLUG_DIR}/CLAP
    ${CLAP_SDK_DIR}/include
    ${CLAP_HELPERS_DIR}/include
    ${CLAP_HELPERS_DIR}/include/clap/helpers
  )

  target_compile_definitions(iPlug2::CLAP INTERFACE
    CLAP_API
    IPLUG_EDITOR=1
    IPLUG_DSP=1
  )

  if(WIN32)
    # Windows CLAP support
    # No additional sources needed - CLAP SDK is header-only
  elseif(APPLE)
    # Set Obj-C++ for iPlug2 CLAP wrapper on macOS
    set_source_files_properties(${CLAP_IPLUG_SRC} PROPERTIES LANGUAGE OBJCXX)

    target_link_libraries(iPlug2::CLAP INTERFACE
      "-framework Cocoa"
    )
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    # WASM/Emscripten support - for future use
    # CLAP SDK is header-only and works on WASM
    # Build as STATIC library with --no-entry linker option
  elseif(UNIX AND NOT APPLE)
    # Linux support - to be added later
    message(WARNING "CLAP Linux support not yet implemented")
  endif()

  target_link_libraries(iPlug2::CLAP INTERFACE iPlug2::IPlug)
endif()

# Configuration function for CLAP targets
function(iplug_configure_clap target project_name)
  target_link_libraries(${target} PUBLIC iPlug2::CLAP)

  if(WIN32)
    set_target_properties(${target} PROPERTIES
      OUTPUT_NAME "${project_name}"
      SUFFIX ".clap"
      LIBRARY_OUTPUT_DIRECTORY "${IPLUG2_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_DEBUG "${IPLUG2_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_RELEASE "${IPLUG2_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${IPLUG2_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${IPLUG2_OUTPUT_DIR}"
    )
  elseif(APPLE)
    # CLAP on macOS is a bundle with .clap extension
    set_target_properties(${target} PROPERTIES
      BUNDLE TRUE
      BUNDLE_EXTENSION "clap"
      MACOSX_BUNDLE_INFO_PLIST ${PLUG_RESOURCES_DIR}/${project_name}-CLAP-Info.plist
      LIBRARY_OUTPUT_DIRECTORY "${IPLUG2_OUTPUT_DIR}"
      MACOSX_BUNDLE_BUNDLE_NAME "${project_name}"
      OUTPUT_NAME "${project_name}"
      XCODE_ATTRIBUTE_WRAPPER_EXTENSION "clap"
      XCODE_ATTRIBUTE_GENERATE_PKGINFO_FILE "YES"
    )

    # For non-Xcode generators (e.g., Ninja), create PkgInfo file manually
    if(NOT XCODE)
      set(PKGINFO_PATH "${IPLUG2_OUTPUT_DIR}/${project_name}.clap/Contents/PkgInfo")
      add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${IPLUG2_OUTPUT_DIR}/${project_name}.clap/Contents"
        COMMAND ${CMAKE_COMMAND} -E copy "${IPLUG2_PKGINFO_FILE}" "${PKGINFO_PATH}"
        COMMENT "Creating PkgInfo for ${project_name}.clap"
      )
    endif()
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    # WASM/Emscripten: Use --no-entry linker option
    target_link_options(${target} PRIVATE --no-entry)
    set_target_properties(${target} PROPERTIES
      OUTPUT_NAME "${project_name}"
      SUFFIX ".clap"
    )
  endif()
endfunction()
