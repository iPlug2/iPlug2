#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# IGraphicsTests - unit and integration tests for IGraphics
#
# The platform-independent core of IGraphics (control stack, event dispatch,
# layout, the control library) is compiled against IGraphicsHeadless, a test
# implementation of the drawing/platform layer. No drawing backend, GPU, window
# server or font is needed, so the same tests run on every platform and in CI.

set(IGRAPHICS_DIR ${IPLUG2_DIR}/IGraphics)

file(GLOB IGRAPHICS_TEST_SRC CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/Unit/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Integration/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Harness/*.h
)

add_executable(IGraphicsTests
  ${IGRAPHICS_TEST_SRC}
  ${IGRAPHICS_DIR}/IControl.cpp
  ${IGRAPHICS_DIR}/IGraphics.cpp
  ${IGRAPHICS_DIR}/IGraphicsEditorDelegate.cpp
  ${IGRAPHICS_DIR}/Controls/IControls.cpp
  ${IGRAPHICS_DIR}/Controls/IPopupMenuControl.cpp
  ${IGRAPHICS_DIR}/Controls/ITextEntryControl.cpp
)

if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
  # Must come before IGraphics/Platforms: see Harness/LinuxShim/IGraphicsLinux.h
  target_include_directories(IGraphicsTests BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Harness/LinuxShim)
endif()

target_include_directories(IGraphicsTests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Harness
  ${CMAKE_CURRENT_SOURCE_DIR}/../IPlug/Harness
  ${IGRAPHICS_DIR}
  ${IGRAPHICS_DIR}/Controls
  ${IGRAPHICS_DIR}/Extras
  ${IPLUG2_DIR}/Dependencies/IGraphics/NanoSVG/src
  ${IPLUG2_DIR}/Dependencies/IGraphics/NanoVG/src
  ${IPLUG2_DIR}/Dependencies/IGraphics/STB
  ${IPLUG2_DIR}/Dependencies/IGraphics/yoga
)

# IGRAPHICS_NANOVG only selects the lightest set of backend types (an int bitmap
# handle and nanosvg): none of the NanoVG sources are compiled.
target_compile_definitions(IGraphicsTests PRIVATE IGRAPHICS_NANOVG IPLUG_AUTOMATED_TESTS)

target_link_libraries(IGraphicsTests PRIVATE iPlug2::IPlug)

if(APPLE)
  target_link_libraries(IGraphicsTests PRIVATE
    "-framework CoreText"
    "-framework CoreGraphics"
  )
endif()

iplug_add_automated_test(IGraphicsTests)
