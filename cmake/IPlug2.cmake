# This module is accessed either by IPlug2's root CMakeLists.txt or
# by an external CMakeLists.txt from a user project.

include_guard(GLOBAL)
include(CMakePrintHelpers)

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

# Get IPlug2 verion number from ROOT/IPlug/IPlugVersion.h
iplug_get_version("${IPLUG2_ROOT_PATH}/IPlug/IPlugVersion.h" IPLUG_VERSION)
iplug_info("IPlug2 version ${IPLUG_VERSION}")

set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)


# Find the VST3 SDK
set(_path $ENV{VST3_SDK_PATH})
if(NOT _path)
    set(_path "${IPLUG2_ROOT_PATH}/Dependencies/IPlug/VST3_SDK")
endif()
set(VST3_SDK_PATH "${_path}" CACHE PATH "Path to Steinberg VST3 SDK")

set(_files
    "CMakeLists.txt"
    "base/CMakeLists.txt"
    "pluginterfaces/CMakeLists.txt"
    "public.sdk/CMakeLists.txt"
    "public.sdk/source/main/winexport.def"
    "public.sdk/source/main/macexport.exp"
)
set(HAVESDK_VST3 TRUE)
foreach(_file IN LISTS _files)
    if(NOT EXISTS "${VST3_SDK_PATH}/${_file}")
        set(HAVESDK_VST3 FALSE)
    endif()
endforeach()


#------------------------------------------------------------------------------
# Glad version settings

set(IPLUG2_GLAD_VERSION "4.5-ES2-3.1-Core" CACHE STRING "Select version of Glad to compile for OpenGL")
set_property(CACHE IPLUG2_GLAD_VERSION PROPERTY STRINGS
    "2.1-Compability"
    "2.1-Core"
    "3.3-Compability"
    "3.3-Core"
    "4.5-ES2-3.1-Compability"
    "4.5-ES2-3.1-Core"
)


#------------------------------------------------------------------------------

# To keep options within a somewhat reasonable level, we choose graphics backend in cmake options,
# not per target. This way we can bake it into the static library. Though, every target will
# have to use the same graphics backend. Unfortunately this also means that we can't compile
# for multiple plugins if we're targeting a web application (at the moment).

# TODO: Add support for the other graphics libraries

set(IPLUG2_GFXLIBRARY "GFXLIB_NANOVG" CACHE STRING "Select backend graphics library for rendering")
set_property(CACHE IPLUG2_GFXLIBRARY PROPERTY STRINGS
    # GFXLIB_AGG
    # GFXLIB_CAIRO
    # GFXLIB_LICE
    GFXLIB_NANOVG
    # GFXLIB_SKIA
    # GFXLIB_CANVAS
)

unset(GFXLIB_AGG)
unset(GFXLIB_CAIRO)
unset(GFXLIB_LICE)
unset(GFXLIB_NANOVG)
unset(GFXLIB_SKIA)
unset(GFXLIB_CANVAS)
set(${IPLUG2_GFXLIBRARY} TRUE)

if(PLATFORM_APPLE)
    option(IPLUG2_ENABLE_METALNANOVG "Use MetalNanoVG for rendering" ON)
endif()
