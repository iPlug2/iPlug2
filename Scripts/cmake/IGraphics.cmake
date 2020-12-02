cmake_minimum_required(VERSION 3.11)


##################
# IGraphics Core #
##################

add_library(iPlug2_IGraphicsCore INTERFACE)
set(_def "IPLUG_EDITOR=1")
set(_lib "")
set(_inc
  # IGraphics
  ${IGRAPHICS_SRC}
  ${IGRAPHICS_SRC}/Controls
  ${IGRAPHICS_SRC}/Drawing
  ${IGRAPHICS_SRC}/Platforms
  ${IGRAPHICS_SRC}/Extras
  ${IGRAPHICS_DEPS}/NanoSVG/src
  ${IGRAPHICS_DEPS}/NanoVG/src
  ${IGRAPHICS_DEPS}/STB
  ${IGRAPHICS_DEPS}/imgui
  ${IGRAPHICS_DEPS}/imgui/examples
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
  ${IPLUG_SRC}/IPlugTaskThread.h
  ${IPLUG_SRC}/IPlugTaskThread.cpp
)

# Platform Settings
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
  list(APPEND _src ${IGRAPHICS_SRC}/Platforms/IGraphicsWin.cpp)

elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
  list(APPEND _src
    ${IGRAPHICS_SRC}/Platforms/IGraphicsLinux.cpp
    ${IGRAPHICS_DEPS}/xcbt/xcbt.c
  )
  list(APPEND _inc
    ${IGRAPHICS_DEPS}/xcbt
  )
  list(APPEND _lib "xcb" "xcb-xfixes" "xcb-icccm" "dl" "fontconfig" "freetype")
  set_property(SOURCE ${IGRAPHICS_DEPS}/xcbt/xcbt.c PROPERTY LANGUAGE C)

elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
  list(APPEND _src 
    ${IPLUG_SRC}/IPlugPaths.mm
    ${IGRAPHICS_SRC}/Platforms/IGraphicsMac.mm
    ${IGRAPHICS_SRC}/Platforms/IGraphicsMac_view.mm
    ${IGRAPHICS_SRC}/Platforms/IGraphicsCoreText.mm
  )
  list(APPEND _inc ${WDL_DIR}/swell)
  list(APPEND _lib
    "-framework Cocoa" "-framework Carbon" "-framework Metal" "-framework MetalKit" "-framework QuartzCore"
    "-framework OpenGL"
  )
  list(APPEND _opts "-Wno-deprecated-declarations")
else()
  message("Unhandled system ${CMAKE_SYSTEM_NAME}" FATAL_ERROR)
endif()

source_group(TREE ${IPLUG2_DIR} PREFIX "IPlug" FILES ${_src})
iplug_target_add(iPlug2_IGraphicsCore INTERFACE DEFINE ${_def} INCLUDE ${_inc} SOURCE ${_src} LINK ${_lib})


###############
# No Graphics #
###############

add_library(iPlug2_NoGraphics INTERFACE)
iplug_target_add(iPlug2_NoGraphics INTERFACE DEFINE "NO_IGRAPHICS=1")

#######################
# OpenGL Dependencies #
#######################

set(_glx_inc "")
set(_glx_src "")
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(_glx_inc ${IGRAPHICS_DEPS}/glad_GLX/include ${IGRAPHICS_DEPS}/glad_GLX/src)
  set(_glx_src ${IGRAPHICS_DEPS}/glad_GLX/src/glad_glx.c)
endif()

add_library(iPlug2_GL2 INTERFACE)
iplug_target_add(iPlug2_GL2 INTERFACE
  INCLUDE 
    ${IGRAPHICS_DEPS}/glad_GL2/include ${IGRAPHICS_DEPS}/glad_GL2/src ${_glx_inc}
  SOURCE
    ${_glx_src}
  DEFINE "IGRAPHICS_GL2"
)
iplug_source_tree(iPlug2_GL2)

add_library(iPlug2_GL3 INTERFACE)
iplug_target_add(iPlug2_GL3 INTERFACE
  INCLUDE
    ${IGRAPHICS_DEPS}/glad_GL3/include ${IGRAPHICS_DEPS}/glad_GL3/src ${_glx_inc}
  SOURCE
    ${_glx_src}
  DEFINE "IGRAPHICS_GL3"
)
iplug_source_tree(iPlug2_GL3)

##########
# NanoVG #
##########

