#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# VST3 target configuration for iPlug2

include(${CMAKE_CURRENT_LIST_DIR}/IPlug.cmake)

if(NOT TARGET iPlug2::VST3)
  # Define SDK path
  set(VST3_SDK_DIR ${IPLUG_DEPS_DIR}/VST3_SDK)

  # Check if VST3 SDK exists
  if(NOT EXISTS ${VST3_SDK_DIR})
    message(STATUS "VST3 SDK not found at ${VST3_SDK_DIR}. VST3 targets will not be available.")
    set(IPLUG2_VST3_SUPPORTED FALSE CACHE INTERNAL "VST3 SDK available")
    # Create a dummy iPlug2::VST3 target so projects can link to it without errors
    add_library(iPlug2::VST3 INTERFACE IMPORTED)
    # Define stub function that excludes the target from default build
    function(iplug_configure_vst3 target project_name)
      message(STATUS "Skipping VST3 target '${target}' - VST3 SDK not available")
      set_target_properties(${target} PROPERTIES EXCLUDE_FROM_ALL TRUE)
    endfunction()
    return()
  endif()

  set(IPLUG2_VST3_SUPPORTED TRUE CACHE INTERNAL "VST3 SDK available")

  add_library(iPlug2::VST3 INTERFACE IMPORTED)

  # iPlug2 VST3 wrapper sources
  set(VST3_IPLUG_SRC
    ${IPLUG_DIR}/VST3/IPlugVST3.cpp
    ${IPLUG_DIR}/VST3/IPlugVST3_ProcessorBase.cpp
  )

  # VST3 SDK base sources
  set(VST3_SDK_BASE_SRC
    ${VST3_SDK_DIR}/base/source/baseiids.cpp
    ${VST3_SDK_DIR}/base/source/fdebug.cpp
    ${VST3_SDK_DIR}/base/source/fobject.cpp
    ${VST3_SDK_DIR}/base/source/fstring.cpp
    ${VST3_SDK_DIR}/base/source/updatehandler.cpp
    ${VST3_SDK_DIR}/base/thread/source/flock.cpp
  )

  # VST3 SDK pluginterfaces sources
  set(VST3_SDK_PLUGINTERFACES_SRC
    ${VST3_SDK_DIR}/pluginterfaces/base/funknown.cpp
    ${VST3_SDK_DIR}/pluginterfaces/base/ustring.cpp
    ${VST3_SDK_DIR}/pluginterfaces/base/coreiids.cpp
  )

  # VST3 SDK common sources
  set(VST3_SDK_COMMON_SRC
    ${VST3_SDK_DIR}/public.sdk/source/common/pluginview.cpp
    ${VST3_SDK_DIR}/public.sdk/source/common/commoniids.cpp
  )

  # VST3 SDK vst sources (single component effect pattern)
  set(VST3_SDK_VST_SRC
    ${VST3_SDK_DIR}/public.sdk/source/vst/vstbus.cpp
    ${VST3_SDK_DIR}/public.sdk/source/vst/vstcomponent.cpp
    ${VST3_SDK_DIR}/public.sdk/source/vst/vstcomponentbase.cpp
    ${VST3_SDK_DIR}/public.sdk/source/vst/vstinitiids.cpp
    ${VST3_SDK_DIR}/public.sdk/source/vst/vstparameters.cpp
    ${VST3_SDK_DIR}/public.sdk/source/vst/vstsinglecomponenteffect.cpp
  )

  # Combine all SDK sources
  set(VST3_SDK_SRC
    ${VST3_SDK_BASE_SRC}
    ${VST3_SDK_PLUGINTERFACES_SRC}
    ${VST3_SDK_COMMON_SRC}
    ${VST3_SDK_VST_SRC}
  )

  target_sources(iPlug2::VST3 INTERFACE
    ${VST3_IPLUG_SRC}
    ${VST3_SDK_SRC}
  )

  target_include_directories(iPlug2::VST3 INTERFACE
    ${IPLUG_DIR}/VST3
    ${VST3_SDK_DIR}
    ${VST3_SDK_DIR}/pluginterfaces
    ${VST3_SDK_DIR}/public.sdk
    ${VST3_SDK_DIR}/public.sdk/source
    ${VST3_SDK_DIR}/base
  )

  target_compile_definitions(iPlug2::VST3 INTERFACE
    VST3_API
    IPLUG_EDITOR=1
    IPLUG_DSP=1
    # VST3 SDK requires one of: DEVELOPMENT, RELEASE, _DEBUG, NDEBUG
    $<$<CONFIG:Debug>:DEVELOPMENT>
    $<$<NOT:$<CONFIG:Debug>>:RELEASE>
  )

  # VST3 SDK main sources (common)
  target_sources(iPlug2::VST3 INTERFACE
    ${VST3_SDK_DIR}/public.sdk/source/main/pluginfactory.cpp
  )

  if(WIN32)
    target_sources(iPlug2::VST3 INTERFACE
      ${VST3_SDK_DIR}/public.sdk/source/main/dllmain.cpp
    )
  elseif(APPLE)
    # Set Obj-C++ for iPlug2 VST3 wrapper on macOS
    set_source_files_properties(${VST3_IPLUG_SRC} PROPERTIES LANGUAGE OBJCXX)

    # macOS entry point
    target_sources(iPlug2::VST3 INTERFACE
      ${VST3_SDK_DIR}/public.sdk/source/main/macmain.cpp
    )
    target_link_libraries(iPlug2::VST3 INTERFACE
      "-framework Cocoa"
    )
  elseif(UNIX AND NOT APPLE)
    # Linux support - to be added later
    message(WARNING "VST3 Linux support not yet implemented")
  endif()

  target_link_libraries(iPlug2::VST3 INTERFACE iPlug2::IPlug)
