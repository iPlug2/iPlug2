#  ==============================================================================
#  
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

cmake_minimum_required(VERSION 3.11)

set(AUv2_INSTALL_PATH "$ENV{HOME}/Library/Audio/Plug-Ins/Components")

#################
# Audio Unit v2 #
#################

find_library(AUDIOUNIT_LIB AudioUnit)
find_library(COREAUDIO_LIB CoreAudio)

add_library(iPlug2_AUv2 INTERFACE)
set(_sdk ${IPLUG2_DIR}/IPlug/AUv2)
iplug_target_add(iPlug2_AUv2 INTERFACE
  DEFINE "AU_API" "IPLUG_EDITOR=1" "IPLUG_DSP=1" "SWELL_CLEANUP_ON_UNLOAD"
  LINK iPlug2_Core ${AUDIOUNIT_LIB} ${COREAUDIO_LIB} "-framework CoreMidi" "-framework AudioToolbox"
  INCLUDE ${_sdk}
  SOURCE 
    ${_sdk}/dfx-au-utilities.c
    ${_sdk}/IPlugAU.cpp
    ${_sdk}/IPlugAU.r
    ${_sdk}/IPlugAU_view_factory.mm
  )
iplug_source_tree(iPlug2_AUv2)

function(iplug_configure_auv2 target)
  iplug_target_add(${target} PUBLIC LINK iPlug2_AUv2)

  set(out_dir "${CMAKE_BINARY_DIR}/out/${PLUG_NAME}.component")
  set(res_dir "${out_dir}/Contents/Resources")
  set(install_dir "${AUv2_INSTALL_PATH}/${PLUG_NAME}.component")
  
  set_target_properties(${target} PROPERTIES
    BUNDLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${PLUG_RESOURCES_DIR}/${PLUG_NAME}-AU-Info.plist
    BUNDLE_EXTENSION "component"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out"
    PREFIX ""
    SUFFIX ""
    XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym"
    XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES"
  )

  # Make sure Xcode generator uses the same output directories
  if(CMAKE_GENERATOR STREQUAL "Xcode")
  set_target_properties(${target} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/out"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/out"
  )
  endif()

  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${out_dir}/Contents/MacOS"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${target}>" "${out_dir}/Contents/MacOS"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${out_dir}" "${install_dir}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/out"
    COMMAND dsymutil "$<TARGET_FILE:${target}>" -o "${CMAKE_BINARY_DIR}/out/${PLUG_NAME}.auv2.dSYM"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_BUNDLE_DIR:${target}>.dSYM" "${CMAKE_BINARY_DIR}/out/${PLUG_NAME}.auv2.dSYM" || echo "No dSYM found for AUv2"
  )
    
  set(PKGINFO_FILE "${out_dir}/Contents/PkgInfo")
  file(WRITE ${PKGINFO_FILE} "BNDL????")
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E touch ${PKGINFO_FILE})

  iplug_target_bundle_resources(${target} "${res_dir}")
endfunction(iplug_configure_auv2)