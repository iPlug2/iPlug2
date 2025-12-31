#  ==============================================================================
#  
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# FindiPlug2.cmake
# This module finds the iPlug2 library and sets up the necessary variables and targets.

# Include iOS platform configuration FIRST if building for iOS or visionOS
# This sets the IOS variable needed by other modules like IGraphics.cmake
if(CMAKE_SYSTEM_NAME MATCHES "^(iOS|visionOS)$")
  include(${CMAKE_CURRENT_LIST_DIR}/iOS.cmake)
endif()

# Include the files that defined the iPlug2 components
include(${CMAKE_CURRENT_LIST_DIR}/IPlug.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/IGraphics.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Deploy.cmake)

# Include plugin format modules to set up targets and availability checks
# These are included early so that iPlug2::* targets exist when projects link to them
include(${CMAKE_CURRENT_LIST_DIR}/VST2.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/VST3.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/CLAP.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/AAX.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/APP.cmake)

# Include AUv3 helper functions (macOS only)
if(APPLE AND NOT IOS)
  include(${CMAKE_CURRENT_LIST_DIR}/AUv3.cmake)
endif()

# Include iOS AUv3 helper functions (iOS/visionOS only)
if(IOS)
  include(${CMAKE_CURRENT_LIST_DIR}/AUv3iOS.cmake)
endif()

# Include WAM/Web/Wasm modules for Emscripten builds
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  include(${CMAKE_CURRENT_LIST_DIR}/WAM.cmake)
  include(${CMAKE_CURRENT_LIST_DIR}/Web.cmake)
  include(${CMAKE_CURRENT_LIST_DIR}/WAMDist.cmake)
  include(${CMAKE_CURRENT_LIST_DIR}/WasmDSP.cmake)
  include(${CMAKE_CURRENT_LIST_DIR}/WasmUI.cmake)
  include(${CMAKE_CURRENT_LIST_DIR}/WasmDist.cmake)
endif()

# Include the plugin helper macro (iplug_add_plugin)
include(${CMAKE_CURRENT_LIST_DIR}/IPlugPlugin.cmake)

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

# Helper function to apply the ObjC prefix header to a target
# This ensures all ObjC/ObjC++ sources use the namespaced class names.
# Uses -include flag for all generators (including Xcode) for reliability.
# Note: We avoid GCC_PRECOMPILE_PREFIX_HEADER=YES because Xcode's shared PCH
# mechanism can fail in CI environments with parallel builds.
function(iplug_apply_objc_prefix_header target)
  if(NOT APPLE)
    return()
  endif()

  # Force-include the prefix header on ObjC/ObjC++ sources
  # The Xcode generator doesn't always properly map COMPILE_LANGUAGE:OBJCXX to
  # OTHER_CPLUSPLUSFLAGS, so we apply to all C-family languages.
  # The prefix header is guarded by #ifdef __OBJC__ so it's safe for C/C++ files.
  target_compile_options(${target} PRIVATE
    "$<$<COMPILE_LANGUAGE:C>:SHELL:-include ${IPLUG_DIR}/IPlugOBJCPrefix.pch>"
    "$<$<COMPILE_LANGUAGE:CXX>:SHELL:-include ${IPLUG_DIR}/IPlugOBJCPrefix.pch>"
    "$<$<COMPILE_LANGUAGE:OBJC>:SHELL:-include ${IPLUG_DIR}/IPlugOBJCPrefix.pch>"
    "$<$<COMPILE_LANGUAGE:OBJCXX>:SHELL:-include ${IPLUG_DIR}/IPlugOBJCPrefix.pch>"
  )
endfunction()

