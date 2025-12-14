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
    ${IGRAPHICS_DIR}/Controls/IControls.cpp
    ${IGRAPHICS_DIR}/Controls/IPopupMenuControl.cpp
    ${IGRAPHICS_DIR}/Controls/ITextEntryControl.cpp
  )

  if(WIN32)
    list(APPEND IGRAPHICS_SRC ${IGRAPHICS_DIR}/Platforms/IGraphicsWin.cpp)
  elseif(IOS)
    # iOS-specific platform sources
    list(APPEND IGRAPHICS_SRC
      ${IGRAPHICS_DIR}/Platforms/IGraphicsIOS.mm
      ${IGRAPHICS_DIR}/Platforms/IGraphicsIOS_view.mm
      ${IGRAPHICS_DIR}/Platforms/IGraphicsCoreText.mm
    )
  elseif(APPLE)
    # macOS platform sources
    list(APPEND IGRAPHICS_SRC
      ${IGRAPHICS_DIR}/Platforms/IGraphicsMac.mm
      ${IGRAPHICS_DIR}/Platforms/IGraphicsMac_view.mm
      ${IGRAPHICS_DIR}/Platforms/IGraphicsCoreText.mm
    )
  elseif(UNIX AND NOT APPLE)
    # Linux platform sources - X11/XCB based windowing
    list(APPEND IGRAPHICS_SRC
      ${IGRAPHICS_DIR}/Platforms/IGraphicsLinux.cpp
      ${IGRAPHICS_DIR}/Platforms/PlatformX11.cpp
    )
  endif()

  target_sources(iPlug2::IGraphics INTERFACE ${IGRAPHICS_SRC})

  target_include_directories(iPlug2::IGraphics INTERFACE
    ${IGRAPHICS_DIR}
    ${IGRAPHICS_DIR}/Controls
    ${IGRAPHICS_DIR}/Drawing
    ${IGRAPHICS_DIR}/Platforms
    ${IGRAPHICS_DIR}/Extras
    ${IGRAPHICS_DEPS_DIR}/STB
    ${IGRAPHICS_DEPS_DIR}/NanoSVG/src
  )

  if(WIN32)
    target_compile_definitions(iPlug2::IGraphics INTERFACE NOMINMAX)
  elseif(IOS)
    # iOS-specific compile options and frameworks
    # Force-include ObjC prefix header to customize class names and avoid conflicts
    # Note: COMPILE_LANGUAGE generator expressions don't work with Xcode generator for INTERFACE libs
    # The pch has #ifdef __OBJC__ guards so it's safe for all languages
    # OBJC_PREFIX is set per-target in iplug_configure_target (must come before -include)
    target_compile_options(iPlug2::IGraphics INTERFACE
      "SHELL:-include ${IPLUG_DIR}/IPlugOBJCPrefix.pch"
    )
    target_link_libraries(iPlug2::IGraphics INTERFACE
      "-framework UIKit"
      "-framework QuartzCore"
      "-framework CoreText"
      "-framework CoreGraphics"
      "-framework Accelerate"
    )
  elseif(APPLE)
    # macOS-specific compile options and frameworks
    # Force-include ObjC prefix header to customize class names and avoid conflicts
    # Note: COMPILE_LANGUAGE generator expressions don't work with Xcode generator for INTERFACE libs
    # The pch has #ifdef __OBJC__ guards so it's safe for all languages
    # OBJC_PREFIX is set per-target in iplug_configure_target (must come before -include)
    target_compile_options(iPlug2::IGraphics INTERFACE
      "SHELL:-include ${IPLUG_DIR}/IPlugOBJCPrefix.pch"
    )
    target_link_libraries(iPlug2::IGraphics INTERFACE
      "-framework Cocoa"
      "-framework Carbon"
      "-framework Accelerate"
      "-framework QuartzCore"
    )
  elseif(UNIX AND NOT APPLE)
    # Linux: Find required X11/XCB libraries via pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(X11_LIBS REQUIRED IMPORTED_TARGET
      freetype2
      fontconfig
      x11-xcb
      xcb
      xcb-util
      xcb-icccm
      xcb-xfixes
    )
    target_link_libraries(iPlug2::IGraphics INTERFACE PkgConfig::X11_LIBS dl)
    target_compile_options(iPlug2::IGraphics INTERFACE -fPIC)
  endif()

  target_link_libraries(iPlug2::IGraphics INTERFACE iPlug2::IPlug)
endif()

