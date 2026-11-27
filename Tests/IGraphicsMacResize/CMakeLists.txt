cmake_minimum_required(VERSION 3.14)
project(IGraphicsMacResize LANGUAGES C CXX)

if(NOT APPLE OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
  message(FATAL_ERROR "This regression test requires macOS and AppKit")
endif()
enable_language(OBJCXX)

set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "iPlug2 root directory")
include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

add_executable(IGraphicsMacResize main.mm)
target_compile_features(IGraphicsMacResize PRIVATE cxx_std_17)
set_target_properties(IGraphicsMacResize PROPERTIES OBJCXX_STANDARD 17)
target_compile_definitions(IGraphicsMacResize PRIVATE AU_API IPLUG_EDITOR=1
  OBJC_PREFIX=vIGraphicsMacResize)
iplug_apply_objc_prefix_header(IGraphicsMacResize)
target_link_libraries(IGraphicsMacResize PRIVATE ${IGRAPHICS_LIB}
  "-framework AudioUnit" "-framework AudioToolbox" "-framework CoreAudio")

enable_testing()
add_test(NAME IGraphicsMacResize COMMAND IGraphicsMacResize)
