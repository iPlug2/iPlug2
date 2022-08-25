#################
# Audio Unit v3 #
#################


set(_sdk ${IPLUG2_DIR}/IPlug/AUv3)
add_library(iPlug2_AUv3 INTERFACE)
iplug_target_add(iPlug2_AUv3 INTERFACE
  INCLUDE ${_sdk}
  SOURCE
    ${_sdk}/IPlugAUAudioUnit.mm
    ${_sdk}/IPlugAUv3.mm
    ${_sdk}/IPlugAUv3Appex.m
    ${_sdk}/IPlugAUViewController.mm
  DEFINE "AUv3_API" "IPLUG_EDITOR=1" "IPLUG_DSP=1" "SWELL_CLEANUP_ON_UNLOAD"
  LINK iPlug2_Core)
iplug_source_tree(iPlug2_AUv3)

function(iplug_configure_auv3)
  #message("AUv3 not yet implemented" FATAL_ERROR)
endfunction(iplug_configure_auv3)