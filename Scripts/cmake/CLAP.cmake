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
  # Define SDK paths
  set(CLAP_SDK_DIR ${IPLUG_DEPS_DIR}/CLAP_SDK)
  set(CLAP_HELPERS_DIR ${IPLUG_DEPS_DIR}/CLAP_HELPERS)

  # Check if CLAP SDK exists
  if(NOT EXISTS ${CLAP_SDK_DIR})
    message(WARNING "CLAP_SDK not found at ${CLAP_SDK_DIR}. CLAP target will not be available.")
    return()
  endif()

  if(NOT EXISTS ${CLAP_HELPERS_DIR})
    message(WARNING "CLAP_HELPERS not found at ${CLAP_HELPERS_DIR}. CLAP target will not be available.")
    return()
  endif()

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
    set(CLAP_OUTPUT_DIR "${CMAKE_BINARY_DIR}/out")
    set_target_properties(${target} PROPERTIES
      OUTPUT_NAME "${project_name}"
      SUFFIX ".clap"
      LIBRARY_OUTPUT_DIRECTORY "${CLAP_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CLAP_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CLAP_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CLAP_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${CLAP_OUTPUT_DIR}"
    )
  elseif(APPLE)
    # CLAP on macOS is a bundle with .clap extension
    set_target_properties(${target} PROPERTIES
      BUNDLE TRUE
      BUNDLE_EXTENSION "clap"
      MACOSX_BUNDLE_INFO_PLIST ${PLUG_RESOURCES_DIR}/${project_name}-CLAP-Info.plist
      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out"
      MACOSX_BUNDLE_BUNDLE_NAME "${project_name}"
      OUTPUT_NAME "${project_name}"
      XCODE_ATTRIBUTE_WRAPPER_EXTENSION "clap"
      XCODE_ATTRIBUTE_GENERATE_PKGINFO_FILE "YES"
    )

    # For non-Xcode generators (e.g., Ninja), create PkgInfo file manually
    if(NOT XCODE)
      set(PKGINFO_PATH "${CMAKE_BINARY_DIR}/out/${project_name}.clap/Contents/PkgInfo")
      add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "BNDL????" > "${PKGINFO_PATH}"
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
