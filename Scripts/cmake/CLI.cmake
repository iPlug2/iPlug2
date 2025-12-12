#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# CLI target configuration for iPlug2
# Headless command-line interface for offline audio processing

include(${CMAKE_CURRENT_LIST_DIR}/IPlug.cmake)

if(NOT TARGET iPlug2::CLI)
  add_library(iPlug2::CLI INTERFACE IMPORTED)

  set(CLI_SRC
    ${IPLUG2_DIR}/IPlug/CLI/IPlugCLI.cpp
    ${IPLUG2_DIR}/IPlug/CLI/IPlugCLI_main.cpp
  )

  # On Windows, add WDL's win32_utf8.c for UTF-8 file path support (used by wavwrite.h)
  if(WIN32)
    list(APPEND CLI_SRC ${IPLUG2_DIR}/WDL/win32_utf8.c)
  endif()

  target_sources(iPlug2::CLI INTERFACE ${CLI_SRC})

  target_include_directories(iPlug2::CLI INTERFACE
    ${IPLUG2_DIR}/IPlug/CLI
  )

  target_compile_definitions(iPlug2::CLI INTERFACE
    CLI_API
    NO_IGRAPHICS
    IPLUG_DSP=1
  )

  target_link_libraries(iPlug2::CLI INTERFACE iPlug2::IPlug)
endif()

function(iplug_configure_cli target project_name)
  target_link_libraries(${target} PUBLIC iPlug2::CLI)

  set_target_properties(${target} PROPERTIES
    OUTPUT_NAME "${project_name}"
    CXX_STANDARD ${IPLUG2_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
  )

  if(APPLE)
    set_target_properties(${target} PROPERTIES
      MACOSX_BUNDLE FALSE
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out"
      # No code signing needed for CLI tools
      XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
    )
  elseif(WIN32)
    set_target_properties(${target} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${project_name}-cli"
    )
  elseif(UNIX AND NOT APPLE)
    # Linux CLI configuration
    set_target_properties(${target} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out"
    )
  endif()
endfunction()
