#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# iPlug2 Unit Tests using Catch2

cmake_minimum_required(VERSION 3.14)
project(IPlug2UnitTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch Catch2
include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.0
)
FetchContent_MakeAvailable(Catch2)

# Define the test executable
add_executable(IPlug2UnitTests
  IPlugQueueTest.cpp
)

# Include iPlug2 headers
target_include_directories(IPlug2UnitTests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../IPlug
  ${CMAKE_CURRENT_SOURCE_DIR}/../../WDL
)

# Link Catch2
target_link_libraries(IPlug2UnitTests PRIVATE Catch2::Catch2WithMain)

# Enable CTest integration
include(CTest)
include(Catch)
catch_discover_tests(IPlug2UnitTests)
