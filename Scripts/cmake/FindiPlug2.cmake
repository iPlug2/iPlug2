#  ==============================================================================
#  
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

cmake_minimum_required(VERSION 3.11)
cmake_policy(SET CMP0076 NEW)

# Set iPlug2 as found
set(iPlug2_FOUND 1)

# Define common variables
set(PLUG_NAME ${CMAKE_PROJECT_NAME} CACHE STRING "Name of the App/CLAP/VST/AU etc.")

# Platform-specific settings
if(WIN32)
  set(OS_WINDOWS 1)
  
  # Determine processor architecture for postbuild-win.bat
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64")
    set(PROCESSOR_ARCH "x64" CACHE STRING "Processor architecture")
  else()
    set(PROCESSOR_ARCH "Win32" CACHE STRING "Processor architecture")
  endif()
  
  # Configure postbuild script
  set(CREATE_BUNDLE_SCRIPT "${IPLUG2_DIR}/Scripts/create_bundle.bat")
  set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/out")
  set(IPLUG2_VST_ICON "${IPLUG2_DIR}/Dependencies/IPlug/VST3_SDK/doc/artwork/VST_Logo_Steinberg.ico")
  set(IPLUG2_VST_ICON "${IPLUG2_DIR}/Dependencies/IPlug/VST3_SDK/doc/artwork/VST_Logo_Steinberg.ico")

  configure_file("${IPLUG2_DIR}/Scripts/postbuild-win.bat.in" "${CMAKE_BINARY_DIR}/postbuild-win.bat")

elseif(APPLE)
  set(OS_MAC 1)
  
  # Find required tool
  find_program(IBTOOL ibtool HINTS "/usr/bin" "${OSX_DEVELOPER_ROOT}/usr/bin")
  if(${IBTOOL} STREQUAL "IBTOOL-NOTFOUND")
    message(SEND_ERROR "ibtool cannot be found")
  endif()

elseif(UNIX AND NOT APPLE)
  set(OS_LINUX 1)
  find_package(PkgConfig REQUIRED)

else()
  message(FATAL_ERROR "Unsupported platform")
endif()

# Check for compiler flags
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-march=native" COMPILER_OPT_ARCH_NATIVE_SUPPORTED)

# Helper function to add target properties
function(iplug_target_add target set_type)
  cmake_parse_arguments(cfg "" "" "INCLUDE;SOURCE;DEFINE;OPTION;LINK;LINK_DIR;DEPEND;FEATURE;RESOURCE" ${ARGN})
  
  if(cfg_INCLUDE)
    target_include_directories(${target} ${set_type} ${cfg_INCLUDE})
  endif()
  
  if(cfg_SOURCE)
    target_sources(${target} ${set_type} ${cfg_SOURCE})
  endif()
  
  if(cfg_DEFINE)
    target_compile_definitions(${target} ${set_type} ${cfg_DEFINE})
  endif()
  
  if(cfg_OPTION)
    target_compile_options(${target} ${set_type} ${cfg_OPTION})
  endif()
  
  if(cfg_LINK)
    target_link_libraries(${target} ${set_type} ${cfg_LINK})
  endif()
  
  if(cfg_LINK_DIR)
    target_link_directories(${target} ${set_type} ${cfg_LINK_DIR})
  endif()
  
  if(cfg_DEPEND)
    add_dependencies(${target} ${cfg_DEPEND})
  endif()
  
  if(cfg_FEATURE)
    target_compile_features(${target} ${set_type} ${cfg_FEATURE})
  endif()
  
  if(cfg_RESOURCE)
    set_property(TARGET ${target} APPEND PROPERTY RESOURCE ${cfg_RESOURCE})
  endif()
  
  if(cfg_UNUSED)
    message(FATAL_ERROR "Unused arguments ${cfg_UNUSED}")
  endif()
endfunction()

# Helper macro for ternary operations
macro(iplug_ternary VAR val_true val_false)
  if(${ARGN})
    set(${VAR} ${val_true})
  else()
    set(${VAR} ${val_false})
  endif()
endmacro()

# Helper macro to set up source tree
macro(iplug_source_tree target)
  get_target_property(_tmp ${target} INTERFACE_SOURCES)
  if(NOT "${_tmp}" STREQUAL "_tmp-NOTFOUND")
    source_group(TREE ${IPLUG2_DIR} PREFIX "IPlug" FILES ${_tmp})
  endif()
endmacro()

