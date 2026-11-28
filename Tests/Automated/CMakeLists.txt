#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# iPlug2 automated tests (unit + integration), built on Catch2 and run via CTest.
#
# Can be configured on its own, which is the quickest way to run the tests and
# needs no plug-in SDKs, graphics libraries or windowing system:
#
#   cmake -S Tests/Automated -B build/tests
#   cmake --build build/tests
#   ctest --test-dir build/tests --output-on-failure
#
# or as part of the main iPlug2 build with -DIPLUG2_BUILD_AUTOMATED_TESTS=ON

cmake_minimum_required(VERSION 3.18)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(iPlug2AutomatedTests LANGUAGES C CXX)

  if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
  endif()

  get_filename_component(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
  include(${IPLUG2_DIR}/iPlug2.cmake)
  # Only the core IPlug targets are needed: the tests list the IGraphics core
  # sources themselves so that no platform/drawing backend has to be found.
  include(${IPLUG2_DIR}/Scripts/cmake/IPlug.cmake)
endif()

enable_testing()

# ------------------------------------------------------------------------------
# Catch2
# ------------------------------------------------------------------------------
set(IPLUG2_TESTS_CATCH2_TAG "v3.8.1" CACHE STRING "Catch2 git tag fetched when no installed Catch2 v3 is found")

find_package(Catch2 3 QUIET)
if(NOT Catch2_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG ${IPLUG2_TESTS_CATCH2_TAG}
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(Catch2)
  list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
  set_target_properties(Catch2 Catch2WithMain PROPERTIES FOLDER "Tests/Catch2")
endif()
include(Catch)

# iplug_add_automated_test(<target>)
# Applies the settings shared by the test executables and registers every
# Catch2 test case with CTest, labelled "unit" or "integration" by its tag.
function(iplug_add_automated_test target)
  set_target_properties(${target} PROPERTIES
    CXX_STANDARD ${IPLUG2_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    FOLDER "Tests/Automated"
  )
  target_link_libraries(${target} PRIVATE Catch2::Catch2WithMain)

  if(MSVC)
    target_compile_options(${target} PRIVATE /bigobj)
  endif()

  if(EMSCRIPTEN)
    # Run under node, with exceptions enabled as Catch2 relies on them (IPlug uses embind)
    target_compile_options(${target} PRIVATE -fexceptions)
    target_link_options(${target} PRIVATE -fexceptions --bind -sALLOW_MEMORY_GROWTH=1 -sEXIT_RUNTIME=1 -sNODERAWFS=1)
  endif()

  foreach(kind unit integration)
    catch_discover_tests(${target}
      TEST_SPEC "[${kind}]"
      TEST_PREFIX "${target}."
      PROPERTIES LABELS "${kind}" TIMEOUT 120
      DISCOVERY_MODE PRE_TEST
    )
  endforeach()
endfunction()

add_subdirectory(IPlug)
add_subdirectory(IGraphics)
