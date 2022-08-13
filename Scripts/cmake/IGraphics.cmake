#  ==============================================================================
#  
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

cmake_minimum_required(VERSION 3.11)

# IGraphics Core Configuration
add_library(iPlug2_IGraphicsCore INTERFACE)

set(_def "IPLUG_EDITOR=1")
set(_lib "")
set(_inc
  ${IGRAPHICS_SRC}
  ${IGRAPHICS_SRC}/Controls
  ${IGRAPHICS_SRC}/Drawing
  ${IGRAPHICS_SRC}/Platforms
  ${IGRAPHICS_SRC}/Extras
  ${IGRAPHICS_DEPS}/MetalNanoVG/src
  ${IGRAPHICS_DEPS}/NanoSVG/src
  ${IGRAPHICS_DEPS}/NanoVG/src
  ${IGRAPHICS_DEPS}/STB
  ${IGRAPHICS_DEPS}/yoga
  ${IGRAPHICS_DEPS}/yoga/yoga
  ${IPLUG2_DIR}/Dependencies/Build/src
  ${IPLUG2_DIR}/Dependencies/Build/src/freetype/include
)

set(_src
  ${IGRAPHICS_SRC}/IControl.cpp
  ${IGRAPHICS_SRC}/IGraphics.cpp
  ${IGRAPHICS_SRC}/IGraphicsEditorDelegate.cpp
  ${IGRAPHICS_SRC}/Controls/IControls.cpp
  ${IGRAPHICS_SRC}/Controls/IPopupMenuControl.cpp
  ${IGRAPHICS_SRC}/Controls/ITextEntryControl.cpp
)

# Platform-specific configurations
if(WIN32)
  list(APPEND _src ${IGRAPHICS_SRC}/Platforms/IGraphicsWin.cpp)
elseif(APPLE)
  list(APPEND _src 
    ${IGRAPHICS_SRC}/Platforms/IGraphicsMac.mm
    ${IGRAPHICS_SRC}/Platforms/IGraphicsMac_view.mm
    ${IGRAPHICS_SRC}/Platforms/IGraphicsCoreText.mm
  )
  list(APPEND _inc ${WDL_DIR}/swell)
  list(APPEND _lib
    "-framework Cocoa" "-framework Carbon" "-framework Metal" "-framework MetalKit"
    "-framework QuartzCore" "-framework OpenGL" "-framework Accelerate"
  )
  list(APPEND _opts "-Wno-deprecated-declarations")
else()
  message(FATAL_ERROR "Unsupported system ${CMAKE_SYSTEM_NAME}")
endif()

# Configure IGraphics Core
source_group(TREE ${IPLUG2_DIR} PREFIX "IPlug" FILES ${_src})
iplug_target_add(iPlug2_IGraphicsCore INTERFACE 
  DEFINE ${_def} 
  INCLUDE ${_inc} 
  SOURCE ${_src} 
  LINK ${_lib}
)

# No Graphics Configuration
add_library(iPlug2_NoGraphics INTERFACE)
iplug_target_add(iPlug2_NoGraphics INTERFACE DEFINE "NO_IGRAPHICS=1")

# OpenGL Dependencies
add_library(iPlug2_GL2 INTERFACE)
iplug_target_add(iPlug2_GL2 INTERFACE
  INCLUDE 
    ${IGRAPHICS_DEPS}/glad_GL2/include ${IGRAPHICS_DEPS}/glad_GL2/src
  DEFINE "IGRAPHICS_GL2"
)
iplug_source_tree(iPlug2_GL2)

add_library(iPlug2_GL3 INTERFACE)
iplug_target_add(iPlug2_GL3 INTERFACE
  INCLUDE
    ${IGRAPHICS_DEPS}/glad_GL3/include ${IGRAPHICS_DEPS}/glad_GL3/src
  DEFINE "IGRAPHICS_GL3"
)
iplug_source_tree(iPlug2_GL3)

# Metal configuration for Apple platforms
if(APPLE)
  add_library(iPlug2_MTL INTERFACE)
  iplug_target_add(iPlug2_MTL INTERFACE
    DEFINE "IGRAPHICS_METAL"
  )
  iplug_source_tree(iPlug2_MTL)
endif()

# NanoVG Configuration
add_library(iPlug2_NanoVG INTERFACE)
iplug_target_add(iPlug2_NanoVG INTERFACE
  DEFINE "IGRAPHICS_NANOVG"
  LINK iPlug2_IGraphicsCore
)