# NanoVG drawing backend with OpenGL (simpler cross-platform option)
if(NOT TARGET iPlug2::IGraphics::NanoVG)
  add_library(iPlug2::IGraphics::NanoVG INTERFACE IMPORTED)

  # Note: Platform files use unity-build style #includes:
  # - IGraphicsWin.cpp includes nanovg.c and glad.c directly
  # - IGraphicsMac.mm includes IGraphicsNanoVG.cpp which includes nanovg.c
  # So we only add nanovg.c as a source on platforms that don't do this
  if(NOT WIN32 AND NOT APPLE)
    set(NANOVG_SRC ${IGRAPHICS_DEPS_DIR}/NanoVG/src/nanovg.c)
    target_sources(iPlug2::IGraphics::NanoVG INTERFACE ${NANOVG_SRC})
  endif()

  target_include_directories(iPlug2::IGraphics::NanoVG INTERFACE
    ${IGRAPHICS_DEPS_DIR}/NanoVG/src
    ${IGRAPHICS_DEPS_DIR}/NanoSVG/src
  )

  if(WIN32)
    # glad_GL2/src is needed because IGraphicsWin.cpp does #include "glad.c"
    # nanovg/src is needed because IGraphicsWin.cpp does #include "nanovg.c"
    target_include_directories(iPlug2::IGraphics::NanoVG INTERFACE
      ${IGRAPHICS_DEPS_DIR}/glad_GL2/include
      ${IGRAPHICS_DEPS_DIR}/glad_GL2/src
    )
  elseif(UNIX AND NOT APPLE)
    # Linux: GLX for OpenGL context, glad_GL2 for GL functions
    pkg_check_modules(GLX REQUIRED IMPORTED_TARGET glx)
    target_include_directories(iPlug2::IGraphics::NanoVG INTERFACE
      ${IGRAPHICS_DEPS_DIR}/glad_GL2/include
      ${IGRAPHICS_DEPS_DIR}/glad_GLX/include
    )
    target_sources(iPlug2::IGraphics::NanoVG INTERFACE
      ${IGRAPHICS_DEPS_DIR}/glad_GL2/src/glad.c
      ${IGRAPHICS_DEPS_DIR}/glad_GLX/src/glad_glx.c
    )
    target_link_libraries(iPlug2::IGraphics::NanoVG INTERFACE PkgConfig::GLX)
  endif()

  target_compile_definitions(iPlug2::IGraphics::NanoVG INTERFACE
    IGRAPHICS_NANOVG
    IGRAPHICS_GL2
  )

  if(APPLE)
    # Force-include OpenGL header so GL types are defined before NanoVG uses them
    target_compile_options(iPlug2::IGraphics::NanoVG INTERFACE
      -include OpenGL/gl.h
    )
    target_link_libraries(iPlug2::IGraphics::NanoVG INTERFACE
      "-framework OpenGL"
    )
  endif()

  target_link_libraries(iPlug2::IGraphics::NanoVG INTERFACE iPlug2::IGraphics)
endif()

# NanoVG with GL3 backend
if(NOT TARGET iPlug2::IGraphics::NanoVG::GL3)
  add_library(iPlug2::IGraphics::NanoVG::GL3 INTERFACE IMPORTED)

  # Note: Platform files use unity-build style #includes (see NanoVG GL2 comment)
  if(NOT WIN32 AND NOT APPLE)
    set(NANOVG_GL3_SRC ${IGRAPHICS_DEPS_DIR}/NanoVG/src/nanovg.c)
    target_sources(iPlug2::IGraphics::NanoVG::GL3 INTERFACE ${NANOVG_GL3_SRC})
  endif()

  target_include_directories(iPlug2::IGraphics::NanoVG::GL3 INTERFACE
    ${IGRAPHICS_DEPS_DIR}/NanoVG/src
    ${IGRAPHICS_DEPS_DIR}/NanoSVG/src
  )

  if(WIN32)
    # glad_GL3/src is needed because IGraphicsWin.cpp does #include "glad.c"
    target_include_directories(iPlug2::IGraphics::NanoVG::GL3 INTERFACE
      ${IGRAPHICS_DEPS_DIR}/glad_GL3/include
      ${IGRAPHICS_DEPS_DIR}/glad_GL3/src
    )
  elseif(UNIX AND NOT APPLE)
    # Linux: GLX for OpenGL context, glad_GL3 for GL functions
    target_include_directories(iPlug2::IGraphics::NanoVG::GL3 INTERFACE
      ${IGRAPHICS_DEPS_DIR}/glad_GL3/include
      ${IGRAPHICS_DEPS_DIR}/glad_GLX/include
    )
    target_sources(iPlug2::IGraphics::NanoVG::GL3 INTERFACE
      ${IGRAPHICS_DEPS_DIR}/glad_GL3/src/glad.c
      ${IGRAPHICS_DEPS_DIR}/glad_GLX/src/glad_glx.c
    )
    target_link_libraries(iPlug2::IGraphics::NanoVG::GL3 INTERFACE PkgConfig::GLX)
  endif()

  target_compile_definitions(iPlug2::IGraphics::NanoVG::GL3 INTERFACE
    IGRAPHICS_NANOVG
    IGRAPHICS_GL3
  )

  if(APPLE)
    target_compile_options(iPlug2::IGraphics::NanoVG::GL3 INTERFACE
      -include OpenGL/gl3.h
    )
    target_link_libraries(iPlug2::IGraphics::NanoVG::GL3 INTERFACE
      "-framework OpenGL"
    )
  endif()

  target_link_libraries(iPlug2::IGraphics::NanoVG::GL3 INTERFACE iPlug2::IGraphics)