# Helper function to find paths
function(iplug_find_path VAR)
  cmake_parse_arguments(arg "REQUIRED;DIR;FILE" "DEFAULT_IDX;DEFAULT;DOC" "PATHS" ${ARGN})
  
  if(NOT arg_DIR AND NOT arg_FILE)
    message(FATAL_ERROR "ERROR: iplug_find_path MUST specify either DIR or FILE as an argument")
  endif()

  set(out 0)
  foreach(pt ${arg_PATHS})
    if(EXISTS ${pt})
      iplug_ternary(is_dir 1 0 IS_DIRECTORY ${pt})

      if((arg_FILE AND NOT ${is_dir}) OR (arg_DIR AND ${is_dir}))
        set(out ${pt})
        break()
      endif()
    endif()
  endforeach()

  # Handle default options
  if((NOT out) AND (arg_DEFAULT))
    set(out ${arg_DEFAULT})
  endif()
  if((NOT out) AND NOT ("${arg_DEFAULT_IDX}" STREQUAL ""))
    list(GET arg_PATHS "${arg_DEFAULT_IDX}" out)
  endif()

  # Determine cache type for the variable
  iplug_ternary(_cache_type PATH FILEPATH ${arg_DIR})
  
  # Handle required flag
  if((NOT out) AND (arg_REQUIRED))
    set(${VAR} "${VAR}-NOTFOUND" CACHE ${_cache_type} ${arg_DOC})
    message(FATAL_ERROR "Path ${VAR} not found!")
  endif()
  
  # Set cache var or var in parent scope
  if(arg_DOC)
    set(${VAR} ${out} CACHE ${_cache_type} ${arg_DOC})
  else()
    set(${VAR} ${out} PARENT_SCOPE)
  endif()
endfunction()

# Function to bundle resources
function(iplug_target_bundle_resources target res_dir)
  get_property(resources TARGET ${target} PROPERTY RESOURCE)
  if(CMAKE_GENERATOR STREQUAL "Xcode")
    # On Xcode, mark each file as non-compiled
    foreach(res ${resources})
      get_filename_component(fn "${res}" NAME)
      set(file_type "file")
      if(fn MATCHES ".*\\.xib")
        set(file_type "file.xib")
      endif()
      set_property(SOURCE ${res} PROPERTY XCODE_LAST_KNOWN_FILE_TYPE ${file_type})
    endforeach()
  else()
    # For other generators, manually copy resources
    foreach(res ${resources})
      get_filename_component(fn "${res}" NAME)
      set(copy TRUE)

      set(dst "${res_dir}/${fn}")
      if(NOT APPLE)
        if(fn MATCHES ".*\\.ttf")
          set(dst "${res_dir}/fonts/${fn}")
        elseif((fn MATCHES ".*\\.png") OR (fn MATCHES ".*\\.svg"))
          set(dst "${res_dir}/img/${fn}")
        endif()
      else()
        # Handle .xib files on non-Xcode Apple builds
        if(fn MATCHES ".*\\.xib")
          get_filename_component(tmp "${res}" NAME_WE)
          set(dst "${res_dir}/${tmp}.nib")
          add_custom_command(OUTPUT ${dst}
            COMMAND ${IBTOOL} ARGS "--errors" "--warnings" "--notices" "--compile" "${dst}" "${res}"
            MAIN_DEPENDENCY "${res}")
          set(copy FALSE)
        endif()
      endif()

      target_sources(${target} PUBLIC "${dst}")

      if(copy)
        add_custom_command(OUTPUT "${dst}"
          COMMAND ${CMAKE_COMMAND} ARGS "-E" "copy" "${res}" "${dst}"
          MAIN_DEPENDENCY "${res}")
      endif()
    endforeach()
  endif()
endfunction()

