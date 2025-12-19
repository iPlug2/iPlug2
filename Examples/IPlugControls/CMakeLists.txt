cmake_minimum_required(VERSION 3.14)
project(IPlugControls VERSION 1.0.0)

if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "iPlug2 root directory")
endif()

include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    IPlugControls.cpp
    IPlugControls.h
    IconsFontaudio.h
    IconsForkAwesome.h
    resources/resource.h
  RESOURCES
    resources/fonts/Roboto-Regular.ttf
    resources/fonts/fontaudio.ttf
    resources/fonts/forkawesome-webfont.ttf
    resources/img/button.png
    resources/img/button@2x.png
    resources/img/font.png
    resources/img/font@2x.png
    resources/img/hslider-handle.svg
    resources/img/hslider-track.svg
    resources/img/knob.png
    resources/img/knob@2x.png
    resources/img/knob-rotate.png
    resources/img/knob-rotate@2x.png
    resources/img/slider-handle.png
    resources/img/slider-handle.svg
    resources/img/slider-handle@2x.png
    resources/img/slider-track.png
    resources/img/slider-track.svg
    resources/img/slider-track@2x.png
    resources/img/switch.png
    resources/img/switch@2x.png
    resources/img/vector-knob.svg
)
