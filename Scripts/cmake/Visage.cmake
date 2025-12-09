#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# Visage configuration for iPlug2
# This module provides VisageEditorDelegate for plugins that use Visage UI instead of IGraphics
#
# Usage:
# 1. Use FetchContent to get Visage library in your plugin's CMakeLists.txt
# 2. Link to iPlug2::Visage and visage targets

include(${CMAKE_CURRENT_LIST_DIR}/IPlug.cmake)

if(NOT TARGET iPlug2::Visage)
  add_library(iPlug2_Visage STATIC)
  add_library(iPlug2::Visage ALIAS iPlug2_Visage)

  set(VISAGE_DIR ${IPLUG2_DIR}/IPlug/Extras/Visage)

  target_sources(iPlug2_Visage PRIVATE
    ${VISAGE_DIR}/IPlugVisageEditorDelegate.cpp
  )

  target_include_directories(iPlug2_Visage PUBLIC
    ${VISAGE_DIR}
  )

  target_compile_definitions(iPlug2_Visage PUBLIC
    VISAGE_EDITOR_DELEGATE
    NO_IGRAPHICS
  )

  target_link_libraries(iPlug2_Visage PUBLIC
    iPlug2::IPlug
  )

  # C++17 required for Visage
  target_compile_features(iPlug2_Visage PUBLIC cxx_std_17)
endif()
