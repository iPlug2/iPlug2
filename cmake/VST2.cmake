cmake_minimum_required(VERSION 3.11)

# Determine VST2 and VST3 directories
set(_doc "Path to install VST2 plugins")
set(vst2_default_path "")
if (WIN32)
  set(_names "VstPlugins")
  if (PROCESSOR_ARCH STREQUAL "Win32")
    set(_paths
      "C:/Program Files (x86)/"
      "C:/Program Files (x86)/Steinberg/")
    set(vst2_default_path "C:/Program Files (x86)/${_names}/")
  else()
    set(vst2_default_path "C:/Program Files/${_names}/")
  endif()
  # Append this for x86, x64, and ARM I guess
  list(APPEND _paths
    "C:/Program Files/"
    "C:/Program Files/Steinberg/")
  
  find_file(VST2_PATH NAMES ${_names} PATHS ${_paths} DOC ${_doc})

elseif (OS_MAC)
  find_file(VST2_PATH
    NAMES "VST"
    PATHS "$ENV{HOME}/Library/Audio/Plug-Ins"
    DOC ${_doc})
  set(vst2_default_path "$ENV{HOME}/Library/Audio/Plug-Ins/VST/")

elseif (OS_LINUX)
  find_file(VST2_PATH
    NAMES ".vst"
    PATHS "$ENV{HOME}"
    DOC ${_doc})
  set(vst2_default_path "$ENV{HOME}/.vst/")

endif()

# If we didn't find a VST2 path, pick the default
set(VST2_PATH "${vst2_default_path}" CACHE PATH ${_doc})

set(sdk ${IPLUG2_DIR}/IPlug/VST2)

add_library(iPlug2_VST2 INTERFACE)
iplug2_target_add(iPlug2_VST2 INTERFACE
  INCLUDE ${sdk} ${IPLUG_DEPS}/VST2_SDK
  SOURCE ${sdk}/IPlugVST2.cpp
  DEFINE "VST2_API" "VST_FORCE_DEPRECATED" "IPLUG_EDITOR=1" "IPLUG_DSP=1"
  LINK iPlug2_Core
)
if (OS_LINUX)
  iplug2_target_add(iPlug2_VST2 INTERFACE
    DEFINE "SMTG_OS_LINUX"
  )
  # CMake doesn't like __cdecl, so instead of having people modify their aeffect.h
  # file, just redefine __cdecl.
  if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
    iplug2_target_add(iPlug2_VST2 INTERFACE DEFINE "__cdecl=__attribute__((__cdecl__))")
  endif()
endif()

function(iplug2_configure_vst2 target)
  if (WIN32)
    set_target_properties(${target} PROPERTIES
      OUTPUT_NAME "${IPLUG_APP_NAME}"
      LIBRARY_OUTPUT_DIRECTORY "${target}"
      PREFIX ""
      SUFFIX ".dll"
    )
    set(res_dir "${CMAKE_BINARY_DIR}/${target}/resources")

    # After building, we run the post-build script
    add_custom_command(TARGET ${target} POST_BUILD 
      COMMAND "${CMAKE_BINARY_DIR}/postbuild-win.bat" 
      ARGS "\"$<TARGET_FILE:${target}>\"" "\".dll\""
    )
    
  elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    # TODO Add MacOS build

    set(res_dir "")

  elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set_target_properties(${target} PROPERTIES
      OUTPUT_NAME "${IPLUG_APP_NAME}"
      LIBRARY_OUTPUT_DIRECTORY "${target}"
      PREFIX ""
      SUFFIX ".so"
    )
    set(res_dir "${CMAKE_BINARY_DIR}/${target}/resources")
  endif()

  # Handle resources
  if (res_dir)
    iplug2_target_bundle_resources(${target} "${res_dir}")
  endif()

endfunction()