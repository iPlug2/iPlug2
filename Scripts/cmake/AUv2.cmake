#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# AUv2 (Audio Unit v2) target configuration for iPlug2
# macOS only

include(${CMAKE_CURRENT_LIST_DIR}/IPlug.cmake)

if(NOT TARGET iPlug2::AUv2)
  if(NOT APPLE)
    message(WARNING "AUv2 is only supported on macOS")
    return()
  endif()

  add_library(iPlug2::AUv2 INTERFACE IMPORTED)

  # iPlug2 AUv2 wrapper sources
  set(AUV2_IPLUG_SRC
    ${IPLUG_DIR}/AUv2/IPlugAU.cpp
    ${IPLUG_DIR}/AUv2/IPlugAU_view_factory.mm
    ${IPLUG_DIR}/AUv2/dfx-au-utilities.c
  )

  target_sources(iPlug2::AUv2 INTERFACE ${AUV2_IPLUG_SRC})

  # Set Obj-C++ for the AU wrapper sources
  set_source_files_properties(${IPLUG_DIR}/AUv2/IPlugAU.cpp
    PROPERTIES LANGUAGE OBJCXX
  )
  set_source_files_properties(${IPLUG_DIR}/AUv2/IPlugAU_view_factory.mm
    PROPERTIES LANGUAGE OBJCXX
  )

  target_include_directories(iPlug2::AUv2 INTERFACE
    ${IPLUG_DIR}/AUv2
  )

  target_compile_definitions(iPlug2::AUv2 INTERFACE
    AU_API
    IPLUG_EDITOR=1
    IPLUG_DSP=1
  )

  target_link_libraries(iPlug2::AUv2 INTERFACE
    "-framework AudioUnit"
    "-framework AudioToolbox"
    "-framework CoreAudio"
    "-framework CoreMIDI"
    "-framework Cocoa"
  )

  target_link_libraries(iPlug2::AUv2 INTERFACE iPlug2::IPlug)
endif()

# Configuration function for AUv2 targets
function(iplug_configure_auv2 target project_name)
  target_link_libraries(${target} PUBLIC iPlug2::AUv2)

  if(APPLE)
    # AUv2 on macOS is a bundle with .component extension
    set_target_properties(${target} PROPERTIES
      BUNDLE TRUE
      BUNDLE_EXTENSION "component"
      MACOSX_BUNDLE_INFO_PLIST ${PLUG_RESOURCES_DIR}/${project_name}-AU-Info.plist
      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out"
      MACOSX_BUNDLE_BUNDLE_NAME "${project_name}"
      OUTPUT_NAME "${project_name}"
      XCODE_ATTRIBUTE_WRAPPER_EXTENSION "component"
      XCODE_ATTRIBUTE_GENERATE_PKGINFO_FILE "YES"
    )

    # For non-Xcode generators (e.g., Ninja), create PkgInfo file manually
    if(NOT XCODE)
      set(PKGINFO_PATH "${CMAKE_BINARY_DIR}/out/${project_name}.component/Contents/PkgInfo")
      add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/out/${project_name}.component/Contents"
        COMMAND ${CMAKE_COMMAND} -E copy "${IPLUG2_PKGINFO_FILE}" "${PKGINFO_PATH}"
        COMMENT "Creating PkgInfo for ${project_name}.component"
      )
    endif()
  endif()
endfunction()
