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

# message(STATUS "iPlug2.cmake: Debugging: IPLUG2_DIR = ${IPLUG2_DIR}")

# TODO: check IPLUG2_DIR exists or set to current dir
# set(IPLUG2_DIR ${CMAKE_CURRENT_LIST_DIR})

set(IPLUG2_CXX_STANDARD 17)

# Add the iPlug2 cmake module path
set(IPLUG2_CMAKE_DIR ${IPLUG2_DIR}/Scripts/cmake)
list(APPEND CMAKE_MODULE_PATH ${IPLUG2_CMAKE_DIR})

# Make sure MSVC uses static linking
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Generate folders for generators that support it (Visual Studio, Xcode, etc.)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# # Set common output directory when !WIN32
# if(NOT WIN32)
#  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out)
#  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out)
#  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out)
# endif()

# # Make sure Xcode generator uses the same output directories
# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")