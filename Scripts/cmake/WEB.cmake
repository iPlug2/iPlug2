cmake_minimum_required(VERSION 3.11)
find_package(Emscripten REQUIRED)

set(sdk ${IPLUG2_DIR}/IPlug/WEB)
set(WAM_SDK_PATH ${IPLUG_DEPS}/IPlug/WAM_SDK)
set(WAM_AWP_PATH ${IPLUG_DEPS}/IPlug/WAM_AWP)

set(_opt "-Wno-bitwise-op-parentheses")
set(_def "WDL_NO_DEFINE_MINMAX")
set(_ldflags
  "$<$<CONFIG:DEBUG>:-g4 -O0>"
  "$<$<CONFIG:RELEASE>:SHELL:-s ASSERTIONS=0>")

set(WEB_EXPORTS "['_main', '_iplug_fsready', '_iplug_syncfs']")

set(WAM_EXPORTS "[\
  '_createModule','_wam_init','_wam_terminate','_wam_resize', \
  '_wam_onprocess', '_wam_onmidi', '_wam_onsysex', '_wam_onparam', \
  '_wam_onmessageN', '_wam_onmessageS', '_wam_onmessageA', '_wam_onpatch']")

add_library(IPLUG_WEB INTERFACE)
iplug_target_add(IPLUG_WEB INTERFACE
  DEFINE "WEB_API" "IPLUG_EDITOR=1" ${_def}
  OPTION ${_opt}
  SOURCE
    ${sdk}/IPlugWeb.h
    ${sdk}/IPlugWeb.cpp
  LINK IPLUG_CORE
  )
target_link_options(IPLUG_WEB INTERFACE
  "SHELL:-s EXPORTED_FUNCTIONS=${WEB_EXPORTS}"
  "SHELL:-s EXTRA_EXPORTED_RUNTIME_METHODS=\"['UTF8ToString']\""
  "SHELL:-s BINARYEN_ASYNC_COMPILATION=1"
  "SHELL:-s FORCE_FILESYSTEM=1"
  "SHELL:-s ENVIRONMENT=web"
  "-lidbfd.js"
  ${_ldflags})

add_library(IPLUG_WAM INTERFACE)
iplug_target_add(IPLUG_WAM INTERFACE
  DEFINE "WAM_API" "IPLUG_DSP=1" "NO_IGRAPHICS" "SAMPLE_TYPE_FLOAT" ${_def}
  OPTION ${_opt}
  SOURCE
    ${sdk}/IPlugWAM.h
    ${sdk}/IPlugWAM.cpp
    ${WAM_SDK_PATH}/processor.h
    ${WAM_SDK_PATH}/processor.cpp
  LINK IPLUG_CORE
  )
target_link_options(IPLUG_WAM INTERFACE
  "SHELL:-s EXTRA_EXPORTED_RUNTIME_METHODS=\"['ccall', 'cwrap', 'setValue', 'UTF8ToString']\""
  "SHELL:-s BINARYEN_ASYNC_COMPILATION=0"
  "SHELL:-s SINGLE_FILE=1"
  "SHELL:-s EXPORT_NAME=\"'AudioWorkletGlobalScope.WAM.${IPLUG_APP_NAME}'\""
  ${_ldflags})

add_library(IPLUG_CANVAS INTERFACE)
iplug_target_add(IPLUG_CANVAS INTERFACE
  DEFINE "IGRAPHICS_CANVAS")

function(iplug_configure_web target)
  message("WEB not yet implemented" FATAL_ERROR)
endfunction()

function(iplug_configure_wam)
  message("WAM not yet implemented" FATAL_ERROR)
endfunction(iplug_configure_wam)