endif()

# Configuration function for VST3 targets
function(iplug_configure_vst3 target project_name)
  target_link_libraries(${target} PUBLIC iPlug2::VST3)

  if(WIN32)
    # Determine architecture for VST3 bundle path
    # VST3 spec: x86_64-win, x86-win, arm64-win, arm64ec-win
    if(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64EC")
      set(VST3_ARCH "arm64ec-win")
    elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64")
      set(VST3_ARCH "arm64-win")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(VST3_ARCH "x86_64-win")
    else()
      set(VST3_ARCH "x86-win")
    endif()

    set(VST3_OUTPUT_DIR "${CMAKE_BINARY_DIR}/out/${project_name}.vst3/Contents/${VST3_ARCH}")

    # Build directly into bundle structure
    # Set for all configs to avoid multi-config generator adding /Release/ etc
    set_target_properties(${target} PROPERTIES
      OUTPUT_NAME "${project_name}"
      SUFFIX ".vst3"
      LIBRARY_OUTPUT_DIRECTORY "${VST3_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_DEBUG "${VST3_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_RELEASE "${VST3_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${VST3_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${VST3_OUTPUT_DIR}"
    )

    # Create Resources folder for bundle completeness
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/out/${project_name}.vst3/Contents/Resources"
      COMMENT "Creating VST3 bundle structure for ${project_name}"
    )
  elseif(APPLE)
    # VST3 on macOS is a bundle with .vst3 extension
    set_target_properties(${target} PROPERTIES
      BUNDLE TRUE
      BUNDLE_EXTENSION "vst3"
      MACOSX_BUNDLE_INFO_PLIST ${PLUG_RESOURCES_DIR}/${project_name}-VST3-Info.plist
      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out"
      MACOSX_BUNDLE_BUNDLE_NAME "${project_name}"
      OUTPUT_NAME "${project_name}"
      XCODE_ATTRIBUTE_WRAPPER_EXTENSION "vst3"
      XCODE_ATTRIBUTE_GENERATE_PKGINFO_FILE "YES"
    )

    # For non-Xcode generators (e.g., Ninja), create PkgInfo file manually
    # Generate PkgInfo at configure time for deterministic output (no platform-dependent newlines)
    if(NOT XCODE)
      set(VST3_PKGINFO_PATH "${CMAKE_BINARY_DIR}/iplug2_pkginfo/VST3_PkgInfo")
      file(WRITE "${VST3_PKGINFO_PATH}" "BNDL????")
      set(PKGINFO_DEST "${CMAKE_BINARY_DIR}/out/${project_name}.vst3/Contents/PkgInfo")
      add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${VST3_PKGINFO_PATH}" "${PKGINFO_DEST}"
        COMMENT "Creating PkgInfo for ${project_name}.vst3"
      )
    endif()
  endif()
endfunction()