endif()

# NanoVG with Metal backend (macOS and iOS)
# We use an OBJECT library so we can set -fobjc-arc only on the Metal source file
if(NOT TARGET iPlug2_IGraphics_NanoVG_Metal_obj)
  if(APPLE OR IOS)
    add_library(iPlug2_IGraphics_NanoVG_Metal_obj OBJECT
      ${IGRAPHICS_DIR}/Drawing/IGraphicsNanoVG_src.m
    )

    target_include_directories(iPlug2_IGraphics_NanoVG_Metal_obj PRIVATE
      ${IGRAPHICS_DEPS_DIR}/NanoVG/src
      ${IGRAPHICS_DEPS_DIR}/NanoSVG/src
      ${IGRAPHICS_DEPS_DIR}/MetalNanoVG/src
      ${IGRAPHICS_DIR}
      ${IGRAPHICS_DIR}/Controls
      ${IGRAPHICS_DIR}/Platforms
      ${IGRAPHICS_DIR}/Drawing
    )

    target_compile_definitions(iPlug2_IGraphics_NanoVG_Metal_obj PRIVATE
      IGRAPHICS_NANOVG
      IGRAPHICS_METAL
    )

    # This source file requires ARC
    target_compile_options(iPlug2_IGraphics_NanoVG_Metal_obj PRIVATE -fobjc-arc)

    set_target_properties(iPlug2_IGraphics_NanoVG_Metal_obj PROPERTIES
      POSITION_INDEPENDENT_CODE ON
    )
  endif()
endif()

if(NOT TARGET iPlug2::IGraphics::NanoVG::Metal)
  if(APPLE OR IOS)
    add_library(iPlug2::IGraphics::NanoVG::Metal INTERFACE IMPORTED)

    target_include_directories(iPlug2::IGraphics::NanoVG::Metal INTERFACE
      ${IGRAPHICS_DEPS_DIR}/NanoVG/src
      ${IGRAPHICS_DEPS_DIR}/NanoSVG/src
      ${IGRAPHICS_DEPS_DIR}/MetalNanoVG/src
    )

    target_compile_definitions(iPlug2::IGraphics::NanoVG::Metal INTERFACE
      IGRAPHICS_NANOVG
      IGRAPHICS_METAL
    )

    # Link object files using generator expression
    target_sources(iPlug2::IGraphics::NanoVG::Metal INTERFACE
      $<TARGET_OBJECTS:iPlug2_IGraphics_NanoVG_Metal_obj>
    )

    target_link_libraries(iPlug2::IGraphics::NanoVG::Metal INTERFACE
      "-framework Metal"
      "-framework MetalKit"
      iPlug2::IGraphics
    )
  endif()
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

# Metal configuration for Apple platforms (macOS and iOS)
if(APPLE OR IOS)
  if(NOT TARGET iPlug2::IGraphics::Metal)
    add_library(iPlug2::IGraphics::Metal INTERFACE IMPORTED)
    target_compile_definitions(iPlug2::IGraphics::Metal INTERFACE IGRAPHICS_METAL)
  endif()
endif()