if(APPLE)
  set(_src ${IGRAPHICS_SRC}/Drawing/IGraphicsNanoVG_src.m)
  iplug_target_add(iPlug2_NanoVG INTERFACE SOURCE ${_src})
  set_property(SOURCE ${_src} APPEND_STRING PROPERTY COMPILE_FLAGS "-fobjc-arc")
endif()
iplug_source_tree(iPlug2_NanoVG)

# NanoVG with OpenGL
add_library(iPlug2_NanoVG_GL2 INTERFACE)
target_link_libraries(iPlug2_NanoVG_GL2 INTERFACE iPlug2_NanoVG iPlug2_GL2)

add_library(iPlug2_NanoVG_GL3 INTERFACE)
target_link_libraries(iPlug2_NanoVG_GL3 INTERFACE iPlug2_NanoVG iPlug2_GL3)

# NanoVG with Metal (Apple only)
if(APPLE)
  add_library(iPlug2_NanoVG_MTL INTERFACE)
  target_link_libraries(iPlug2_NanoVG_MTL INTERFACE iPlug2_NanoVG iPlug2_MTL)
endif()

# Skia Configuration (if requested)
if(Skia IN_LIST iPlug2_FIND_COMPONENTS)
  add_library(iPlug2_Skia INTERFACE)
  iplug_target_add(iPlug2_Skia INTERFACE 
    DEFINE "IGRAPHICS_SKIA"
    LINK iPlug2_IGraphicsCore
  )

  # Platform-specific Skia configurations
  if(WIN32)
    set(sdk "${BUILD_DEPS}/win/${PROCESSOR_ARCH}/$<IF:$<CONFIG:DEBUG>,Debug,Release>")
    iplug_target_add(iPlug2_Skia INTERFACE
      LINK
        "${sdk}/libpng.lib"
        "${sdk}/skia.lib"
        "${sdk}/skottie.lib"
        "${sdk}/skparagraph.lib"
        "${sdk}/sksg.lib"
        "${sdk}/skshaper.lib"
        "${sdk}/skunicode.lib"
        "${sdk}/svg.lib"
        "${sdk}/zlib.lib"
    )
  elseif(APPLE)
    set(sdk "${BUILD_DEPS}/mac/lib")
    iplug_target_add(iPlug2_Skia INTERFACE
      LINK
        "${sdk}/libskia.a"
        "${sdk}/libskottie.a"
        "${sdk}/libskparagraph.a"
        "${sdk}/libsksg.a"
        "${sdk}/libskshaper.a"
        "${sdk}/libskunicode.a"
        "${sdk}/libsvg.a"
    )
  endif()

  # Common Skia include directories
  iplug_target_add(iPlug2_Skia INTERFACE
    INCLUDE
      ${BUILD_DEPS}/src/skia/
      ${BUILD_DEPS}/src/skia/include/core
      ${BUILD_DEPS}/src/skia/include/config
      ${BUILD_DEPS}/src/skia/include/effects
      ${BUILD_DEPS}/src/skia/include/gpu
      ${BUILD_DEPS}/src/skia/include/utils
      ${BUILD_DEPS}/src/skia/include/utils/mac
      ${BUILD_DEPS}/src/skia/modules/svg
      ${BUILD_DEPS}/src/skia/modules/svg/include
  )

  # Skia with OpenGL
  add_library(iPlug2_Skia_GL2 INTERFACE)
  target_link_libraries(iPlug2_Skia_GL2 INTERFACE iPlug2_Skia iPlug2_GL2)

  add_library(iPlug2_Skia_GL3 INTERFACE)
  target_link_libraries(iPlug2_Skia_GL3 INTERFACE iPlug2_Skia iPlug2_GL3)

  # Skia with Metal (Apple only)
  if(APPLE)
    add_library(iPlug2_Skia_MTL INTERFACE)
    target_link_libraries(iPlug2_Skia_MTL INTERFACE iPlug2_Skia iPlug2_MTL)
  endif()

  # Skia CPU-only configuration
  add_library(iPlug2_Skia_CPU INTERFACE)
  iplug_target_add(iPlug2_Skia_CPU INTERFACE DEFINE "IGRAPHICS_CPU" LINK iPlug2_Skia)
endif()