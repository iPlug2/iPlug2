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

  # Check if CLAP SDK exists (must have include dir, not just placeholder)
  if(NOT EXISTS ${CLAP_SDK_DIR}/include)
    message(STATUS "CLAP SDK not found at ${CLAP_SDK_DIR}. CLAP targets will not be available.")
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

  if(NOT EXISTS ${CLAP_HELPERS_DIR}/include)
    message(STATUS "CLAP helpers not found at ${CLAP_HELPERS_DIR}. CLAP targets will not be available.")
    set(IPLUG2_CLAP_SUPPORTED FALSE CACHE INTERNAL "CLAP SDK available")
    add_library(iPlug2::CLAP INTERFACE IMPORTED)
    function(iplug_configure_clap target project_name)
      message(STATUS "Skipping CLAP target '${target}' - CLAP helpers not available")
      set_target_properties(${target} PROPERTIES EXCLUDE_FROM_ALL TRUE)
    endfunction()
    return()
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
    # Check for generated plist first, fall back to resources folder
    set(GENERATED_PLIST "${IPLUG_GENERATED_RESOURCES_DIR}/${project_name}-CLAP-Info.plist")
    if(IPLUG_GENERATED_RESOURCES_DIR AND EXISTS "${GENERATED_PLIST}")
      set(CLAP_PLIST "${GENERATED_PLIST}")
    else()
      set(CLAP_PLIST "${PLUG_RESOURCES_DIR}/${project_name}-CLAP-Info.plist")
    endif()

    # CLAP on macOS is a bundle with .clap extension
    set_target_properties(${target} PROPERTIES
      BUNDLE TRUE
      BUNDLE_EXTENSION "clap"
      MACOSX_BUNDLE_INFO_PLIST ${CLAP_PLIST}
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
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/out/${project_name}.clap/Contents"
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