add_library(iPlug2_NANOVG INTERFACE)
iplug_target_add(iPlug2_NANOVG INTERFACE
  DEFINE "IGRAPHICS_NANOVG"
  LINK iPlug2_IGraphicsCore
)
if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(_src ${IGRAPHICS_DEPS}/NanoVG/src/nanovg.c)
  iplug_target_add(iPlug2_NANOVG INTERFACE SOURCE ${_src})
  set_property(SOURCE ${_src} PROPERTY LANGUAGE C)
endif()
iplug_source_tree(iPlug2_NANOVG)

########
# Skia #
########

if (Skia IN_LIST iPlug2_FIND_COMPONENTS)

  add_library(iPlug2_Skia INTERFACE)
  iplug_target_add(iPlug2_Skia INTERFACE 
    DEFINE "IGRAPHICS_SKIA"
    LINK iPlug2_IGraphicsCore)

  if (WIN32)
    set(sdk "${BUILD_DEPS}/win/${PROCESSOR_ARCH}/$<IF:$<CONFIG:DEBUG>,Debug,Release>")
    iplug_target_add(iPlug2_Skia INTERFACE
      LINK
        "${sdk}/skia.lib"
        "${sdk}/skottie.lib"
        "${sdk}/skparagraph.lib"
        "${sdk}/sksg.lib"
        "${sdk}/skshaper.lib"
        "${sdk}/svg.lib")

  elseif (OS_MAC)
    # TODO MAC: Check if this is the real path
    set(sdk "${IPLUG_DEPS}/../Build/mac/${PROCESSOR_ARCH}/lib")

  elseif (OS_LINUX)
    set(sdk "${IPLUG_DEPS}/../Build/linux/lib")
    iplug_target_add(iPlug2_Skia INTERFACE
      LINK 
        "${sdk}/libskia.a"
        "${sdk}/libskottie.a"
        "${sdk}/libskparagraph.a"
        "${sdk}/libsksg.a"
        "${sdk}/libskshaper.a"
        "${sdk}/libsvg.a")

  endif()

  iplug_target_add(iPlug2_Skia INTERFACE
    INCLUDE
      ${BUILD_DEPS}/src/skia
      ${BUILD_DEPS}/src/skia/include/core
      ${BUILD_DEPS}/src/skia/include/effects
      ${BUILD_DEPS}/src/skia/include/config
      ${BUILD_DEPS}/src/skia/include/utils
      ${BUILD_DEPS}/src/skia/include/gpu
      ${BUILD_DEPS}/src/skia/modules/svg)

  add_library(iPlug2_Skia_GL2 INTERFACE)
  target_link_libraries(iPlug2_Skia_GL2 INTERFACE iPlug2_Skia iPlug2_GL2)

  add_library(iPlug2_Skia_GL3 INTERFACE)
  target_link_libraries(iPlug2_Skia_GL3 INTERFACE iPlug2_Skia iPlug2_GL3)

  add_library(iPlug2_Skia_CPU INTERFACE)
  iplug_target_add(iPlug2_Skia_CPU INTERFACE DEFINE "IGRAPHICS_CPU" LINK iPlug2_Skia)
endif()

########
# LICE #
########

include("${IPLUG2_CMAKE_DIR}/LICE.cmake")

# LICE build is different between APP and all other targets when using swell.
# Link to iPlug2_LICE and we'll fix it in configure.
add_library(iPlug2_LICE INTERFACE)
iplug_target_add(iPlug2_LICE INTERFACE
  DEFINE "IGRAPHICS_LICE" "SWELL_EXTRA_MINIMAL" "SWELL_LICE_GDI" "SWELL_FREETYPE"
  LINK iPlug2_IGraphicsCore LICE_Core LICE_PNG LICE_ZLIB "dl" "pthread"
)

# set(swell_src
#   swell.h
#   swell.cpp
#   swell-dlg-generic.cpp
#   swell-gdi-generic.cpp
#   swell-ini.cpp
#   swell-menu-generic.cpp
#   swell-wnd-generic.cpp
#   swell-gdi-lice.cpp
# )
# list(TRANSFORM swell_src PREPEND "${WDL_DIR}/swell/")

if (OS_LINUX)
  pkg_check_modules(Freetype2 REQUIRED IMPORTED_TARGET "freetype2")
  iplug_target_add(iPlug2_LICE INTERFACE
    INCLUDE 
      ${IGRAPHICS_DEPS}/glad_GL2/include
      ${IGRAPHICS_DEPS}/glad_GL2/src
      ${_glx_inc}
    SOURCE ${_glx_src}
    LINK PkgConfig::Freetype2
  )  
endif()
