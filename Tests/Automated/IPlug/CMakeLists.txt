#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# IPlugTests - unit and integration tests for IPlug, built without IGraphics

file(GLOB IPLUG_TEST_SRC CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/Unit/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Integration/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Harness/*.h
)

add_executable(IPlugTests ${IPLUG_TEST_SRC})

target_include_directories(IPlugTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Harness)
target_compile_definitions(IPlugTests PRIVATE NO_IGRAPHICS IPLUG_AUTOMATED_TESTS)
target_link_libraries(IPlugTests PRIVATE
  iPlug2::IPlug
  iPlug2::Extras::Synth
  iPlug2::Extras::HIIR
)

iplug_add_automated_test(IPlugTests)