# Skia drawing backend
if(NOT TARGET iPlug2::IGraphics::Skia)
  add_library(iPlug2::IGraphics::Skia INTERFACE IMPORTED)
  target_link_libraries(iPlug2::IGraphics::Skia INTERFACE iPlug2::IGraphics)
  target_compile_definitions(iPlug2::IGraphics::Skia INTERFACE IGRAPHICS_SKIA)

  set(SKIA_PATH ${DEPS_DIR}/Build/src/skia)
  target_include_directories(iPlug2::IGraphics::Skia INTERFACE
    ${SKIA_PATH}
    ${SKIA_PATH}/include
    ${SKIA_PATH}/include/core
    ${SKIA_PATH}/include/effects
    ${SKIA_PATH}/include/config
    ${SKIA_PATH}/include/utils
    ${SKIA_PATH}/include/utils/mac
    ${SKIA_PATH}/include/gpu
    ${SKIA_PATH}/include/private
    ${SKIA_PATH}/modules/svg/include
    ${SKIA_PATH}/modules/skcms
  )

  if(WIN32)
    set(SKIA_LIB_PATH ${SKIA_PATH}/out/Release-x64)
    target_link_libraries(iPlug2::IGraphics::Skia INTERFACE
      ${SKIA_LIB_PATH}/skia.lib
      ${SKIA_LIB_PATH}/svg.lib
    )
  elseif(APPLE)
    set(SKIA_LIB_PATH ${DEPS_DIR}/Build/mac/lib)
    target_link_libraries(iPlug2::IGraphics::Skia INTERFACE
      ${SKIA_LIB_PATH}/libskia.a
      ${SKIA_LIB_PATH}/libsvg.a
      ${SKIA_LIB_PATH}/libskshaper.a
      ${SKIA_LIB_PATH}/libskparagraph.a
      ${SKIA_LIB_PATH}/libskunicode_core.a
      ${SKIA_LIB_PATH}/libskunicode_icu.a
    )
  endif()
endif()

# Skia with Metal backend (macOS and iOS)
if(APPLE OR IOS)
  if(NOT TARGET iPlug2::IGraphics::Skia::Metal)
    add_library(iPlug2::IGraphics::Skia::Metal INTERFACE IMPORTED)

    target_compile_definitions(iPlug2::IGraphics::Skia::Metal INTERFACE
      IGRAPHICS_SKIA
      IGRAPHICS_METAL
    )

    target_link_libraries(iPlug2::IGraphics::Skia::Metal INTERFACE
      "-framework Metal"
      "-framework MetalKit"
      iPlug2::IGraphics::Skia
    )
  endif()
endif()

# Skia with GL3 backend
if(NOT TARGET iPlug2::IGraphics::Skia::GL3)
  add_library(iPlug2::IGraphics::Skia::GL3 INTERFACE IMPORTED)

  target_compile_definitions(iPlug2::IGraphics::Skia::GL3 INTERFACE
    IGRAPHICS_SKIA
    IGRAPHICS_GL3
  )

  if(APPLE)
    target_link_libraries(iPlug2::IGraphics::Skia::GL3 INTERFACE
      "-framework OpenGL"
    )
  endif()

  target_link_libraries(iPlug2::IGraphics::Skia::GL3 INTERFACE
    iPlug2::IGraphics::Skia
  )
endif()

# Skia with CPU backend (software rendering)
if(NOT TARGET iPlug2::IGraphics::Skia::CPU)
  add_library(iPlug2::IGraphics::Skia::CPU INTERFACE IMPORTED)

  target_compile_definitions(iPlug2::IGraphics::Skia::CPU INTERFACE
    IGRAPHICS_SKIA
    IGRAPHICS_CPU
  )

  target_link_libraries(iPlug2::IGraphics::Skia::CPU INTERFACE
    iPlug2::IGraphics::Skia
  )
endif()

# =============================================================================
# IGraphics Backend Selection - Convenience Variables
# =============================================================================
# These can be overridden by setting IGRAPHICS_BACKEND and/or IGRAPHICS_RENDERER
# BEFORE including iPlug2.cmake or calling find_package(iPlug2)

# Backend selection (NANOVG or SKIA)
if(NOT DEFINED IGRAPHICS_BACKEND)
  set(IGRAPHICS_BACKEND "NANOVG" CACHE STRING "IGraphics drawing backend")
  set_property(CACHE IGRAPHICS_BACKEND PROPERTY STRINGS "NANOVG" "SKIA")
endif()

# Renderer selection with platform-aware defaults
# NANOVG supports: GL2, GL3, METAL (Metal is macOS/iOS only)
# SKIA supports: GL3, METAL, CPU
if(NOT DEFINED IGRAPHICS_RENDERER)
  if(WIN32)
    set(DEFAULT_RENDERER "GL2")
  elseif(APPLE OR IOS)
    set(DEFAULT_RENDERER "METAL")
  else()
    set(DEFAULT_RENDERER "GL2")
  endif()
  set(IGRAPHICS_RENDERER "${DEFAULT_RENDERER}" CACHE STRING "IGraphics renderer")
  set_property(CACHE IGRAPHICS_RENDERER PROPERTY STRINGS "GL2" "GL3" "METAL" "CPU")
endif()

# Construct IGRAPHICS_LIB target based on selections
if(NOT DEFINED IGRAPHICS_LIB)
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
endif()
