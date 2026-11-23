#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# ARA-enabled VST3 target configuration for iPlug2
#
# A VST3_ARA target is a regular VST3 plug-in that additionally exposes the ARA
# plug-in extension (ARA::IPlugInEntryPoint2) and ARA main factory. Targets link
# iPlug2::VST3 and iPlug2::ARA. Note that a VST3_ARA target produces a bundle with
# the same name as a plain VST3 target, so a project should build one or the other.

include(${CMAKE_CURRENT_LIST_DIR}/VST3.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/ARA.cmake)

# Configuration function for ARA-enabled VST3 targets
function(iplug_configure_vst3ara target project_name)
  if(NOT IPLUG2_ARA_SUPPORTED)
    message(STATUS "Skipping VST3 ARA target '${target}' - ARA SDK not available")
    set_target_properties(${target} PROPERTIES EXCLUDE_FROM_ALL TRUE)
    return()
  endif()

  # Bundle/packaging setup is identical to a plain VST3
  iplug_configure_vst3(${target} ${project_name})
endfunction()
