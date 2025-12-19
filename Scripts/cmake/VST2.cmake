#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# VST2 target configuration for iPlug2

include(${CMAKE_CURRENT_LIST_DIR}/IPlug.cmake)

if(NOT TARGET iPlug2::VST2)
  # Define SDK path
  set(VST2_SDK_DIR ${IPLUG_DEPS_DIR}/VST2_SDK)

  # Check if VST2 SDK exists (must have aeffectx.h header)
  if(NOT EXISTS ${VST2_SDK_DIR}/pluginterfaces/vst2.x/aeffectx.h)
    # Also check root directory for older SDK layouts
    if(NOT EXISTS ${VST2_SDK_DIR}/aeffectx.h)
      message(STATUS "VST2 SDK not found at ${VST2_SDK_DIR}. VST2 targets will not be available.")
      set(IPLUG2_VST2_SUPPORTED FALSE CACHE INTERNAL "VST2 SDK available")
      # Create a dummy iPlug2::VST2 target so projects can link to it without errors
      add_library(iPlug2::VST2 INTERFACE IMPORTED)
      # Define stub function that excludes the target from default build
      function(iplug_configure_vst2 target project_name)
        message(STATUS "Skipping VST2 target '${target}' - VST2 SDK not available")
        set_target_properties(${target} PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endfunction()
      return()
    endif()
  endif()

  set(IPLUG2_VST2_SUPPORTED TRUE CACHE INTERNAL "VST2 SDK available")

  add_library(iPlug2::VST2 INTERFACE IMPORTED)

  # iPlug2 VST2 wrapper sources
  set(VST2_IPLUG_SRC
    ${IPLUG_DIR}/VST2/IPlugVST2.cpp
  )

  target_sources(iPlug2::VST2 INTERFACE ${VST2_IPLUG_SRC})

  target_include_directories(iPlug2::VST2 INTERFACE
    ${IPLUG_DIR}/VST2
    ${VST2_SDK_DIR}
  )

  target_compile_definitions(iPlug2::VST2 INTERFACE
    VST2_API
    VST_FORCE_DEPRECATED
    IPLUG_EDITOR=1
    IPLUG_DSP=1
  )

  if(WIN32)
    # Windows VST2 support - no additional dependencies
  elseif(APPLE)
    # Set Obj-C++ for iPlug2 VST2 wrapper on macOS
    set_source_files_properties(${VST2_IPLUG_SRC} PROPERTIES LANGUAGE OBJCXX)

    target_link_libraries(iPlug2::VST2 INTERFACE
      "-framework Cocoa"
    )
  elseif(UNIX AND NOT APPLE)
    # Linux support - to be added later
    message(WARNING "VST2 Linux support not yet implemented")
  endif()

  target_link_libraries(iPlug2::VST2 INTERFACE iPlug2::IPlug)
endif()

# Configuration function for VST2 targets
function(iplug_configure_vst2 target project_name)
  target_link_libraries(${target} PUBLIC iPlug2::VST2)

  if(WIN32)
    set(VST2_OUTPUT_DIR "${CMAKE_BINARY_DIR}/out")
    set_target_properties(${target} PROPERTIES
      OUTPUT_NAME "${project_name}"
      SUFFIX ".dll"
      LIBRARY_OUTPUT_DIRECTORY "${VST2_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_DEBUG "${VST2_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_RELEASE "${VST2_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${VST2_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${VST2_OUTPUT_DIR}"
    )
  elseif(APPLE)
    # VST2 on macOS is a bundle with .vst extension
    set_target_properties(${target} PROPERTIES
      BUNDLE TRUE
      BUNDLE_EXTENSION "vst"
      MACOSX_BUNDLE_INFO_PLIST ${PLUG_RESOURCES_DIR}/${project_name}-VST2-Info.plist
      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out"
      MACOSX_BUNDLE_BUNDLE_NAME "${project_name}"
      OUTPUT_NAME "${project_name}"
      XCODE_ATTRIBUTE_WRAPPER_EXTENSION "vst"
      XCODE_ATTRIBUTE_GENERATE_PKGINFO_FILE "YES"
    )

    # For non-Xcode generators (e.g., Ninja), create PkgInfo file manually
    if(NOT XCODE)
      set(PKGINFO_PATH "${CMAKE_BINARY_DIR}/out/${project_name}.vst/Contents/PkgInfo")
      add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/out/${project_name}.vst/Contents"
        COMMAND ${CMAKE_COMMAND} -E copy "${IPLUG2_PKGINFO_FILE}" "${PKGINFO_PATH}"
        COMMENT "Creating PkgInfo for ${project_name}.vst"
      )
    endif()
  endif()
endfunction()
