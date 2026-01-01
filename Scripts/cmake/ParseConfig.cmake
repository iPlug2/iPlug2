#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for more info.
#
#  ==============================================================================

# ParseConfig.cmake
# Parses config.h to extract plugin metadata into CMake variables.
#
# Usage:
#   iplug_parse_config(<project_dir>)
#
# Sets the following variables in parent scope:
#   PLUG_NAME, PLUG_MFR, PLUG_VERSION_HEX, PLUG_VERSION_STR
#   PLUG_UNIQUE_ID, PLUG_MFR_ID, PLUG_COPYRIGHT_STR
#   BUNDLE_NAME, BUNDLE_MFR, BUNDLE_DOMAIN
#   PLUG_TYPE, PLUG_DOES_MIDI_IN, PLUG_HAS_UI
#   AUV2_FACTORY, AUV2_VIEW_CLASS
#   FULL_VER_STR (computed), PLUG_VERSION_INT (computed)
#   AU_TYPE (computed based on PLUG_TYPE and PLUG_DOES_MIDI_IN)

include_guard(GLOBAL)

function(iplug_parse_config project_dir)
  set(CONFIG_FILE "${project_dir}/config.h")

  if(NOT EXISTS "${CONFIG_FILE}")
    message(FATAL_ERROR "config.h not found at ${CONFIG_FILE}")
  endif()

  # Read config.h content
  file(STRINGS "${CONFIG_FILE}" CONFIG_LINES)

  # String elements to extract (values in quotes)
  set(STRING_ELEMENTS
    PLUG_NAME
    PLUG_MFR
    PLUG_VERSION_STR
    PLUG_COPYRIGHT_STR
    BUNDLE_NAME
    BUNDLE_MFR
    BUNDLE_DOMAIN
  )

  # Identifier elements (not quoted - C identifiers)
  set(IDENTIFIER_ELEMENTS
    AUV2_FACTORY
    AUV2_VIEW_CLASS
    PLUG_CLASS_NAME
  )

  # Integer elements to extract
  set(INT_ELEMENTS
    PLUG_TYPE
    PLUG_DOES_MIDI_IN
    PLUG_DOES_MIDI_OUT
    PLUG_HAS_UI
    PLUG_WIDTH
    PLUG_HEIGHT
  )

  # 4-char code elements (in single quotes)
  set(FOURCC_ELEMENTS
    PLUG_UNIQUE_ID
    PLUG_MFR_ID
  )

  # Hex elements
  set(HEX_ELEMENTS
    PLUG_VERSION_HEX
  )

  # Initialize defaults
  foreach(elem ${STRING_ELEMENTS})
    set(${elem} "" PARENT_SCOPE)
  endforeach()
  foreach(elem ${IDENTIFIER_ELEMENTS})
    set(${elem} "" PARENT_SCOPE)
  endforeach()
  foreach(elem ${INT_ELEMENTS})
    set(${elem} 0 PARENT_SCOPE)
  endforeach()
  foreach(elem ${FOURCC_ELEMENTS})
    set(${elem} "" PARENT_SCOPE)
  endforeach()
  foreach(elem ${HEX_ELEMENTS})
    set(${elem} "0x00010000" PARENT_SCOPE)
  endforeach()

  # Parse each line
  foreach(line ${CONFIG_LINES})
    # Skip comments
    string(REGEX MATCH "^[ \t]*//" IS_COMMENT "${line}")
    if(IS_COMMENT)
      continue()
    endif()

    # Extract string elements (quoted with double quotes)
    foreach(elem ${STRING_ELEMENTS})
      string(REGEX MATCH "#define[ \t]+${elem}[ \t]+\"([^\"]*)\"" MATCH "${line}")
      if(MATCH)
        set(${elem} "${CMAKE_MATCH_1}" PARENT_SCOPE)
        set(${elem}_LOCAL "${CMAKE_MATCH_1}")
      endif()
    endforeach()

    # Extract identifier elements (C identifiers, not quoted)
    foreach(elem ${IDENTIFIER_ELEMENTS})
      string(REGEX MATCH "#define[ \t]+${elem}[ \t]+([A-Za-z_][A-Za-z0-9_]*)" MATCH "${line}")
      if(MATCH)
        set(${elem} "${CMAKE_MATCH_1}" PARENT_SCOPE)
        set(${elem}_LOCAL "${CMAKE_MATCH_1}")
      endif()
    endforeach()

    # Extract integer elements
    foreach(elem ${INT_ELEMENTS})
      string(REGEX MATCH "#define[ \t]+${elem}[ \t]+([0-9]+)" MATCH "${line}")
      if(MATCH)
        set(${elem} "${CMAKE_MATCH_1}" PARENT_SCOPE)
        set(${elem}_LOCAL "${CMAKE_MATCH_1}")
      endif()
    endforeach()

    # Extract 4-char code elements (single quotes)
    foreach(elem ${FOURCC_ELEMENTS})
      string(REGEX MATCH "#define[ \t]+${elem}[ \t]+'([^']*)'" MATCH "${line}")
      if(MATCH)
        set(${elem} "${CMAKE_MATCH_1}" PARENT_SCOPE)
        set(${elem}_LOCAL "${CMAKE_MATCH_1}")
      endif()
    endforeach()

    # Extract hex elements
    foreach(elem ${HEX_ELEMENTS})
      string(REGEX MATCH "#define[ \t]+${elem}[ \t]+(0x[0-9a-fA-F]+)" MATCH "${line}")
      if(MATCH)
        set(${elem} "${CMAKE_MATCH_1}" PARENT_SCOPE)
        set(${elem}_LOCAL "${CMAKE_MATCH_1}")
      endif()
    endforeach()
  endforeach()

  # Compute derived values

  # Parse version from hex: 0xVVVVMMBB -> VVVV.MM.BB
  if(DEFINED PLUG_VERSION_HEX_LOCAL)
    # Convert hex to decimal
    math(EXPR VERSION_INT "${PLUG_VERSION_HEX_LOCAL}" OUTPUT_FORMAT DECIMAL)
    set(PLUG_VERSION_INT ${VERSION_INT} PARENT_SCOPE)

    # Extract major.minor.bugfix
    math(EXPR VERSION_MAJOR "(${VERSION_INT} >> 16) & 0xFFFF")
    math(EXPR VERSION_MINOR "(${VERSION_INT} >> 8) & 0xFF")
    math(EXPR VERSION_BUGFIX "${VERSION_INT} & 0xFF")

    set(FULL_VER_STR "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_BUGFIX}" PARENT_SCOPE)
    set(VERSION_MAJOR ${VERSION_MAJOR} PARENT_SCOPE)
    set(VERSION_MINOR ${VERSION_MINOR} PARENT_SCOPE)
    set(VERSION_BUGFIX ${VERSION_BUGFIX} PARENT_SCOPE)
  else()
    set(FULL_VER_STR "1.0.0" PARENT_SCOPE)
    set(PLUG_VERSION_INT 65536 PARENT_SCOPE)
    set(VERSION_MAJOR 1 PARENT_SCOPE)
    set(VERSION_MINOR 0 PARENT_SCOPE)
    set(VERSION_BUGFIX 0 PARENT_SCOPE)
  endif()

  # Determine AudioUnit type based on PLUG_TYPE and PLUG_DOES_MIDI_IN
  # PLUG_TYPE: 0=Effect, 1=Instrument, 2=MIDIProcessor
  set(PLUG_TYPE_VAL 0)
  set(PLUG_DOES_MIDI_VAL 0)
  if(DEFINED PLUG_TYPE_LOCAL)
    set(PLUG_TYPE_VAL ${PLUG_TYPE_LOCAL})
  endif()
  if(DEFINED PLUG_DOES_MIDI_IN_LOCAL)
    set(PLUG_DOES_MIDI_VAL ${PLUG_DOES_MIDI_IN_LOCAL})
  endif()

  if(PLUG_TYPE_VAL EQUAL 0)
    if(PLUG_DOES_MIDI_VAL)
      set(AU_TYPE "aumf" PARENT_SCOPE)  # MusicEffect
      set(AU_TYPE_TAG "Effects" PARENT_SCOPE)
    else()
      set(AU_TYPE "aufx" PARENT_SCOPE)  # Effect
      set(AU_TYPE_TAG "Effects" PARENT_SCOPE)
    endif()
  elseif(PLUG_TYPE_VAL EQUAL 1)
    set(AU_TYPE "aumu" PARENT_SCOPE)    # MusicDevice (Instrument)
    set(AU_TYPE_TAG "Synth" PARENT_SCOPE)
  elseif(PLUG_TYPE_VAL EQUAL 2)
    set(AU_TYPE "aumi" PARENT_SCOPE)    # MIDIProcessor
    set(AU_TYPE_TAG "Effects" PARENT_SCOPE)
  else()
    set(AU_TYPE "aufx" PARENT_SCOPE)    # Default to Effect
    set(AU_TYPE_TAG "Effects" PARENT_SCOPE)
  endif()

  # Compute bundle info string
  if(DEFINED BUNDLE_NAME_LOCAL AND DEFINED FULL_VER_STR_LOCAL AND DEFINED PLUG_COPYRIGHT_STR_LOCAL)
    set(CFBundleGetInfoString "${BUNDLE_NAME_LOCAL} v${FULL_VER_STR_LOCAL} ${PLUG_COPYRIGHT_STR_LOCAL}" PARENT_SCOPE)
  elseif(DEFINED BUNDLE_NAME_LOCAL)
    set(CFBundleGetInfoString "${BUNDLE_NAME_LOCAL}" PARENT_SCOPE)
  endif()

