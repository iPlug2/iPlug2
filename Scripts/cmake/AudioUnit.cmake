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

function(iplug_configure_au2 target)
  iplug_target_add(${target} PUBLIC LINK iPlug2_AUv2)

  if (CMAKE_GENERATOR STREQUAL "Xcode")
    set(out_dir "${CMAKE_BINARY_DIR}/$<CONFIG>/${PLUG_NAME}.component")
    set(res_dir "")
  else()
    set(out_dir "${CMAKE_BINARY_DIR}/${PLUG_NAME}.component")
    set(res_dir "${out_dir}/Contents/Resources")
  endif()
  set(install_dir "${AUv2_INSTALL_PATH}/${PLUG_NAME}.component")
  
  set_target_properties(${target} PROPERTIES
    BUNDLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/resources/${PLUG_NAME}-AU-Info.plist
    BUNDLE_EXTENSION "component"
    PREFIX ""
    SUFFIX "")

  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} ARGS "-E" "copy_directory" "${out_dir}" "${install_dir}")

  if (res_dir)
    iplug_target_bundle_resources(${target} "${res_dir}")
  endif()
endfunction(iplug_configure_au2)


#################
# Audio Unit v3 #
#################


set(_sdk ${IPLUG2_DIR}/IPlug/AUv3)
add_library(iPlug2_AUv3 INTERFACE)
iplug_target_add(iPlug2_AUv3 INTERFACE
  INCLUDE ${_sdk}
  SOURCE
    ${_sdk}/GenericUI.mm
    ${_sdk}/IPlugAUAudioUnit.mm
    ${_sdk}/IPlugAUv3.mm
    ${_sdk}/IPlugAUv3Appex.m
    ${_sdk}/IPlugAUViewController.mm
  DEFINE "AUv3_API" "IPLUG_EDITOR=1" "IPLUG_DSP=1" "SWELL_CLEANUP_ON_UNLOAD"
  LINK iPlug2_Core)
iplug_source_tree(iPlug2_AUv3)

function(iplug_configure_au3)
  #message("AUv3 not yet implemented" FATAL_ERROR)
endfunction(iplug_configure_au3)
