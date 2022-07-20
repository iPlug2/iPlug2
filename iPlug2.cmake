#
# This file should be included in your main CMakeLists.txt file. #
#


if (APPLE)
  enable_language(OBJCXX)
endif()

# We need this so we can find call FindFaust.cmake
set(IPLUG2_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR}/Scripts/cmake)
list(APPEND CMAKE_MODULE_PATH ${IPLUG2_CMAKE_DIR})

# This is used in many places
set(IPLUG2_DIR ${CMAKE_CURRENT_LIST_DIR})

# Make sure MSVC uses static linking for compatibility with Skia libraries and easier distribution.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# We generate folders for targets that support it (Visual Studio, Xcode, etc.)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