# Helper function to configure a target based on its type
function(iplug_configure_target target target_type project_name)
  set(SUPPORTED_TYPES
    APP
    VST2
    VST3
    CLAP
    AUv2
    AAX
    AUv3Framework
    AUv3Appex
    # iOS targets
    IOSApp
    AUv3iOSFramework
    AUv3iOSAppex
    # Web/Emscripten targets
    WAM
    Web
  )

  if(NOT ${target_type} IN_LIST SUPPORTED_TYPES)
    message(FATAL_ERROR "Unsupported target type '${target_type}' for target '${target}'")
  endif()

  set_target_properties(${target} PROPERTIES
    CXX_STANDARD ${IPLUG2_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
  )

  # Set unique OBJC_PREFIX per target to avoid namespace conflicts when
  # a DAW loads multiple plugin formats of the same plugin simultaneously.
  # The API suffix (_vst3, _au, etc.) is added by IPlugOBJCPrefix.pch based on
  # the format-specific API defines (VST3_API, AU_API, etc.)
  if(APPLE)
    target_compile_definitions(${target} PRIVATE OBJC_PREFIX=v${project_name})

    # Prevent Xcode from combining @1x/@2x PNGs into TIFF files
    # IPlug expects the original PNG filenames at runtime
    set_target_properties(${target} PROPERTIES
      XCODE_ATTRIBUTE_COMBINE_HIDPI_IMAGES "NO"
    )

    # Apply the ObjC prefix header (works for both Xcode and non-Xcode generators)
    iplug_apply_objc_prefix_header(${target})

    # Configure WebView sources if present (requires ARC)
    # This handles targets that link to iPlug2::Extras::IWebViewControl
    if(DEFINED IPLUG2_WEBVIEW_MAC_SOURCE)
      iplug_configure_webview_sources(${target})
    endif()
  endif()

  # Include and configure based on target type
  include("${IPLUG2_CMAKE_DIR}/${target_type}.cmake")

  string(TOLOWER ${target_type} lowercase_type)
  string(REPLACE "_" "-" function_suffix ${lowercase_type})
  cmake_language(CALL iplug_configure_${function_suffix} ${target} ${project_name})

  # Auto-deploy main plugin formats (skip AUv3 intermediate targets)
  set(DEPLOYABLE_TYPES APP VST2 VST3 CLAP AUv2 AAX)
  if(${target_type} IN_LIST DEPLOYABLE_TYPES)
    iplug_deploy_target(${target} ${target_type} ${project_name})
  endif()

  # Debuggable plugin types
  set(DEBUGGABLE_TYPES "")
  if(IPLUG2_VST2_SUPPORTED)
    list(APPEND DEBUGGABLE_TYPES VST2)
  endif()
  if(IPLUG2_VST3_SUPPORTED)
    list(APPEND DEBUGGABLE_TYPES VST3)
  endif()
  if(IPLUG2_CLAP_SUPPORTED)
    list(APPEND DEBUGGABLE_TYPES CLAP)
  endif()
  if(IPLUG2_AAX_SUPPORTED)
    list(APPEND DEBUGGABLE_TYPES AAX)
  endif()
  if(APPLE)
    list(APPEND DEBUGGABLE_TYPES AUv2 AUv3Appex)
  endif()

  # Set Visual Studio debugger properties for plugin targets (Windows only)
  if(WIN32 AND IPLUG2_DEBUG_HOST AND ${target_type} IN_LIST DEBUGGABLE_TYPES)
    set_target_properties(${target} PROPERTIES
      VS_DEBUGGER_COMMAND "${IPLUG2_DEBUG_HOST}"
    )
    if(IPLUG2_DEBUG_HOST_ARGS)
      set_target_properties(${target} PROPERTIES
        VS_DEBUGGER_COMMAND_ARGUMENTS "${IPLUG2_DEBUG_HOST_ARGS}"
      )
    endif()
    get_filename_component(_debug_host_dir "${IPLUG2_DEBUG_HOST}" DIRECTORY)
    if(_debug_host_dir)
      set_target_properties(${target} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${_debug_host_dir}"
      )
    endif()
  endif()

  # Set Xcode scheme properties for plugin targets (macOS only)
  # Generates schemes with "Ask on Launch" behavior (no executable set)
  # or uses IPLUG2_DEBUG_HOST if provided
  if(APPLE AND XCODE AND ${target_type} IN_LIST DEBUGGABLE_TYPES)
    set_target_properties(${target} PROPERTIES
      XCODE_GENERATE_SCHEME TRUE
      XCODE_SCHEME_DEBUG_DOCUMENT_VERSIONING NO
    )
    if(IPLUG2_DEBUG_HOST)
      set_target_properties(${target} PROPERTIES
        XCODE_SCHEME_EXECUTABLE "${IPLUG2_DEBUG_HOST}"
      )
      if(IPLUG2_DEBUG_HOST_ARGS)
        set_target_properties(${target} PROPERTIES
          XCODE_SCHEME_ARGUMENTS "${IPLUG2_DEBUG_HOST_ARGS}"
        )
      endif()
    endif()
  endif()

  # APP targets are standalone executables - always generate schemes (no debug host needed)
  if(APPLE AND XCODE AND ${target_type} STREQUAL "APP")
    set_target_properties(${target} PROPERTIES
      XCODE_GENERATE_SCHEME TRUE
      XCODE_SCHEME_DEBUG_DOCUMENT_VERSIONING NO
    )
  endif()
endfunction()