# Function to configure target for specific output type
function(iplug_configure_target target target_type)
  set_property(TARGET ${target} PROPERTY CXX_STANDARD ${IPLUG2_CXX_STANDARD})

  # Platform-specific configurations
  if(WIN32)
    # Windows-specific configurations
    set(_res "${PLUG_RESOURCES_DIR}/main.rc")
    iplug_target_add(${target} PUBLIC RESOURCE ${_res})
    source_group("Resources" FILES ${_res})
    
    # Configure PDB generation
    if(${CMAKE_CXX_COMPILER_FRONTEND_VARIANT} STREQUAL MSVC)
      target_compile_options(${target} PRIVATE /Zi)
    else()
      target_compile_options(${target} PRIVATE -DEBUG)
    endif()
    set_target_properties(${target} PROPERTIES
      COMPILE_PDB_NAME "${target}_${PROCESSOR_ARCH}"
    )
    
  elseif(APPLE)
    # macOS-specific configurations
    set_property(TARGET ${target} PROPERTY OUTPUT_NAME "${PLUG_NAME}")
    set_target_properties(${TARGET} PROPERTIES XCODE_ATTRIBUTE_GENERATE_PKGINFO_FILE "YES")
    
  elseif(UNIX AND NOT APPLE)
    # Linux-specific configurations
    # Add Linux-specific configurations here
  endif()
  
  # Configure target based on type
  if(target_type STREQUAL "aax")
    include("${IPLUG2_CMAKE_DIR}/AAX.cmake")
    iplug_configure_aax(${target})
  elseif(target_type STREQUAL "app")
    include("${IPLUG2_CMAKE_DIR}/APP.cmake")
    iplug_configure_app(${target})
  elseif(target_type STREQUAL "auv2")
    include("${IPLUG2_CMAKE_DIR}/AUv2.cmake")
    iplug_configure_auv2(${target})
  elseif(target_type STREQUAL "vst2")
    include("${IPLUG2_CMAKE_DIR}/VST2.cmake")
    iplug_configure_vst2(${target})
  elseif(target_type STREQUAL "vst3")
    include("${IPLUG2_CMAKE_DIR}/VST3.cmake")
    iplug_configure_vst3(${target})
  elseif(target_type STREQUAL "clap")
    include("${IPLUG2_CMAKE_DIR}/CLAP.cmake")
    iplug_configure_clap(${target})
  else()
    message(FATAL_ERROR "Unknown target type '${target_type}' for target '${target}'")
  endif()
endfunction()

# Set common variables and include directories
set(IPLUG_SRC ${IPLUG2_DIR}/IPlug)
set(IGRAPHICS_SRC ${IPLUG2_DIR}/IGraphics)
set(WDL_DIR ${IPLUG2_DIR}/WDL)
set(IPLUG_DEPS ${IPLUG2_DIR}/Dependencies/IPlug)
set(IGRAPHICS_DEPS ${IPLUG2_DIR}/Dependencies/IGraphics)
set(BUILD_DEPS ${IPLUG2_DIR}/Dependencies/Build)

# Core iPlug2 interface
add_library(iPlug2_Core INTERFACE)

set(_def "NOMINMAX" "$<$<CONFIG:Debug>:DEBUG>" "$<$<CONFIG:Debug>:_DEBUG>")
set(_opts "")
set(_lib "")
set(_inc
  ${WDL_DIR}
  ${WDL_DIR}/libpng
  ${WDL_DIR}/zlib
  ${IPLUG_SRC}
  ${IPLUG_SRC}/Extras
)

set(sdk ${IPLUG_SRC})
set(_src
  ${sdk}/IPlugAPIBase.h
  ${sdk}/IPlugAPIBase.cpp
  ${sdk}/IPlugConstants.h
  ${sdk}/IPlugEditorDelegate.h
  ${sdk}/IPlugLogger.h
  ${sdk}/IPlugMidi.h
  ${sdk}/IPlugParameter.h
  ${sdk}/IPlugParameter.cpp
  ${sdk}/IPlugPaths.h
  ${sdk}/IPlugPaths.cpp
  ${sdk}/IPlugPlatform.h
  ${sdk}/IPlugPluginBase.h
  ${sdk}/IPlugPluginBase.cpp
  ${sdk}/IPlugProcessor.h
  ${sdk}/IPlugProcessor.cpp
  ${sdk}/IPlugQueue.h
  ${sdk}/IPlugStructs.h
  ${sdk}/IPlugTimer.h
  ${sdk}/IPlugTimer.cpp
  ${sdk}/IPlugUtilities.h
)

# Set resource directory
if(NOT PLUG_RESOURCES_DIR)
  set(PLUG_RESOURCES_DIR ${CMAKE_SOURCE_DIR}/resources)
  message(STATUS "Setting PLUG_RESOURCES_DIR to ${PLUG_RESOURCES_DIR}")
endif()

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/out")

# Platform-specific configurations
if(WIN32)
  target_link_libraries(iPlug2_Core INTERFACE "Shlwapi.lib" "comctl32.lib" "wininet.lib")
elseif(UNIX AND NOT APPLE)
  list(APPEND _inc ${WDL_DIR}/swell)
  list(APPEND _lib "pthread" "rt")
  list(APPEND _opts "-Wno-multichar")
