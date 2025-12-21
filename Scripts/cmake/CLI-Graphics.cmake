#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# CLI target configuration for iPlug2 with IGraphics support
# Headless command-line interface with CPU rendering for UI screenshots

include(${CMAKE_CURRENT_LIST_DIR}/IPlug.cmake)

# Set up paths (IGraphics.cmake would do this, but we don't want the platform sources)
set(IGRAPHICS_DIR ${IPLUG2_DIR}/IGraphics)
set(IGRAPHICS_DEPS_DIR ${DEPS_DIR}/IGraphics)

if(NOT TARGET iPlug2::CLI::Graphics)
  add_library(iPlug2::CLI::Graphics INTERFACE IMPORTED)

  # Core IGraphics sources (without platform-specific windowing code)
  set(CLI_GRAPHICS_SRC
    # IPlug CLI sources
    ${IPLUG2_DIR}/IPlug/CLI/IPlugCLI.cpp
    ${IPLUG2_DIR}/IPlug/CLI/IPlugCLI_ScreenshotRenderer.cpp
    ${IPLUG2_DIR}/IPlug/CLI/IPlugCLI_main.cpp
    # Core IGraphics (no platform GUI)
    ${IGRAPHICS_DIR}/IControl.cpp
    ${IGRAPHICS_DIR}/IGraphics.cpp
    ${IGRAPHICS_DIR}/IGraphicsEditorDelegate.cpp
    ${IGRAPHICS_DIR}/Controls/IControls.cpp
    ${IGRAPHICS_DIR}/Controls/IPopupMenuControl.cpp
    ${IGRAPHICS_DIR}/Controls/ITextEntryControl.cpp
    # Headless platform implementation
    ${IGRAPHICS_DIR}/Platforms/IGraphicsHeadless.cpp
    # Skia drawing backend
    ${IGRAPHICS_DIR}/Drawing/IGraphicsSkia.cpp
  )

  # On Windows, add WDL's win32_utf8.c for UTF-8 file path support (used by wavwrite.h)
  if(WIN32)
    list(APPEND CLI_GRAPHICS_SRC ${IPLUG2_DIR}/WDL/win32_utf8.c)
  endif()

  # On macOS, add CoreText support for font handling
  if(APPLE)
    list(APPEND CLI_GRAPHICS_SRC ${IGRAPHICS_DIR}/Platforms/IGraphicsCoreText.mm)
  endif()

  target_sources(iPlug2::CLI::Graphics INTERFACE ${CLI_GRAPHICS_SRC})

  # Include FlexBox support (via object library to avoid IGraphics platform dependency)
  include(${CMAKE_CURRENT_LIST_DIR}/IGraphics.cmake)  # Defines iPlug2_Extras_FlexBox_obj
  target_sources(iPlug2::CLI::Graphics INTERFACE $<TARGET_OBJECTS:iPlug2_Extras_FlexBox_obj>)
  target_include_directories(iPlug2::CLI::Graphics INTERFACE
    ${IGRAPHICS_DEPS_DIR}/yoga
    ${IGRAPHICS_DEPS_DIR}/yoga/yoga
  )

  # Include directories for IGraphics
  target_include_directories(iPlug2::CLI::Graphics INTERFACE
    ${IPLUG2_DIR}/IPlug/CLI
    ${IGRAPHICS_DIR}
    ${IGRAPHICS_DIR}/Controls
    ${IGRAPHICS_DIR}/Drawing
    ${IGRAPHICS_DIR}/Platforms
    ${IGRAPHICS_DIR}/Extras
    ${IGRAPHICS_DEPS_DIR}/STB
    ${IGRAPHICS_DEPS_DIR}/NanoSVG/src
    ${IGRAPHICS_DEPS_DIR}/yoga
    ${IGRAPHICS_DEPS_DIR}/yoga/yoga
  )

  # Skia include directories
  set(SKIA_PATH ${DEPS_DIR}/Build/src/skia)
  target_include_directories(iPlug2::CLI::Graphics INTERFACE
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

  target_compile_definitions(iPlug2::CLI::Graphics INTERFACE
    CLI_API
    IPLUG_DSP=1
    IPLUG_EDITOR=1
    IGRAPHICS_SKIA
    IGRAPHICS_CPU
    IGRAPHICS_HEADLESS
  )

  # Link against IPlug
  target_link_libraries(iPlug2::CLI::Graphics INTERFACE iPlug2::IPlug)

  # Link Skia libraries directly
  if(WIN32)
    set(SKIA_LIB_PATH ${DEPS_DIR}/Build/win/x64/Release)
    # Note: Link order matters for static libraries. Dependent libs must come before their dependencies.
    target_link_libraries(iPlug2::CLI::Graphics INTERFACE
      ${SKIA_LIB_PATH}/svg.lib
      ${SKIA_LIB_PATH}/skshaper.lib
      ${SKIA_LIB_PATH}/skparagraph.lib
      ${SKIA_LIB_PATH}/skunicode_core.lib
      ${SKIA_LIB_PATH}/skunicode_icu.lib
      ${SKIA_LIB_PATH}/skia.lib
    )
  elseif(APPLE)
    set(SKIA_LIB_PATH ${DEPS_DIR}/Build/mac/lib)
    target_link_libraries(iPlug2::CLI::Graphics INTERFACE
      ${SKIA_LIB_PATH}/libskia.a
      ${SKIA_LIB_PATH}/libsvg.a
      ${SKIA_LIB_PATH}/libskshaper.a
      ${SKIA_LIB_PATH}/libskparagraph.a
      ${SKIA_LIB_PATH}/libskunicode_core.a
      ${SKIA_LIB_PATH}/libskunicode_icu.a
      "-framework Cocoa"
      "-framework QuartzCore"
      "-framework CoreText"
      "-framework CoreGraphics"
      "-framework Accelerate"
    )
  elseif(UNIX)
    set(SKIA_LIB_PATH ${DEPS_DIR}/Build/linux/lib)
    # Note: Link order matters for static libraries. Dependent libs must come before their dependencies.
    # libsvg and other modules depend on libskia, so libskia goes last.
    target_link_libraries(iPlug2::CLI::Graphics INTERFACE
      ${SKIA_LIB_PATH}/libsvg.a
      ${SKIA_LIB_PATH}/libskshaper.a
      ${SKIA_LIB_PATH}/libskparagraph.a
      ${SKIA_LIB_PATH}/libskunicode_core.a
      ${SKIA_LIB_PATH}/libskunicode_icu.a
      ${SKIA_LIB_PATH}/libskia.a
      fontconfig
      freetype
      pthread
      dl
    )
  endif()
endif()

function(iplug_configure_cli_graphics target project_name)
  target_link_libraries(${target} PUBLIC iPlug2::CLI::Graphics)

  set_target_properties(${target} PROPERTIES
    OUTPUT_NAME "${project_name}"
    CXX_STANDARD ${IPLUG2_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
  )

  if(APPLE)
    set_target_properties(${target} PROPERTIES
      MACOSX_BUNDLE FALSE
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out"
      # No code signing needed for CLI tools
      XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
    )
  elseif(WIN32)
    set_target_properties(${target} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${project_name}-cli"
    )
  elseif(UNIX)
    set_target_properties(${target} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out"
    )
  endif()
endfunction()
