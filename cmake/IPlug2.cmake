include_guard(GLOBAL)

cmake_policy(SET CMP0054 NEW)  # Only interpret if() arguments as variables or keywords when unquoted

include(CMakePrintHelpers)

# Get default IPLUG2_ROOT_PATH
get_filename_component(_root ${CMAKE_CURRENT_LIST_DIR} DIRECTORY)
set(IPLUG2_ROOT_PATH "${_root}" CACHE PATH "")  # I am root...

if(IPLUG2_ROOT_PATH)
    list(APPEND CMAKE_MODULE_PATH "${IPLUG2_ROOT_PATH}/cmake")
    list(REMOVE_DUPLICATES CMAKE_MODULE_PATH)
endif()

include(IPlug2Helpers)
include(IPlug2Core)
include(IPlug2Internal)

_iplug_pre_project_setup()
