#  ==============================================================================
#  
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0076 NEW)
cmake_policy(SET CMP0054 NEW) # Only interpret if() arguments as variables or keywords when unquoted 

set(iPlug2_FOUND 1)

set(IPLUG_APP_NAME ${CMAKE_PROJECT_NAME} CACHE STRING "Name of the VST/AU/App/etc.")
set(PLUG_NAME ${CMAKE_PROJECT_NAME} CACHE STRING "Name of the VST/AU/App/etc.")

include("${IPLUG2_CMAKE_DIR}/iPlug2Helpers.cmake")
include(CheckCXXCompilerFlag)

if (WIN32)
  # Need to determine processor arch for postbuild-win.bat
  if (CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64")
    set(PROCESSOR_ARCH "x64" CACHE STRING "Processor architecture")
  else()
    set(PROCESSOR_ARCH "Win32" CACHE STRING "Processor architecture")
  endif()
  set(OS_WINDOWS 1)

elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
  find_program( IBTOOL ibtool HINTS "/usr/bin" "${OSX_DEVELOPER_ROOT}/usr/bin" )
  if ( ${IBTOOL} STREQUAL "IBTOOL-NOTFOUND" )
    message( "ibtool can not be found" SEND_ERROR )
  endif()
  set(OS_MAC 1)

elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
  find_package(PkgConfig REQUIRED)
  set(OS_LINUX 1)

else()
  message("Unsupported platform" FATAL_ERROR)
endif()

CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_OPT_ARCH_NATIVE_SUPPORTED)
CHECK_CXX_COMPILER_FLAG("/arch:AVX" COMPILER_OPT_ARCH_AVX_SUPPORTED)

############################
# General iPlug2 Interface #
############################

set(IPLUG_SRC ${IPLUG2_DIR}/IPlug)
set(IGRAPHICS_SRC ${IPLUG2_DIR}/IGraphics)
set(WDL_DIR ${IPLUG2_DIR}/WDL)
set(IPLUG_DEPS ${IPLUG2_DIR}/Dependencies/IPlug)
set(IGRAPHICS_DEPS ${IPLUG2_DIR}/Dependencies/IGraphics)
set(BUILD_DEPS ${IPLUG2_DIR}/Dependencies/Build)

# Core iPlug2 interface. All targets MUST link to this.
add_library(iPlug2_Core INTERFACE)

# Make sure we define DEBUG for debug builds
set(_def "NOMINMAX" "$<$<CONFIG:Debug>:DEBUG>")
set(_opts "")
set(_lib "")
set(_inc
  # iPlug2
  ${WDL_DIR}
  ${WDL_DIR}/libpng
  ${WDL_DIR}/zlib
  ${IPLUG_SRC}
  ${IPLUG_SRC}/Extras
)

set(sdk ${IPLUG_SRC})
set(_src
  ${sdk}/IPlugAPIBase.h
  ${sdk}/IPlugAPIBase.cpp
  ${sdk}/IPlugConstants.h
  ${sdk}/IPlugEditorDelegate.h
  ${sdk}/IPlugLogger.h
  ${sdk}/IPlugMidi.h
  ${sdk}/IPlugParameter.h
  ${sdk}/IPlugParameter.cpp
  ${sdk}/IPlugPaths.h
  ${sdk}/IPlugPaths.cpp
  ${sdk}/IPlugPlatform.h
  ${sdk}/IPlugPluginBase.h
  ${sdk}/IPlugPluginBase.cpp
  ${sdk}/IPlugProcessor.h
  ${sdk}/IPlugProcessor.cpp
  ${sdk}/IPlugQueue.h
  ${sdk}/IPlugStructs.h
  ${sdk}/IPlugTimer.h
  ${sdk}/IPlugTimer.cpp
  ${sdk}/IPlugUtilities.h
)

# Platform Settings
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
  list(APPEND _src ${IGRAPHICS_SRC}/Platforms/IGraphicsWin.cpp)
  target_link_libraries(iPlug2_Core INTERFACE "Shlwapi.lib" "comctl32.lib" "wininet.lib")
  
  # postbuild-win.bat is used by VST2/VST3/AAX on Windows, so we just always configure it on Windows
  # Note: For visual studio, we COULD use $(TargetPath) for the target, but for all other generators, no.
  set(plugin_build_dir "${CMAKE_BINARY_DIR}/out")
  set(create_bundle_script "${IPLUG2_DIR}/Scripts/create_bundle.bat")
  configure_file("${IPLUG2_DIR}/Scripts/postbuild-win.bat.in" "${CMAKE_BINARY_DIR}/postbuild-win.bat")

elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
  list(APPEND _inc
    ${WDL_DIR}/swell
  )
  list(APPEND _lib "pthread" "rt")
  list(APPEND _opts "-Wno-multichar")

elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
  list(APPEND _src 
    ${IPLUG_SRC}/IPlugPaths.mm
    ${IGRAPHICS_SRC}/Platforms/IGraphicsMac.mm
    ${IGRAPHICS_SRC}/Platforms/IGraphicsMac_view.mm
    ${IGRAPHICS_SRC}/Platforms/IGraphicsCoreText.mm
  )
  list(APPEND _inc ${WDL_DIR}/swell)
  list(APPEND _lib
    "-framework CoreFoundation" "-framework CoreData" "-framework Foundation" "-framework CoreServices"
  )
  list(APPEND _opts "-Wno-deprecated-declarations"  "-Wno-c++11-narrowing")
else()
  message("Unhandled system ${CMAKE_SYSTEM_NAME}" FATAL_ERROR)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  list(APPEND _def "_CRT_SECURE_NO_WARNINGS" "_CRT_SECURE_NO_DEPRECATE" "_CRT_NONSTDC_NO_DEPRECATE" "NOMINMAX" "_MBCS")
  list(APPEND _opts "/wd4996" "/wd4250" "/wd4018" "/wd4267" "/wd4068" "/MT$<$<CONFIG:Debug>:d>")
endif()

# Set certain compiler flags, specifically errors if there are undefined symbols
if ((CMAKE_CXX_COMPILER_ID MATCHES "Clang") OR (CMAKE_CXX_COMPILER_ID MATCHES "GNU"))
  list(APPEND _opts "-Wl,--no-undefined")
endif()

# Use advanced SIMD instructions when available.
if (COMPILER_OPT_ARCH_NATIVE_SUPPORTED)
  list(APPEND _opts "-march=native")
elseif(COMPILER_OPT_ARCH_AVX_SUPPORTED)
  list(APPEND _opts "/arch:AVX")
endif()

source_group(TREE ${IPLUG2_DIR} PREFIX "IPlug" FILES ${_src})
iplug_target_add(iPlug2_Core INTERFACE DEFINE ${_def} INCLUDE ${_inc} SOURCE ${_src} OPTION ${_opts} LINK ${_lib})

#############
# IGraphics #
#############

# We include this first because APP requires LICE.
include("${IPLUG2_CMAKE_DIR}/IGraphics.cmake")

####################
# Reaper Extension #
####################

add_library(iPlug2_REAPER INTERFACE)
set(_sdk ${IPLUG2_DIR}/IPlug/ReaperExt)
iplug_target_add(iPlug2_REAPER INTERFACE
  INCLUDE "${_sdk}" "${IPLUG_DEPS}/IPlug/Reaper"
  SOURCE "${_sdk}/ReaperExtBase.cpp"
  DEFINE "REAPER_PLUGIN"
  LINK iPlug2_VST2
)

###############################
# Minor Configuration Targets #
###############################

add_library(iPlug2_Faust INTERFACE)
iplug_target_add(iPlug2_Faust INTERFACE
  INCLUDE "${IPLUG2_DIR}/IPlug/Extras/Faust" "${FAUST_INCLUDE_DIR}"
)

add_library(iPlug2_FaustGen INTERFACE)
iplug_target_add(iPlug2_FaustGen INTERFACE
  SOURCE "${IPLUG_SRC}/Extras/Faust/IPlugFaustGen.cpp"
  LINK iPlug2_Faust)
iplug_source_tree(iPlug2_FaustGen)

add_library(iPlug2_HIIR INTERFACE)
iplug_target_add(iPlug2_HIIR INTERFACE
  INCLUDE ${IPLUG_SRC}/Extras/HIIR
  SOURCE "${IPLUG_SRC}/Extras/HIIR/PolyphaseIIR2Designer.cpp")
iplug_source_tree(iPlug2_HIIR)

add_library(iPlug2_OSC INTERFACE)
iplug_target_add(iPlug2_OSC INTERFACE
  INCLUDE ${IPLUG_SRC}/Extras/OSC
  SOURCE ${IPLUG_SRC}/Extras/OSC/IPlugOSC_msg.cpp)
iplug_source_tree(iPlug2_OSC)

add_library(iPlug2_Synth INTERFACE)
iplug_target_add(iPlug2_Synth INTERFACE
  INCLUDE ${IPLUG_SRC}/Extras/Synth
  SOURCE
    "${IPLUG_SRC}/Extras/Synth/MidiSynth.cpp"
    "${IPLUG_SRC}/Extras/Synth/VoiceAllocator.cpp")
