#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# WasmUI.cmake - Wasm Web UI module configuration for iPlug2
# Creates the UI/IGraphics WASM module for the wasm split architecture
#
# The UI module runs on the main thread with:
# - Asynchronous WASM loading
# - Full IGraphics support with NanoVG/WebGL
# - IndexedDB filesystem for persistent storage

include(${CMAKE_CURRENT_LIST_DIR}/IPlug.cmake)

if(NOT TARGET iPlug2::WasmUI)
  # Only create target when building with Emscripten
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    message(STATUS "WasmUI target requires Emscripten toolchain - skipping")
    return()
  endif()

  add_library(iPlug2::WasmUI INTERFACE IMPORTED)

  set(IGRAPHICS_DIR ${IPLUG2_DIR}/IGraphics)
  set(IGRAPHICS_DEPS_DIR ${DEPS_DIR}/IGraphics)
  set(IPLUG_WEB_DIR ${IPLUG_DIR}/WEB)

  # WasmUI sources (IGraphics + IPlugWasmUI)
  # Note: IGraphicsWeb.cpp #includes IGraphicsNanoVG.cpp (unity build style)
  # so we do NOT compile IGraphicsNanoVG.cpp separately
  set(WASM_UI_SRC
    # IPlug Wasm UI API
    ${IPLUG_WEB_DIR}/IPlugWasmUI.cpp
    # IGraphics core
    ${IGRAPHICS_DIR}/IGraphics.cpp
    ${IGRAPHICS_DIR}/IControl.cpp
    ${IGRAPHICS_DIR}/IGraphicsEditorDelegate.cpp
    # IGraphics controls
    ${IGRAPHICS_DIR}/Controls/IControls.cpp
    ${IGRAPHICS_DIR}/Controls/IPopupMenuControl.cpp
    ${IGRAPHICS_DIR}/Controls/ITextEntryControl.cpp
    # IGraphics Web platform (includes IGraphicsNanoVG.cpp)
    ${IGRAPHICS_DIR}/Platforms/IGraphicsWeb.cpp
  )

  target_sources(iPlug2::WasmUI INTERFACE ${WASM_UI_SRC})

  target_include_directories(iPlug2::WasmUI INTERFACE
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

  target_compile_definitions(iPlug2::WasmUI INTERFACE
    WASM_UI_API
    IPLUG_EDITOR=1
    IGRAPHICS_NANOVG
    IGRAPHICS_GLES2
    WDL_NO_DEFINE_MINMAX
    NDEBUG=1
  )

  # UI module exported functions
  set(WASM_UI_EXPORTS "'_malloc','_free','_main','_iplug_fsready','_iplug_syncfs'")

  # Emscripten link flags for WasmUI
  # - BINARYEN_ASYNC_COMPILATION=1: Async compilation (can run on main thread)
  # - FORCE_FILESYSTEM=1: Enable IndexedDB filesystem
  # - USE_WEBGL2=0, FULL_ES3=1: WebGL/GLES configuration for NanoVG
  target_link_options(iPlug2::WasmUI INTERFACE
    "SHELL:-s ALLOW_MEMORY_GROWTH=1"
    "--bind"
    "SHELL:-s EXPORTED_FUNCTIONS=[${WASM_UI_EXPORTS}]"
    "SHELL:-s EXPORTED_RUNTIME_METHODS=['ccall','UTF8ToString','HEAPU8']"
    "SHELL:-s BINARYEN_ASYNC_COMPILATION=1"
    "SHELL:-s FORCE_FILESYSTEM=1"
    "SHELL:-s ENVIRONMENT=web"
    "SHELL:-s DEFAULT_LIBRARY_FUNCS_TO_INCLUDE=['\$Browser']"
    "-lidbfs.js"
    "SHELL:-s ASSERTIONS=0"
    # NanoVG WebGL settings
    "SHELL:-s USE_WEBGL2=0"
    "SHELL:-s FULL_ES3=1"
  )

  target_compile_options(iPlug2::WasmUI INTERFACE
    -Wno-bitwise-op-parentheses
    -Wno-deprecated-declarations
    # Force-include GLES2 header so GL types are defined before NanoVG
    -include GLES2/gl2.h
  )

  target_link_libraries(iPlug2::WasmUI INTERFACE iPlug2::IPlug)
endif()

# Configuration function for WasmUI targets
function(iplug_configure_wasm_ui target project_name)
  target_link_libraries(${target} PUBLIC iPlug2::WasmUI)

  set_target_properties(${target} PROPERTIES
    OUTPUT_NAME "${project_name}-ui"
    SUFFIX ".js"
  )
endfunction()
