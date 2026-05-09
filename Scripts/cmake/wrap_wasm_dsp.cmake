#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# wrap_wasm_dsp.cmake - Prepend AudioWorklet scope shim to DSP JS.
#
# Emscripten's SINGLE_FILE=1 output embeds the wasm binary as a pseudo-Latin1
# string inside the .js file, which contains arbitrary byte values including
# '\\', '\x00' and quote characters. Reading it through CMake's file(READ) /
# file(WRITE) corrupts or truncates it, so we shell out to Python for a
# binary-safe prepend.
#
# Required variables:
#   Python3_EXECUTABLE - Path to Python interpreter (passed from configure).
#   DSP_JS_FILE        - Absolute path to the DSP JS file to prepend to.
#   SHIM_FILE          - Absolute path to the shim file to prepend.

if(NOT DEFINED Python3_EXECUTABLE)
  message(FATAL_ERROR "Python3_EXECUTABLE must be defined")
endif()

if(NOT DEFINED DSP_JS_FILE)
  message(FATAL_ERROR "DSP_JS_FILE must be defined")
endif()

if(NOT DEFINED SHIM_FILE)
  message(FATAL_ERROR "SHIM_FILE must be defined")
endif()

get_filename_component(DSP_JS_FILE_REAL "${DSP_JS_FILE}" REALPATH)
get_filename_component(SHIM_FILE_REAL   "${SHIM_FILE}"   REALPATH)

if(NOT EXISTS "${DSP_JS_FILE_REAL}")
  message(FATAL_ERROR "DSP JS file not found: ${DSP_JS_FILE_REAL}")
endif()
if(NOT EXISTS "${SHIM_FILE_REAL}")
  message(FATAL_ERROR "Shim file not found: ${SHIM_FILE_REAL}")
endif()

execute_process(
  COMMAND ${Python3_EXECUTABLE} -c "import sys; shim=open(sys.argv[1],'rb').read(); body=open(sys.argv[2],'rb').read(); open(sys.argv[2],'wb').write(shim+body)"
  "${SHIM_FILE_REAL}" "${DSP_JS_FILE_REAL}"
  RESULT_VARIABLE _rc
)

if(NOT _rc EQUAL 0)
  message(FATAL_ERROR "Failed to prepend worklet shim to ${DSP_JS_FILE_REAL}")
endif()

message(STATUS "Prepended AudioWorklet shim to ${DSP_JS_FILE_REAL}")