elseif(APPLE)
  set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
  set(CMAKE_XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES")

  list(APPEND _src ${IPLUG_SRC}/IPlugPaths.mm)
  list(APPEND _inc ${WDL_DIR}/swell)
  list(APPEND _lib
    "-framework CoreFoundation" "-framework CoreData" "-framework Foundation"
    "-framework CoreServices"
  )
  list(APPEND _opts "-Wno-deprecated-declarations" "-Wno-c++11-narrowing" "-Wno-unused-command-line-argument")
endif()

# Compiler-specific configurations
if(MSVC)
  list(APPEND _def "_CRT_SECURE_NO_WARNINGS" "_CRT_SECURE_NO_DEPRECATE" "_CRT_NONSTDC_NO_DEPRECATE" "NOMINMAX" "_MBCS")
  list(APPEND _opts "/wd4996" "/wd4250" "/wd4018" "/wd4267" "/wd4068" "/MT$<$<CONFIG:Debug>:d>")
endif()

# Set certain compiler flags
if((CMAKE_CXX_COMPILER_ID MATCHES "Clang") OR (CMAKE_CXX_COMPILER_ID MATCHES "GNU"))
  list(APPEND _opts "-Wl,--no-undefined")
endif()

# Configure core iPlug2 interface
source_group(TREE ${IPLUG2_DIR} PREFIX "IPlug" FILES ${_src})
iplug_target_add(iPlug2_Core INTERFACE 
  DEFINE ${_def} 
  INCLUDE ${_inc} 
  SOURCE ${_src} 
  OPTION ${_opts} 
  LINK ${_lib}
)

# Include additional CMake files
include("${IPLUG2_CMAKE_DIR}/IGraphics.cmake")

# Reaper Extension configuration
add_library(iPlug2_REAPER INTERFACE)
set(_sdk ${IPLUG2_DIR}/IPlug/ReaperExt)
iplug_target_add(iPlug2_REAPER INTERFACE
  INCLUDE "${_sdk}" "${IPLUG_DEPS}/IPlug/Reaper"
  SOURCE "${_sdk}/ReaperExtBase.cpp"
  DEFINE "REAPER_PLUGIN"
  LINK iPlug2_VST2
)

# Additional configuration targets
add_library(iPlug2_WebView INTERFACE)
iplug_target_add(iPlug2_WebView INTERFACE
  INCLUDE "${IPLUG2_DIR}/IPlug/Extras/WebView"
  SOURCE "${IPLUG_SRC}/Extras/WebView/IPlugWebView.cpp"
  SOURCE "${IPLUG_SRC}/Extras/WebView/IPlugWebView.mm"
)

add_library(iPlug2_Faust INTERFACE)
iplug_target_add(iPlug2_Faust INTERFACE
  INCLUDE "${IPLUG2_DIR}/IPlug/Extras/Faust" "${FAUST_INCLUDE_DIR}"
)

add_library(iPlug2_FaustGen INTERFACE)
iplug_target_add(iPlug2_FaustGen INTERFACE
  SOURCE "${IPLUG_SRC}/Extras/Faust/IPlugFaustGen.cpp"
  LINK iPlug2_Faust
)
iplug_source_tree(iPlug2_FaustGen)

add_library(iPlug2_HIIR INTERFACE)
iplug_target_add(iPlug2_HIIR INTERFACE
  INCLUDE ${IPLUG_SRC}/Extras/HIIR
  SOURCE "${IPLUG_SRC}/Extras/HIIR/PolyphaseIIR2Designer.cpp"
)
iplug_source_tree(iPlug2_HIIR)

add_library(iPlug2_OSC INTERFACE)
iplug_target_add(iPlug2_OSC INTERFACE
  INCLUDE ${IPLUG_SRC}/Extras/OSC
  SOURCE ${IPLUG_SRC}/Extras/OSC/IPlugOSC_msg.cpp
)
iplug_source_tree(iPlug2_OSC)

add_library(iPlug2_Synth INTERFACE)
iplug_target_add(iPlug2_Synth INTERFACE
  INCLUDE ${IPLUG_SRC}/Extras/Synth
  SOURCE
    "${IPLUG_SRC}/Extras/Synth/MidiSynth.cpp"
    "${IPLUG_SRC}/Extras/Synth/VoiceAllocator.cpp"
)
iplug_source_tree(iPlug2_Synth)

# Function to set up debug properties for macOS xcode targets
function(iplug_target_debug_xcode target_name application_path application_arguments)
  if(CMAKE_GENERATOR STREQUAL "Xcode")
    set_target_properties(${target_name} PROPERTIES
      XCODE_GENERATE_SCHEME TRUE
      XCODE_SCHEME_EXECUTABLE "${application_path}"
      XCODE_SCHEME_ARGUMENTS "${application_arguments}"
      XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/out"
    )
  endif()
endfunction()

# Function to set up debug properties for Windows MSVS targets
function(iplug_target_debug_vs target_name application_path application_arguments)
  if(MSVC)
    set_target_properties(${target_name} PROPERTIES
      VS_DEBUGGER_COMMAND "${application_path}"
      VS_DEBUGGER_COMMAND_ARGUMENTS "${application_arguments}"
      VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/out"
    )
  endif()
endfunction()