endfunction()

# Utility function to print parsed config values (for debugging)
function(iplug_print_config)
  message(STATUS "=== Parsed config.h values ===")
  message(STATUS "  PLUG_NAME: ${PLUG_NAME}")
  message(STATUS "  PLUG_MFR: ${PLUG_MFR}")
  message(STATUS "  PLUG_VERSION_HEX: ${PLUG_VERSION_HEX}")
  message(STATUS "  PLUG_VERSION_STR: ${PLUG_VERSION_STR}")
  message(STATUS "  PLUG_UNIQUE_ID: ${PLUG_UNIQUE_ID}")
  message(STATUS "  PLUG_MFR_ID: ${PLUG_MFR_ID}")
  message(STATUS "  PLUG_COPYRIGHT_STR: ${PLUG_COPYRIGHT_STR}")
  message(STATUS "  BUNDLE_NAME: ${BUNDLE_NAME}")
  message(STATUS "  BUNDLE_MFR: ${BUNDLE_MFR}")
  message(STATUS "  BUNDLE_DOMAIN: ${BUNDLE_DOMAIN}")
  message(STATUS "  PLUG_TYPE: ${PLUG_TYPE}")
  message(STATUS "  PLUG_DOES_MIDI_IN: ${PLUG_DOES_MIDI_IN}")
  message(STATUS "  PLUG_HAS_UI: ${PLUG_HAS_UI}")
  message(STATUS "  AUV2_FACTORY: ${AUV2_FACTORY}")
  message(STATUS "  AUV2_VIEW_CLASS: ${AUV2_VIEW_CLASS}")
  message(STATUS "  FULL_VER_STR: ${FULL_VER_STR}")
  message(STATUS "  PLUG_VERSION_INT: ${PLUG_VERSION_INT}")
  message(STATUS "  AU_TYPE: ${AU_TYPE}")
  message(STATUS "==============================")
endfunction()
