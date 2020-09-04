# This module is accessed either by IPlug2's root CMakeLists.txt or
# by an external CMakeLists.txt from a user project.

include_guard(GLOBAL)
include(CMakePrintHelpers)

#------------------------------------------------------------------------------

# Get default IPLUG2_ROOT_PATH
get_filename_component(_root ${CMAKE_CURRENT_LIST_DIR} DIRECTORY)
set(IPLUG2_ROOT_PATH "${_root}" CACHE PATH "")  # I am root...

if(IPLUG2_ROOT_PATH AND EXISTS "${IPLUG2_ROOT_PATH}/cmake/IPlug2.cmake")
    list(APPEND CMAKE_MODULE_PATH "${IPLUG2_ROOT_PATH}/cmake")
    list(REMOVE_DUPLICATES CMAKE_MODULE_PATH)
endif()

include(IPlug2Helpers)
include(IPlug2Core)
include(IPlug2Internal)

validate_path("${IPLUG2_ROOT_PATH}" "IPLUG2_ROOT_PATH path not set correctly")

# Standard in-source build prevention
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    iplug_syntax_error(
        "Generating build files in source tree is not allowed to prevent sleepless nights and lots of tears."
        "Create a build directory outside of the source code and call cmake from there.")
endif()

#------------------------------------------------------------------------------
# Get IPlug2 verion number from ROOT/IPlug/IPlugVersion.h
iplug_get_version("${IPLUG2_ROOT_PATH}/IPlug/IPlugVersion.h" IPLUG_VERSION)
iplug_info("IPlug2 version ${IPLUG_VERSION}")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")