iplug_source_tree(iPlug2_Synth)


#! iplug_project_setup : Setup an iPlug2 project
# 
# \param:NAME Name of the plugin/project. This will be used as the output name
#   for targets unless otherwise specified.
#
macro(iplug_project_setup)
  cmake_parse_arguments("arg" "" "NAME" "" ${ARGN})
  iplug_ternary(IPLUG_PROJECT_NAME "${arg_NAME}" "${CMAKE_PROJECT_NAME}" arg_NAME)
  set(IPLUG_PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endmacro(iplug_project_setup)

#! iplug_configure_target : Configure a target for the given output type
#
# \param:NAME Output name for this particular target, overrides IPLUG_PROJECT_NAME
#
function(iplug_configure_target target target_type)
  cmake_parse_arguments("arg" "" "NAME" "" ${ARGN})

  set_property(TARGET ${target} PROPERTY CXX_STANDARD ${IPLUG2_CXX_STANDARD})

  # ALL Configurations
  if (WIN32)
    # On Windows ours fonts are included in the RC file, meaning we need to include main.rc
    # in ALL our builds. Yay for platform-specific bundling!
    set(_res "${CMAKE_SOURCE_DIR}/resources/main.rc")
    iplug_target_add(${target} PUBLIC RESOURCE ${_res})
    source_group("Resources" FILES ${_res})
    
  elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin") 
    # For MacOS we make sure the output name is the same as the app name.
    # This is basically required for bundles.
    set_property(TARGET ${target} PROPERTY OUTPUT_NAME "${PLUG_NAME}")

  elseif (OS_LINUX)

    # Fix LICE linking
    get_target_property(libs ${target} LINK_LIBRARIES)
    if ("${libs}" MATCHES "iPlug2_LICE")
      if ("${target_type}" STREQUAL "app")
        target_link_libraries(${target} PUBLIC iPlug2_LICE_APP)
      else()
        target_link_libraries(${target} PUBLIC iPlug2_LICE_Normal)
      endif()
    endif()

  endif()
  
  if ("${target_type}" STREQUAL "aax")
    if (NOT TARGET iPlug2_AAX)
      include("${IPLUG2_CMAKE_DIR}/AAX.cmake")
    endif()
    iplug_configure_aax(${target})
  elseif ("${target_type}" STREQUAL "app")
    if (NOT TARGET iPlug2_APP)
      include("${IPLUG2_CMAKE_DIR}/APP.cmake")
    endif()
    iplug_configure_app(${target})
  elseif ("${target_type}" STREQUAL "au2")
    if (NOT TARGET iPlug2_AUv2)
      include("${IPLUG2_CMAKE_DIR}/AudioUnit.cmake")
    endif()
    iplug_configure_au2(${target})
  elseif ("${target_type}" STREQUAL "au3")
    if (NOT TARGET iPlug2_AUv3)
      include("${IPLUG2_CMAKE_DIR}/AudioUnit.cmake")
    endif()
    iplug_configure_au3(${target})
  elseif ("${target_type}" STREQUAL "lv2")
    if (NOT TARGET iPlug2_LV2)
      include("${IPLUG2_CMAKE_DIR}/LV2.cmake")
    endif()
    iplug_configure_lv2(${target})
  # elseif ("${target_type}" STREQUAL "reaper")
  #   iplug_conifgure_reaper(${target})
  elseif ("${target_type}" STREQUAL "vst2")
    if (NOT TARGET iPlug2_VST2)
      include("${IPLUG2_CMAKE_DIR}/VST2.cmake")
    endif()
    iplug_configure_vst2(${target})
  elseif ("${target_type}" STREQUAL "vst3")
    if (NOT TARGET iPlug2_VST3)
      include("${IPLUG2_CMAKE_DIR}/VST3.cmake")
    endif()
    iplug_configure_vst3(${target})
  elseif ("${target_type}" STREQUAL "web")
    if (NOT TARGET iPlug2_WEB)
      include("${IPLUG2_CMAKE_DIR}/WEB.cmake")
    endif()
    iplug_configure_web(${target})
  elseif ("${target_type}" STREQUAL "wam")
    if (NOT TARGET iPlug2_WAM)
      include("${IPLUG2_CMAKE_DIR}/WEB.cmake")
    endif()
    iplug_configure_wam(${target})
  else()
    message("Unknown target type \'${target_type}\' for target '${target}'" FATAL_ERROR)
  endif()
endfunction()
