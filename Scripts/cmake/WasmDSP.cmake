#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# WasmDSP.cmake - Wasm Web DSP module configuration for iPlug2
# Creates the DSP/AudioWorklet WASM module for the wasm split architecture
#
# The DSP module runs in an AudioWorklet with:
# - Synchronous WASM loading (SINGLE_FILE=1 embeds as BASE64)
# - No graphics (NO_IGRAPHICS)
# - SIMD support for audio processing

include(${CMAKE_CURRENT_LIST_DIR}/IPlug.cmake)

if(NOT TARGET iPlug2::WasmDSP)
  # Only create target when building with Emscripten
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    message(STATUS "WasmDSP target requires Emscripten toolchain - skipping")
    return()
  endif()

  add_library(iPlug2::WasmDSP INTERFACE IMPORTED)

  set(IPLUG_WEB_DIR ${IPLUG_DIR}/WEB)

  # WasmDSP sources
  set(WASM_DSP_SRC
    ${IPLUG_WEB_DIR}/IPlugWasmDSP.cpp
    ${IPLUG_DIR}/IPlugProcessor.cpp
  )

  target_sources(iPlug2::WasmDSP INTERFACE ${WASM_DSP_SRC})

  set(IGRAPHICS_DIR ${IPLUG2_DIR}/IGraphics)
  set(IGRAPHICS_DEPS_DIR ${DEPS_DIR}/IGraphics)

  # Include IGraphics paths so plugin sources can find headers even with NO_IGRAPHICS
  target_include_directories(iPlug2::WasmDSP INTERFACE
    ${IPLUG_WEB_DIR}
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

  target_compile_definitions(iPlug2::WasmDSP INTERFACE
    WASM_DSP_API
    IPLUG_DSP=1
    NO_IGRAPHICS
    SAMPLE_TYPE_FLOAT
    WDL_NO_DEFINE_MINMAX
    NDEBUG=1
  )

  # Minimal exports - JS calls via emscripten bindings
  set(WASM_DSP_EXPORTS "'_malloc','_free'")

  # Emscripten link flags for WasmDSP
  # CRITICAL: SINGLE_FILE=1 embeds WASM as BASE64 for synchronous loading in AudioWorklet
  # CRITICAL: BINARYEN_ASYNC_COMPILATION=0 for synchronous compilation in AudioWorklet scope
  # NOTE: ENVIRONMENT=web,worker,shell needed for AudioWorklet
  target_link_options(iPlug2::WasmDSP INTERFACE
    "SHELL:-s ALLOW_MEMORY_GROWTH=1"
    "--bind"
    "SHELL:-s EXPORTED_FUNCTIONS=[${WASM_DSP_EXPORTS}]"
    "SHELL:-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','setValue','UTF8ToString','HEAPF32','HEAPU8']"
    "SHELL:-s BINARYEN_ASYNC_COMPILATION=0"
    "SHELL:-s SINGLE_FILE=1"
    "SHELL:-s ENVIRONMENT=web,worker,shell"
    "SHELL:-s ASSERTIONS=0"
    "-msimd128"
    "--pre-js=${IPLUG2_DIR}/IPlug/WEB/Template/scripts/atob-polyfill.js"
  )

  target_compile_options(iPlug2::WasmDSP INTERFACE
    -Wno-bitwise-op-parentheses
    -Wno-deprecated-declarations
  )

  target_link_libraries(iPlug2::WasmDSP INTERFACE iPlug2::IPlug)
endif()

# Configuration function for WasmDSP targets
function(iplug_configure_wasm_dsp target project_name)
  target_link_libraries(${target} PUBLIC iPlug2::WasmDSP)

  set_target_properties(${target} PROPERTIES
    OUTPUT_NAME "${project_name}-dsp"
    SUFFIX ".js"
  )
endfunction()
