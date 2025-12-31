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

# iPlug2 source files needed for tests
set(IPLUG_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/../../IPlug/IPlugParameter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../IPlug/Extras/Synth/VoiceAllocator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../IPlug/Extras/Synth/MidiSynth.cpp
)

# Test source files
set(TEST_SOURCES
  IPlugQueueTest.cpp
  IPlugParameterTest.cpp
  ISenderTest.cpp
  DSPTest.cpp
  MidiSynthTest.cpp
  SerializationTest.cpp
)

# Define the test executable
add_executable(IPlug2UnitTests
  ${TEST_SOURCES}
  ${IPLUG_SOURCES}
)

# Include iPlug2 headers
target_include_directories(IPlug2UnitTests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../IPlug
  ${CMAKE_CURRENT_SOURCE_DIR}/../../IPlug/Extras
  ${CMAKE_CURRENT_SOURCE_DIR}/../../IPlug/Extras/Synth
  ${CMAKE_CURRENT_SOURCE_DIR}/../../WDL
  ${CMAKE_CURRENT_SOURCE_DIR}/../../WDL/fft
)

# Define IPLUG2_TESTS to allow test-specific code paths if needed
target_compile_definitions(IPlug2UnitTests PRIVATE
  IPLUG2_TESTS
  SAMPLE_TYPE_FLOAT  # Use float for sample type in tests
)

# Link Catch2
target_link_libraries(IPlug2UnitTests PRIVATE Catch2::Catch2WithMain)

# Enable CTest integration
include(CTest)
include(Catch)
catch_discover_tests(IPlug2UnitTests)
