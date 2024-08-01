#  ==============================================================================
#  
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# IGraphics configuration for iPlug2

include(${CMAKE_CURRENT_LIST_DIR}/IPlug.cmake)

set(IGRAPHICS_DEPS_DIR ${DEPS_DIR}/IGraphics)

if(NOT TARGET iPlug2::IGraphics)
  add_library(iPlug2::IGraphics INTERFACE IMPORTED)
  
  set(IGRAPHICS_DIR ${IPLUG2_DIR}/IGraphics)
  
  set(IGRAPHICS_SRC
    ${IGRAPHICS_DIR}/IControl.cpp
    ${IGRAPHICS_DIR}/IGraphics.cpp
    ${IGRAPHICS_DIR}/IGraphicsEditorDelegate.cpp
    # ${IGRAPHICS_DIR}/Controls/IControls.cpp
    # special IControls
    # ${IGRAPHICS_DIR}/Controls/IPopupMenuControl.cpp
    # ${IGRAPHICS_DIR}/Controls/ITextEntryControl.cpp
    # ${IGRAPHICS_DIR}/Drawing/IGraphicsNanoVG.cpp
    # ${IGRAPHICS_DEPS_DIR}/NanoVG/src/nanovg.c
  )

  if(WIN32)
    list(APPEND IGRAPHICS_SRC ${IGRAPHICS_DIR}/Platforms/IGraphicsWin.cpp)
  elseif(APPLE)
    list(APPEND IGRAPHICS_SRC 
      ${IGRAPHICS_DIR}/Platforms/IGraphicsMac.mm
      ${IGRAPHICS_DIR}/Platforms/IGraphicsMac_view.mm
      ${IGRAPHICS_DIR}/Platforms/IGraphicsCoreText.mm
    )
  elseif(UNIX AND NOT APPLE)
    message("Error - Linux not yet supported")
  endif()

  target_sources(iPlug2::IGraphics INTERFACE ${IGRAPHICS_SRC})
  
  target_include_directories(iPlug2::IGraphics INTERFACE 
    ${IGRAPHICS_DIR}
    ${IGRAPHICS_DIR}/Controls
    ${IGRAPHICS_DIR}/Drawing
    ${IGRAPHICS_DIR}/Platforms
    ${IGRAPHICS_DIR}/Extras
    ${IGRAPHICS_DEPS_DIR}/STB
  )
  
  if(WIN32)
    target_compile_definitions(iPlug2::IGraphics INTERFACE NOMINMAX)
  elseif(APPLE)
    target_link_libraries(iPlug2::IGraphics INTERFACE
      "-framework Cocoa"
      "-framework Carbon" 
      "-framework Accelerate"
      "-framework QuartzCore"
    )
  elseif(UNIX AND NOT APPLE)
    message("Error - Linux not yet supported")
  endif()
  
  target_link_libraries(iPlug2::IGraphics INTERFACE iPlug2::IPlug)
endif()

# NanoVG configurations
if(NOT TARGET iPlug2::IGraphics::NanoVG)
  add_library(iPlug2::IGraphics::NanoVG INTERFACE IMPORTED)
  target_include_directories(iPlug2::IGraphics INTERFACE 
    ${IGRAPHICS_DEPS_DIR}/NanoVG
    ${IGRAPHICS_DEPS_DIR}/NanoSVG/src
    # ${IGRAPHICS_DEPS_DIR}/freetype/include
    )
  target_link_libraries(iPlug2::IGraphics::NanoVG INTERFACE iPlug2::IGraphics)
  target_compile_definitions(iPlug2::IGraphics::NanoVG INTERFACE IGRAPHICS_NANOVG)
endif()

# OpenGL configurations
if(NOT TARGET iPlug2::IGraphics::GL2)
  add_library(iPlug2::IGraphics::GL2 INTERFACE IMPORTED)
  target_include_directories(iPlug2::IGraphics::GL2 INTERFACE 
    ${IGRAPHICS_DEPS_DIR}/glad_GL2/include 
    ${IGRAPHICS_DEPS_DIR}/glad_GL2/src
  )
  target_compile_definitions(iPlug2::IGraphics::GL2 INTERFACE IGRAPHICS_GL2)
endif()

if(NOT TARGET iPlug2::IGraphics::GL3)
  add_library(iPlug2::IGraphics::GL3 INTERFACE IMPORTED)
  target_include_directories(iPlug2::IGraphics::GL3 INTERFACE 
    ${IGRAPHICS_DEPS_DIR}/glad_GL3/include 
    ${IGRAPHICS_DEPS_DIR}/glad_GL3/src
  )
  target_compile_definitions(iPlug2::IGraphics::GL3 INTERFACE IGRAPHICS_GL3)
endif()

# Metal configuration for Apple platforms
if(APPLE)
  if(NOT TARGET iPlug2::IGraphics::Metal)
    add_library(iPlug2::IGraphics::Metal INTERFACE IMPORTED)
    target_compile_definitions(iPlug2::IGraphics::Metal INTERFACE IGRAPHICS_METAL)
  endif()
endif()

# Skia configuration (if requested)
if(Skia IN_LIST IPLUG2_FIND_COMPONENTS)
  if(NOT TARGET iPlug2::IGraphics::Skia)
    add_library(iPlug2::IGraphics::Skia INTERFACE IMPORTED)
    target_link_libraries(iPlug2::IGraphics::Skia INTERFACE iPlug2::IGraphics)
    target_compile_definitions(iPlug2::IGraphics::Skia INTERFACE IGRAPHICS_SKIA)
    
    set(SKIA_PATH ${IGRAPHICS_DEPS_DIR}/skia)
    target_include_directories(iPlug2::IGraphics::Skia INTERFACE 
      ${SKIA_PATH}
      ${SKIA_PATH}/include/core
      ${SKIA_PATH}/include/effects
      ${SKIA_PATH}/include/config
      ${SKIA_PATH}/include/utils
      ${SKIA_PATH}/include/utils/mac
      ${SKIA_PATH}/include/gpu
      ${SKIA_PATH}/modules/svglib
      ${SKIA_PATH}/include/private
      ${SKIA_PATH}/include/third_party/skcms
      ${SKIA_PATH}/third_party/externals/icu/source/common
    )
    
    if(WIN32)
      set(SKIA_LIB_PATH ${DEPS_DIR}/Build/src/skia/out/Release-x64)
      target_link_libraries(iPlug2::IGraphics::Skia INTERFACE
        ${SKIA_LIB_PATH}/skia.lib
        ${SKIA_LIB_PATH}/svg.lib
      )
    elseif(APPLE)
      set(SKIA_LIB_PATH ${DEPS_DIR}/Build/src/skia/out/Release)
      target_link_libraries(iPlug2::IGraphics::Skia INTERFACE
        ${SKIA_LIB_PATH}/libskia.a
        ${SKIA_LIB_PATH}/libsvg.a
      )
    endif()
  endif()
endif()
