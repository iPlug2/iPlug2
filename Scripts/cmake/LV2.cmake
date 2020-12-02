cmake_minimum_required(VERSION 3.11)

set(_doc "Path to install LV2 plugins")
if (WIN32)
  iplug_find_path(LV2_INSTALL_PATH DIR DEFAULT_IDX 0 DOC ${_doc}
    PATHS "$ENV{APPDATA}/LV2" "$ENV{COMMONPROGRAMFILES}/LV2")
elseif (OS_MAC)
  iplug_find_path(LV2_INSTALL_PATH DIR DEFAULT_IDX 0 DOC ${_doc}
    PATHS "$ENV{HOME}/Library/Audio/Plug-Ins/LV2" "/Library/Audio/Plug-Ins/LV2")
elseif (OS_LINUX)
  iplug_find_path(LV2_INSTALL_PATH DIR DEFAULT_IDX 0 DOC ${_doc}
    PATHS "$ENV{HOME}/.lv2" "/usr/local/lib/lv2" "/usr/lib/lv2")
endif()

find_path(LV2_SDK "LV2" PATHS "${IPLUG_DEPS}" DOC "Path to LV2 sdk")

set(_src
  IPlugLV2.h
  IPlugLV2.cpp
  IPlugLV2_cfg.cpp
)
list(TRANSFORM _src PREPEND "${IPLUG_SRC}/LV2/")
add_library(iPlug2_LV2 INTERFACE)
iplug_target_add(iPlug2_LV2 INTERFACE
  DEFINE "LV2_API" "IPLUG_DSP=1"
  INCLUDE "${LV2_SDK}"
  SOURCE ${_src}
  LINK iPlug2_Core
)

function(iplug_configure_lv2 target)
  iplug_target_add(${target} PUBLIC LINK iPlug2_LV2)

  set(out_dir "${CMAKE_BINARY_DIR}/${target}")
  set(install_dir "${LV2_INSTALL_PATH}/${PLUG_NAME}.lv2")

  set_target_properties(${target} PROPERTIES
    OUTPUT_NAME "${IPLUG_APP_NAME}"
    LIBRARY_OUTPUT_DIRECTORY "${out_dir}"
    PREFIX "")

  if (WIN32)
    set_target_properties(${target} PROPERTIES
      SUFFIX ".dll")
    set(res_dir "${CMAKE_BINARY_DIR}/${target}/resources")

  elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set_target_properties(${target} PROPERTIES
      SUFFIX ".dylib")
    set(res_dir "${CMAKE_BINARY_DIR}/${target}/resources")

  elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set_target_properties(${target} PROPERTIES
      SUFFIX ".so")
    set(res_dir "${CMAKE_BINARY_DIR}/${target}/resources")
    
  endif()

  # Handle resources
  if (res_dir)
    iplug_target_bundle_resources(${target} "${res_dir}")
  endif()

  # After building copy to the correct directory
  add_custom_command(TARGET ${target} POST_BUILD 
    COMMAND ${CMAKE_COMMAND} ARGS "-E" "copy_directory" "${out_dir}" "${install_dir}")
endfunction()
