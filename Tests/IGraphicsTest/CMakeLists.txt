cmake_minimum_required(VERSION 3.14)
project(IGraphicsTest VERSION 1.0.0)

if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "iPlug2 root directory")
endif()

include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

# Source files
set(SOURCES
  IGraphicsTest.cpp
  IGraphicsTest.h
  resources/resource.h
)

# Apple-specific test controls using Metal
set(EXTRA_LINK)
if(APPLE)
  list(APPEND SOURCES
    ${IPLUG2_DIR}/IGraphics/Controls/Test/TestMPSControl.mm
    ${IPLUG2_DIR}/IGraphics/Controls/Test/TestCustomShaderControl.mm
  )
  # MetalPerformanceShaders needed for TestMPSControl
  set(EXTRA_LINK "-framework MetalPerformanceShaders")
endif()

iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    ${SOURCES}
  RESOURCES
    resources/fonts/Roboto-Regular.ttf
    resources/fonts/Montserrat-LightItalic.ttf
    resources/img/23.svg
    resources/img/orbs.svg
    resources/img/smiley.png
    resources/img/smiley@2x.png
    resources/img/iplug.png
    resources/img/iplug@2x.png
    resources/img/src.png
    resources/img/src@2x.png
    resources/img/dst.png
    resources/img/dst@2x.png
  LINK iPlug2::Extras::FlexBox ${EXTRA_LINK}
)

# TestCustomShaderControl needs ShaderTypes.h from resources/shaders
if(APPLE)
  target_include_directories(_${PROJECT_NAME}-base INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders)
endif()
