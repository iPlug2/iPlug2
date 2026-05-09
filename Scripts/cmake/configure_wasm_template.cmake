#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# configure_wasm_template.cmake - Configure wasm template files with placeholders
#
# This script performs placeholder substitution on template files for the wasm
# web distribution.
#
# Required variables:
#   INPUT_FILE  - Path to the template file
#   OUTPUT_FILE - Path for the configured output file
#
# Optional placeholder variables (all use string substitution):
#   NAME_PLACEHOLDER         - Plugin name (e.g., "IPlugEffect")
#   NAME_PLACEHOLDER_LC      - Plugin name lowercase (e.g., "iplugeffect")
#   MAXNINPUTS_PLACEHOLDER   - Maximum number of audio inputs
#   MAXNOUTPUTS_PLACEHOLDER  - Maximum number of audio outputs
#   IS_INSTRUMENT_PLACEHOLDER - "true" if no audio inputs, "false" otherwise
#   HAS_UI_PLACEHOLDER       - "true" if plugin has UI, "false" otherwise
#   HOST_RESIZE_PLACEHOLDER  - "true" if plugin supports host resize
#   DOES_MIDI_IN_PLACEHOLDER - "true" if plugin receives MIDI
#   DOES_MIDI_OUT_PLACEHOLDER - "true" if plugin sends MIDI

if(NOT DEFINED INPUT_FILE)
  message(FATAL_ERROR "INPUT_FILE must be defined")
endif()

if(NOT DEFINED OUTPUT_FILE)
  message(FATAL_ERROR "OUTPUT_FILE must be defined")
endif()

# Normalize path to resolve any .. or . components
get_filename_component(INPUT_FILE_REAL "${INPUT_FILE}" REALPATH)

if(NOT EXISTS "${INPUT_FILE_REAL}")
  message(FATAL_ERROR "Template file not found: ${INPUT_FILE_REAL}")
endif()

# Read the template
file(READ "${INPUT_FILE_REAL}" TEMPLATE_CONTENT)

# Perform placeholder substitutions
# IMPORTANT: Replace longer patterns first to avoid partial matches
# (e.g., NAME_PLACEHOLDER_LC must be replaced before NAME_PLACEHOLDER)
if(DEFINED NAME_PLACEHOLDER_LC)
  string(REPLACE "NAME_PLACEHOLDER_LC" "${NAME_PLACEHOLDER_LC}" TEMPLATE_CONTENT "${TEMPLATE_CONTENT}")
endif()

if(DEFINED NAME_PLACEHOLDER)
  string(REPLACE "NAME_PLACEHOLDER" "${NAME_PLACEHOLDER}" TEMPLATE_CONTENT "${TEMPLATE_CONTENT}")
endif()

if(DEFINED MAXNINPUTS_PLACEHOLDER)
  string(REPLACE "MAXNINPUTS_PLACEHOLDER" "${MAXNINPUTS_PLACEHOLDER}" TEMPLATE_CONTENT "${TEMPLATE_CONTENT}")
endif()

if(DEFINED MAXNOUTPUTS_PLACEHOLDER)
  string(REPLACE "MAXNOUTPUTS_PLACEHOLDER" "${MAXNOUTPUTS_PLACEHOLDER}" TEMPLATE_CONTENT "${TEMPLATE_CONTENT}")
endif()

if(DEFINED IS_INSTRUMENT_PLACEHOLDER)
  string(REPLACE "IS_INSTRUMENT_PLACEHOLDER" "${IS_INSTRUMENT_PLACEHOLDER}" TEMPLATE_CONTENT "${TEMPLATE_CONTENT}")
endif()

if(DEFINED HAS_UI_PLACEHOLDER)
  string(REPLACE "HAS_UI_PLACEHOLDER" "${HAS_UI_PLACEHOLDER}" TEMPLATE_CONTENT "${TEMPLATE_CONTENT}")
endif()

if(DEFINED HOST_RESIZE_PLACEHOLDER)
  string(REPLACE "HOST_RESIZE_PLACEHOLDER" "${HOST_RESIZE_PLACEHOLDER}" TEMPLATE_CONTENT "${TEMPLATE_CONTENT}")
endif()

if(DEFINED DOES_MIDI_IN_PLACEHOLDER)
  string(REPLACE "DOES_MIDI_IN_PLACEHOLDER" "${DOES_MIDI_IN_PLACEHOLDER}" TEMPLATE_CONTENT "${TEMPLATE_CONTENT}")
endif()

if(DEFINED DOES_MIDI_OUT_PLACEHOLDER)
  string(REPLACE "DOES_MIDI_OUT_PLACEHOLDER" "${DOES_MIDI_OUT_PLACEHOLDER}" TEMPLATE_CONTENT "${TEMPLATE_CONTENT}")
endif()

# Comment out <script> tags for resource bundles that were not produced.
# The template uses <!--FONTS--><script src="scripts/fonts.js"></script> markers;
# when the corresponding FOUND_<TAG> is "false" we wrap the whole tag in an
# HTML comment so the browser skips the missing file.
foreach(_entry FONTS:fonts.js SVGS:svgs.js IMGS:imgs.js IMGS2X:imgs@2x.js)
  string(REPLACE ":" ";" _parts "${_entry}")
  list(GET _parts 0 _tag)
  list(GET _parts 1 _js)
  set(_flag_var "FOUND_${_tag}")
  if(DEFINED ${_flag_var} AND "${${_flag_var}}" STREQUAL "false")
    string(REPLACE
      "<!--${_tag}--><script src=\"scripts/${_js}\"></script>"
      "<!-- <script src=\"scripts/${_js}\"></script> -->"
      TEMPLATE_CONTENT "${TEMPLATE_CONTENT}")
  endif()
endforeach()

# Write the configured file
file(WRITE "${OUTPUT_FILE}" "${TEMPLATE_CONTENT}")
