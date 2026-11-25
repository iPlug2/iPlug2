cmake_minimum_required(VERSION 3.14)
project(IGraphicsLinuxResize LANGUAGES C CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "This regression test requires Linux and X11")
endif()

set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "iPlug2 root directory")
include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

add_executable(IGraphicsLinuxResize main.cpp)
target_compile_features(IGraphicsLinuxResize PRIVATE cxx_std_17)
target_compile_definitions(IGraphicsLinuxResize PRIVATE IPLUG_EDITOR=1)
target_link_libraries(IGraphicsLinuxResize PRIVATE ${IGRAPHICS_LIB})

enable_testing()
add_test(NAME IGraphicsLinuxResize COMMAND IGraphicsLinuxResize)
