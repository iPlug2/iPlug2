cmake_minimum_required(VERSION 3.11)

# Determine VST2 and VST3 directories
find_file(VST2_32_PATH
  "VstPlugins"
  PATHS "C:/Program Files (x86)"
  DOC "Path to install 32-bit VST2 plugins"
)
find_file(VST2_64_PATH
  NAMES "VstPlugins" "VST"
  PATHS "C:/Program Files" "$ENV{HOME}/Library/Audio/Plug-Ins" 
  DOC "Path to install 64-bit VST2 plugins"
)

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