#  ==============================================================================
#  
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# This file should be included in your main CMakeLists.txt file.

if (APPLE)
  enable_language(OBJC)
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

# Set common output directory when !WIN32
if(NOT WIN32)
 set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out)
 set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out)
 set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out)
endif()

# Make sure Xcode generator uses the same output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
