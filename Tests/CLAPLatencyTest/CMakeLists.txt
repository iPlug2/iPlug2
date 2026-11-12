cmake_minimum_required(VERSION 3.14)
project(CLAPLatencyTest LANGUAGES C CXX)
enable_testing()
get_filename_component(IPLUG2_ROOT "../.." ABSOLUTE)
set(CLAP_SDK_DIR "${IPLUG2_ROOT}/Dependencies/IPlug/CLAP_SDK" CACHE PATH "CLAP SDK")
set(CLAP_HELPERS_DIR "${IPLUG2_ROOT}/Dependencies/IPlug/CLAP_HELPERS" CACHE PATH "CLAP helpers")
add_executable(CLAPLatencyTest CLAPLatencyTest.cpp
  ${IPLUG2_ROOT}/IPlug/CLAP/IPlugCLAP.cpp
  ${IPLUG2_ROOT}/IPlug/IPlugAPIBase.cpp
  ${IPLUG2_ROOT}/IPlug/IPlugPluginBase.cpp
  ${IPLUG2_ROOT}/IPlug/IPlugProcessor.cpp
  ${IPLUG2_ROOT}/IPlug/IPlugParameter.cpp
  ${IPLUG2_ROOT}/IPlug/IPlugPaths.cpp
  ${IPLUG2_ROOT}/IPlug/IPlugTimer.cpp)
target_compile_features(CLAPLatencyTest PRIVATE cxx_std_17)
target_compile_definitions(CLAPLatencyTest PRIVATE CLAP_API IPLUG_DSP=1 IPLUG_EDITOR=1 NO_IGRAPHICS NOMINMAX)
target_include_directories(CLAPLatencyTest PRIVATE . ${IPLUG2_ROOT}/IPlug ${IPLUG2_ROOT}/IPlug/CLAP
  ${IPLUG2_ROOT}/IPlug/Extras ${IPLUG2_ROOT}/WDL ${CLAP_SDK_DIR}/include
  ${CLAP_HELPERS_DIR}/include ${CLAP_HELPERS_DIR}/include/clap/helpers)
if(WIN32)
  target_sources(CLAPLatencyTest PRIVATE ${IPLUG2_ROOT}/WDL/win32_utf8.c)
  target_compile_definitions(CLAPLatencyTest PRIVATE _CRT_SECURE_NO_WARNINGS)
  target_link_libraries(CLAPLatencyTest PRIVATE shlwapi comctl32 wininet)
elseif(APPLE)
  enable_language(OBJCXX)
  target_sources(CLAPLatencyTest PRIVATE ${IPLUG2_ROOT}/IPlug/IPlugPaths.mm)
  target_link_libraries(CLAPLatencyTest PRIVATE "-framework Cocoa")
else()
  message(FATAL_ERROR "The upstream CLAP wrapper currently requires Windows or macOS")
endif()
add_test(NAME CLAPLatencyTest COMMAND CLAPLatencyTest)
