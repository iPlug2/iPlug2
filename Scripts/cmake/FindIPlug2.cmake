#  ==============================================================================
#  
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# FindiPlug2.cmake
# This module finds the iPlug2 library and sets up the necessary variables and targets.

# Set iPlug2 root directory if not already defined
# if(NOT DEFINED IPLUG2_DIR)
#   set(IPLUG2_DIR "${CMAKE_CURRENT_LIST_DIR}/.." CACHE PATH "iPlug2 root directory")
# endif()

# message(STATUS "FindIPlug2.cmake: Debugging: IPLUG2_DIR = ${IPLUG2_DIR}")

# Include the files that defined the iPlug2 components 
include(${CMAKE_CURRENT_LIST_DIR}/IPlug.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/IGraphics.cmake)

# Set up the iPlug2 package
set(IPLUG2_FOUND TRUE)

# Define the components of iPlug2
set(IPLUG2_COMPONENTS IPlug IGraphics)

# Check which components were requested
foreach(component ${IPLUG2_FIND_COMPONENTS})
  if(NOT component IN_LIST IPLUG2_COMPONENTS)
    set(IPLUG2_FOUND FALSE)
    set(IPLUG2_NOT_FOUND_MESSAGE "Unsupported component: ${component}")
  endif()
endforeach()

# If no components were explicitly requested, assume all are wanted
if(NOT IPLUG2_FIND_COMPONENTS)
  set(IPLUG2_FIND_COMPONENTS ${IPLUG2_COMPONENTS})
endif()

# Verify that requested components are available
foreach(component ${IPLUG2_FIND_COMPONENTS})
  if(NOT TARGET iPlug2::${component})
    set(IPLUG2_${component}_FOUND FALSE)
    if(IPLUG2_FIND_REQUIRED_${component})
      set(IPLUG2_FOUND FALSE)
      list(APPEND IPLUG2_NOT_FOUND_MESSAGE "Required component '${component}' is missing.")
    endif()
  else()
    set(IPLUG2_${component}_FOUND TRUE)
  endif()
endforeach()

# Report the result of the package search
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(iPlug2
  REQUIRED_VARS IPLUG2_DIR
  # HANDLE_COMPONENTS
  FAIL_MESSAGE "iPlug2 not found. Please set IPLUG2_DIR to the root directory of iPlug2."
)

# If found, set up an overall iPlug2 interface target
if(IPLUG2_FOUND AND NOT TARGET iPlug2::iPlug2)
  add_library(iPlug2::iPlug2 INTERFACE IMPORTED)
  target_link_libraries(iPlug2::iPlug2 INTERFACE iPlug2::IPlug)
endif()

# Helper function to set up a target (app, plug-in etc)
function(iplug_add_target target visibility)
  cmake_parse_arguments(PARSE_ARGV 2 CONFIG
    ""
    ""
    "INCLUDE;SOURCE;DEFINE;OPTION;LINK;LINK_DIR;DEPEND;FEATURE;RESOURCE"
  )

  if(CONFIG_INCLUDE)
    target_include_directories(${target} ${visibility} ${CONFIG_INCLUDE})
  endif()
  
  if(CONFIG_SOURCE)
    target_sources(${target} ${visibility} ${CONFIG_SOURCE})
  endif()
  
  if(CONFIG_DEFINE)
    target_compile_definitions(${target} ${visibility} ${CONFIG_DEFINE})
  endif()
  
  if(CONFIG_OPTION)
    target_compile_options(${target} ${visibility} ${CONFIG_OPTION})
  endif()
  
  if(CONFIG_LINK)
    target_link_libraries(${target} ${visibility} ${CONFIG_LINK})
  endif()
  
  if(CONFIG_LINK_DIR)
    target_link_directories(${target} ${visibility} ${CONFIG_LINK_DIR})
  endif()
  
  if(CONFIG_DEPEND)
    add_dependencies(${target} ${CONFIG_DEPEND})
  endif()
  
  if(CONFIG_FEATURE)
    target_compile_features(${target} ${visibility} ${CONFIG_FEATURE})
  endif()
  
  if(CONFIG_RESOURCE)
    set_target_properties(${target} PROPERTIES RESOURCE "${CONFIG_RESOURCE}")
  endif()

  if(CONFIG_UNPARSED_ARGUMENTS)
    message(WARNING "Unused arguments: ${CONFIG_UNPARSED_ARGUMENTS}")
  endif()
endfunction()

# Helper function to configure a target based on its type
function(iplug_configure_target target target_type project_name)
  set(SUPPORTED_TYPES 
    APP
    # AAX_PLUGIN
    # AUv2_PLUGIN
    # VST2_PLUGIN
    # VST3_PLUGIN
    # CLAP_PLUGIN
  )
  
  if(NOT ${target_type} IN_LIST SUPPORTED_TYPES)
    message(FATAL_ERROR "Unsupported target type '${target_type}' for target '${target}'")
  endif()

  set_target_properties(${target} PROPERTIES
    CXX_STANDARD ${IPLUG2_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
  )

  # Platform-specific configurations
  # if(WIN32)
  #   iplug_configure_windows_target(${target})
  # elseif(APPLE)
  #   iplug_configure_macos_target(${target})
  # elseif(UNIX AND NOT APPLE)
  #   iplug_configure_linux_target(${target})
  # else()
  #   message(WARNING "Unsupported platform for target '${target}'")
  # endif()
  
  # Include and configure based on target type
  include("${IPLUG2_CMAKE_DIR}/${target_type}.cmake")
  
  string(TOLOWER ${target_type} lowercase_type)
  string(REPLACE "_" "-" function_suffix ${lowercase_type})
  cmake_language(CALL iplug_configure_${function_suffix} ${target} ${project_name})
endfunction()