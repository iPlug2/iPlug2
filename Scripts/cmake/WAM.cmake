#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# WAM (Web Audio Module) processor target configuration for iPlug2
# This creates the DSP/audio worklet WASM module

include(${CMAKE_CURRENT_LIST_DIR}/IPlug.cmake)

if(NOT TARGET iPlug2::WAM)
  # Only create WAM target when building with Emscripten
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    message(STATUS "WAM target requires Emscripten toolchain - skipping")
    return()
  endif()

  add_library(iPlug2::WAM INTERFACE IMPORTED)

  # WAM SDK path
  set(WAM_SDK_DIR ${IPLUG_DEPS_DIR}/WAM_SDK/wamsdk)
  set(IPLUG_WEB_DIR ${IPLUG_DIR}/WEB)

  # Check WAM SDK exists
  if(NOT EXISTS ${WAM_SDK_DIR})
    message(WARNING "WAM_SDK not found at ${WAM_SDK_DIR}. WAM target will not be available.")
    return()
  endif()

  # WAM processor sources
  set(WAM_SRC
    ${IPLUG_WEB_DIR}/IPlugWAM.cpp
    ${WAM_SDK_DIR}/processor.cpp
    ${IPLUG_DIR}/IPlugProcessor.cpp
  )

  target_sources(iPlug2::WAM INTERFACE ${WAM_SRC})

  set(IGRAPHICS_DIR ${IPLUG2_DIR}/IGraphics)
  set(IGRAPHICS_DEPS_DIR ${DEPS_DIR}/IGraphics)

  # Include IGraphics paths so plugin sources can find headers even with NO_IGRAPHICS
  # (the headers need to exist for includes, but NO_IGRAPHICS prevents compilation)
  target_include_directories(iPlug2::WAM INTERFACE
    ${IPLUG_WEB_DIR}
    ${WAM_SDK_DIR}
    ${IGRAPHICS_DIR}
    ${IGRAPHICS_DIR}/Controls
    ${IGRAPHICS_DIR}/Platforms
    ${IGRAPHICS_DIR}/Drawing
    ${IGRAPHICS_DIR}/Extras
    ${IGRAPHICS_DEPS_DIR}/NanoVG/src
    ${IGRAPHICS_DEPS_DIR}/NanoSVG/src
    ${IGRAPHICS_DEPS_DIR}/STB
    ${IGRAPHICS_DEPS_DIR}/yoga
    ${IGRAPHICS_DEPS_DIR}/yoga/yoga
  )

  target_compile_definitions(iPlug2::WAM INTERFACE
    WAM_API
    IPLUG_DSP=1
    NO_IGRAPHICS
    SAMPLE_TYPE_FLOAT
    WDL_NO_DEFINE_MINMAX
    NDEBUG=1
  )

  # WAM processor exported functions
  set(WAM_EXPORTS "'_malloc','_free','_createModule','_wam_init','_wam_terminate','_wam_resize','_wam_onprocess','_wam_onmidi','_wam_onsysex','_wam_onparam','_wam_onmessageN','_wam_onmessageS','_wam_onmessageA','_wam_onpatch'")

  # Emscripten link flags for WAM processor
  # - SINGLE_FILE=1: Embed WASM as BASE64 in JS (required for AudioWorklet)
  # - BINARYEN_ASYNC_COMPILATION=0: Sync compilation (required for worklet scope)
  # - EXPORT_NAME: Factory function name for the module
  target_link_options(iPlug2::WAM INTERFACE
    "SHELL:-s ALLOW_MEMORY_GROWTH=1"
    "--bind"
    "SHELL:-s EXPORTED_FUNCTIONS=[${WAM_EXPORTS}]"
    "SHELL:-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','setValue','UTF8ToString']"
    "SHELL:-s BINARYEN_ASYNC_COMPILATION=0"
    "SHELL:-s SINGLE_FILE=1"
    "SHELL:-s EXPORT_NAME='ModuleFactory'"
    "SHELL:-s ASSERTIONS=0"
    "--pre-js=${IPLUG2_DIR}/IPlug/WEB/Template/scripts/atob-polyfill.js"
  )

  target_compile_options(iPlug2::WAM INTERFACE
    -Wno-bitwise-op-parentheses
  )

  target_link_libraries(iPlug2::WAM INTERFACE iPlug2::IPlug)
endif()

# Configuration function for WAM targets
function(iplug_configure_wam target project_name)
  target_link_libraries(${target} PUBLIC iPlug2::WAM)

  set_target_properties(${target} PROPERTIES
    OUTPUT_NAME "${project_name}-wam"
    SUFFIX ".js"
  )
endfunction()
