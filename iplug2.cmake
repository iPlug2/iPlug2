#
# This file should be included in your main CMakeLists.txt file. #
#

# We need this so we can find call FindFaust.cmake
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

# This is used in many places
set(IPLUG2_DIR ${CMAKE_CURRENT_LIST_DIR})

# CMP0091 is REQURED to be NEW for this to work.
cmake_policy(GET CMP0091 _tmp)
if (NOT "${_tmp}" STREQUAL "NEW")
  message(FATAL_ERROR "CMake policy CMP0091 must be set to NEW!\nCall cmake_policy(SET CMP0091 NEW) before your project() definition.")
endif()
# Make sure MSVC uses static linking for compatibility with Skia libraries and easier distribution.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# We generate folders for targets that support it (Visual Studio, Xcode, etc.)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
