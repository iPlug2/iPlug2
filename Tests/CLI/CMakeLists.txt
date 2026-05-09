cmake_minimum_required(VERSION 3.14)
project(iPlug2CLITests)

# ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
# ==============================================================================

# CLI target smoke/regression tests. These use the dedicated IPlugCLITest
# example plugin so the tests can exercise the real command-line executable.

if(NOT DEFINED IPLUG2_DIR)
  get_filename_component(IPLUG2_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

enable_testing()

if(IOS OR CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  return()
endif()

if(NOT TARGET IPlugCLITest-cli)
  add_subdirectory(${IPLUG2_DIR}/Examples/IPlugCLITest ${CMAKE_CURRENT_BINARY_DIR}/IPlugCLITest)
endif()

find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_test(
  NAME IPlugCLITest.CLI
  COMMAND ${Python3_EXECUTABLE}
          ${CMAKE_CURRENT_LIST_DIR}/cli_smoke_test.py
          --executable $<TARGET_FILE:IPlugCLITest-cli>
)

set_tests_properties(IPlugCLITest.CLI PROPERTIES
  LABELS "cli;iplug"
  FIXTURES_REQUIRED IPlugCLITestCLIExecutable
)

add_test(
  NAME IPlugCLITest.CLI.Build
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target IPlugCLITest-cli --config $<CONFIG>
)

set_tests_properties(IPlugCLITest.CLI.Build PROPERTIES
  LABELS "cli;iplug"
  FIXTURES_SETUP IPlugCLITestCLIExecutable
)
