#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# deploy_codesign.cmake - Sign a deployed plugin copy (Xcode generator only).
#
# Xcode runs CMake POST_BUILD script phases *before* its own CodeSign phase, and
# links with -no_adhoc_codesign when it intends to sign later. A bundle copied
# in POST_BUILD is therefore completely unsigned and dlopen refuses to load it
# on Apple Silicon. This script re-signs the copy with the same identity Xcode
# uses for the build product, read from the build settings Xcode exports to
# script phases.
#
# Required variables:
#   DEPLOY_PATH - Absolute path to the deployed bundle or file.

if(NOT DEPLOY_PATH)
  message(FATAL_ERROR "deploy_codesign.cmake: DEPLOY_PATH not set")
endif()

# Signing disabled for this target: the linker ad-hoc signs the binary itself
if("$ENV{CODE_SIGNING_ALLOWED}" STREQUAL "NO")
  return()
endif()

set(_identity "$ENV{EXPANDED_CODE_SIGN_IDENTITY}")
if(_identity STREQUAL "")
  return()
endif()

set(_args --force --sign "${_identity}" --timestamp=none)

if("$ENV{ENABLE_HARDENED_RUNTIME}" STREQUAL "YES")
  list(APPEND _args --options runtime)
endif()

if(NOT "$ENV{CODE_SIGN_ENTITLEMENTS}" STREQUAL "")
  set(_entitlements "$ENV{CODE_SIGN_ENTITLEMENTS}")
  if(NOT IS_ABSOLUTE "${_entitlements}")
    set(_entitlements "$ENV{SRCROOT}/${_entitlements}")
  endif()
  if(EXISTS "${_entitlements}")
    list(APPEND _args --entitlements "${_entitlements}")
  endif()
endif()

execute_process(
  COMMAND /usr/bin/codesign ${_args} "${DEPLOY_PATH}"
  RESULT_VARIABLE _result
  ERROR_VARIABLE _error
)

if(NOT _result EQUAL 0)
  message(FATAL_ERROR "[iPlug2] Failed to sign deployed plugin ${DEPLOY_PATH}:\n${_error}")
endif()

message(STATUS "[iPlug2] Signed deployed plugin: ${DEPLOY_PATH}")
