cmake_minimum_required(VERSION 3.14)
project(iPlug2Examples)

# IGraphics backend selection
set(IGRAPHICS_BACKEND "NANOVG" CACHE STRING "IGraphics drawing backend")
set_property(CACHE IGRAPHICS_BACKEND PROPERTY STRINGS "NANOVG" "SKIA")

# IGraphics renderer selection (platform-aware defaults)
# NANOVG supports: GL2, GL3, METAL (Metal is macOS/iOS only)
# SKIA supports: GL3, METAL, CPU
if(WIN32)
  set(DEFAULT_RENDERER "GL2")
elseif(APPLE)
  set(DEFAULT_RENDERER "METAL")
else()
  set(DEFAULT_RENDERER "GL2")
endif()
set(IGRAPHICS_RENDERER "${DEFAULT_RENDERER}" CACHE STRING "IGraphics renderer")
set_property(CACHE IGRAPHICS_RENDERER PROPERTY STRINGS "GL2" "GL3" "METAL" "CPU")

# Construct the IGraphics library target based on selections
if(IGRAPHICS_BACKEND STREQUAL "SKIA")
  if(IGRAPHICS_RENDERER STREQUAL "CPU")
    set(IGRAPHICS_LIB iPlug2::IGraphics::Skia::CPU)
  elseif(IGRAPHICS_RENDERER STREQUAL "GL3")
    set(IGRAPHICS_LIB iPlug2::IGraphics::Skia::GL3)
  else()
    set(IGRAPHICS_LIB iPlug2::IGraphics::Skia::Metal)
  endif()
else()
  # NanoVG
  if(IGRAPHICS_RENDERER STREQUAL "GL3")
    set(IGRAPHICS_LIB iPlug2::IGraphics::NanoVG::GL3)
  elseif(IGRAPHICS_RENDERER STREQUAL "METAL")
    set(IGRAPHICS_LIB iPlug2::IGraphics::NanoVG::Metal)
  else()
    # Default to GL2 for NanoVG
    set(IGRAPHICS_LIB iPlug2::IGraphics::NanoVG)
  endif()
endif()
message(STATUS "IGraphics: ${IGRAPHICS_BACKEND}/${IGRAPHICS_RENDERER} -> ${IGRAPHICS_LIB}")

add_subdirectory(IPlugEffect)
add_subdirectory(IPlugInstrument)
add_subdirectory(IPlugControls)
add_subdirectory(IPlugConvoEngine)
add_subdirectory(IPlugWebUI)
add_subdirectory(IPlugSwiftUI)
