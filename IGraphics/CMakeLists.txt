cmake_minimum_required(VERSION 3.11)
include_guard(GLOBAL)

add_library(IGraphics INTERFACE)

file(GLOB_RECURSE _files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "*")

target_sources(IGraphics INTERFACE ${_files})

