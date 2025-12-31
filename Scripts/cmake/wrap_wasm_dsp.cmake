#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# wrap_wasm_dsp.cmake - Post-process DSP JS for AudioWorklet scope
#
# This script wraps the Emscripten-generated DSP JS file with a shim that makes
# it work in AudioWorklet scope where 'window', 'document', and 'location' are
# not available.
#
# Required variables:
#   DSP_JS_FILE - Path to the DSP JS file to wrap
#
# The shim provides:
#   - globalThis.Module reference
#   - Fake location object (required by Emscripten runtime)
#   - Self reference for AudioWorklet compatibility

if(NOT DEFINED DSP_JS_FILE)
  message(FATAL_ERROR "DSP_JS_FILE must be defined")
endif()

# Normalize path to resolve any .. or . components
get_filename_component(DSP_JS_FILE_REAL "${DSP_JS_FILE}" REALPATH)

if(NOT EXISTS "${DSP_JS_FILE_REAL}")
  message(FATAL_ERROR "DSP JS file not found: ${DSP_JS_FILE_REAL}")
endif()

# Read the original file
file(READ "${DSP_JS_FILE_REAL}" DSP_CONTENT)

# Create the AudioWorklet scope wrapper
# This shim makes Emscripten-generated code work in AudioWorklet where
# 'window' and 'document' are undefined
set(WORKLET_SHIM "// AudioWorklet scope shim for Emscripten
var self = globalThis;
self.location = self.location || { href: 'https://localhost/' };
var Module = globalThis.Module = globalThis.Module || {};

")

# Write the wrapped file
file(WRITE "${DSP_JS_FILE_REAL}" "${WORKLET_SHIM}${DSP_CONTENT}")

message(STATUS "Wrapped ${DSP_JS_FILE_REAL} for AudioWorklet scope")
