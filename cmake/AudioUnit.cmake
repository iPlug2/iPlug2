cmake_minimum_required(VERSION 3.11)


#################
# Audio Unit v2 #
#################

add_library(iPlug2_AU INTERFACE)
set(_sdk ${IPLUG2_DIR}/IPlug/AUv2)
set(_src
  ${_sdk}/dfx-au-utilities.c
  ${_sdk}/IPlugAU.cpp
  ${_sdk}/IPlugAU.r
  ${_sdk}/IPlugAU_view_factory.mm
)
set(_inc
  ${_sdk}
)
set(_def "AU_API" "IPLUG_EDITOR=1" "IPLUG_DSP=1" "SWELL_CLEANUP_ON_UNLOAD")

add_library(iPlugAU INTERFACE)
iplug2_target_add(iPlug2_AU INTERFACE INCLUDE ${_inc} SOURCE ${_src} DEFINE ${_def} LINK iPlug2_Core)

function(iplug2_configure_au2)
  message("AUv2 not yet implemented" FATAL_ERROR)
endfunction(iplug2_configure_au2)


#################
# Audio Unit v3 #
#################

add_library(iPlug2_AUv3 INTERFACE)
set(_sdk ${IPLUG2_DIR}/IPlug/AUv3)
set(_src
  ${_sdk}/GenericUI.mm
  ${_sdk}/IPlugAUAudioUnit.mm
  ${_sdk}/IPlugAUv3.mm
  ${_sdk}/IPlugAUv3Appex.m
  ${_sdk}/IPlugAUViewController.mm
)
set(_inc ${sdk})
set(_def "AUv3_API" "IPLUG_EDITOR=1" "IPLUG_DSP=1" "SWELL_CLEANUP_ON_UNLOAD")

iplug2_target_add(iPlug2_AUv3 INTERFACE INCLUDE ${_inc} SOURCE ${_src} DEFINE ${_def} LINK iPlug2_Core)

function(iplug2_configure_au3)
  message("AUv3 not yet implemented" FATAL_ERROR)
endfunction(iplug2_configure_au3)
