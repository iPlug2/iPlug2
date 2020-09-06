cmake_minimum_required(VERSION 3.11)
include_guard(GLOBAL)

cmake_policy(SET CMP0076 NEW)  # The target_sources() command converts relative paths to absolute.

add_library(IPlug INTERFACE)

file(GLOB_RECURSE _files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS "*")

target_sources(IPlug INTERFACE ${_files})
