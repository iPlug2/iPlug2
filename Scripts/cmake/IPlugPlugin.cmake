#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# IPlugPlugin.cmake
# Provides iplug_add_plugin() macro to simplify example/project CMakeLists.txt files
# Reduces ~190 lines of boilerplate to ~15 lines

#[=============================================================================[
iplug_add_plugin(<name>
  SOURCES <file1> [file2 ...]
  [RESOURCES <file1> [file2 ...]]        # Goes into Resources/
  [WEB_RESOURCES <file1> [file2 ...]]    # Goes into Resources/web/ (for WebView UI)
  [FORMATS <format1> [format2 ...]]      # Defaults to ALL
  [EXCLUDE_FORMATS <format1> [format2 ...]]
  [LINK <library1> [library2 ...]]       # Extra dependencies
  [DEFINES <def1> [def2 ...]]
  [UI <IGRAPHICS|WEBVIEW|NONE>]          # Defaults to IGRAPHICS
  [WAM_SITE_ORIGIN <origin>]             # Defaults to "/"
)

Format names:
  APP      - Standalone application (no embedded AUv3 by default)
  VST2     - VST2 plugin (if SDK available) - DEPRECATED
  VST3     - VST3 plugin
  CLAP     - CLAP plugin (if SDK available)
  AAX      - AAX plugin (if SDK available)
  AU       - AUv2 (macOS only)
  AUV3     - AUv3 with framework/appex/embedding (macOS + iOS) - OPT-IN
  WAM      - Web Audio Module (Emscripten only)
  WASM   - Wasm Web (split DSP/UI modules, Emscripten only)

Format groups:
  ALL            - All formats (APP, VST2, VST3, CLAP, AAX, AU, AUV3, WAM, WASM)
  ALL_PLUGINS    - All plugin formats without APP (VST2, VST3, CLAP, AAX, AU, AUV3, WAM, WASM)
  ALL_DESKTOP    - All desktop formats including AUv3 (APP, VST2, VST3, CLAP, AAX, AU, AUV3)
  MINIMAL_PLUGINS - Core plugin formats (VST3, CLAP, AU)
  DESKTOP        - Desktop formats without AUv3/VST2 (APP, VST3, CLAP, AAX, AU)
  WEB            - Web formats only (WAM, WASM)
#]=============================================================================]

# ============================================================================
# Expand format groups to individual formats
# ============================================================================
function(_iplug_expand_formats input_formats output_var)
  set(result)
  foreach(fmt ${input_formats})
    if(fmt STREQUAL "ALL")
      list(APPEND result APP VST2 VST3 CLAP AAX AU AUV3 WAM WASM)
    elseif(fmt STREQUAL "ALL_PLUGINS")
      list(APPEND result VST2 VST3 CLAP AAX AU AUV3 WAM WASM)
    elseif(fmt STREQUAL "ALL_DESKTOP")
      list(APPEND result APP VST2 VST3 CLAP AAX AU AUV3)
    elseif(fmt STREQUAL "MINIMAL_PLUGINS")
      list(APPEND result VST3 CLAP AU)
    elseif(fmt STREQUAL "DESKTOP")
      list(APPEND result APP VST3 CLAP AAX AU)
    elseif(fmt STREQUAL "WEB")
      list(APPEND result WAM WASM)
    else()
      list(APPEND result ${fmt})
    endif()
  endforeach()
  list(REMOVE_DUPLICATES result)
  set(${output_var} ${result} PARENT_SCOPE)
endfunction()

# ============================================================================
# Add resources to a bundle target (macOS/iOS)
# ============================================================================
function(_iplug_add_resources target resources)
  if(NOT resources)
    return()
  endif()
  target_sources(${target} PRIVATE ${resources})
  set_source_files_properties(${resources} PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources
  )
endfunction()

# ============================================================================
# Create APP, VST3, CLAP, AAX targets (macOS/Windows only)
# ============================================================================
function(_iplug_create_desktop_targets plugin_name formats sources ui_lib resources web_resources base_lib)
  # Skip on iOS and Emscripten
  if(IOS OR CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    return()
  endif()

  # APP target (uses add_executable, needs .rc on Windows)
  if("APP" IN_LIST formats)
    set(_app_sources ${sources})
    set(_rc_file "${CMAKE_CURRENT_SOURCE_DIR}/resources/main.rc")
    if(WIN32 AND EXISTS "${_rc_file}")
      list(APPEND _app_sources "${_rc_file}")
      # Tell RC compiler where to find resources (fonts, images, etc.)
      # The .rc file references files like "Roboto-Regular.ttf" without path
      set_source_files_properties("${_rc_file}" PROPERTIES
        COMPILE_FLAGS "/I\"${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts\" /I\"${CMAKE_CURRENT_SOURCE_DIR}/resources/img\" /I\"${CMAKE_CURRENT_SOURCE_DIR}/resources\""
      )
    endif()
    add_executable(${plugin_name}-app ${_app_sources})
    iplug_add_target(${plugin_name}-app PUBLIC
      LINK iPlug2::APP ${ui_lib} ${base_lib}
    )
    iplug_configure_target(${plugin_name}-app APP ${plugin_name})
    _iplug_add_resources(${plugin_name}-app "${resources}")
    _iplug_add_web_resources(${plugin_name}-app "${web_resources}")
  endif()

  # VST2 (conditional on SDK availability - deprecated)
  if("VST2" IN_LIST formats AND IPLUG2_VST2_SUPPORTED)
    add_library(${plugin_name}-vst2 MODULE ${sources})
    iplug_add_target(${plugin_name}-vst2 PUBLIC
      LINK iPlug2::VST2 ${ui_lib} ${base_lib}
    )
    iplug_configure_target(${plugin_name}-vst2 VST2 ${plugin_name})
    _iplug_add_resources(${plugin_name}-vst2 "${resources}")
    _iplug_add_web_resources(${plugin_name}-vst2 "${web_resources}")
  endif()

  # VST3 (always available)
  if("VST3" IN_LIST formats)
    add_library(${plugin_name}-vst3 MODULE ${sources})
    iplug_add_target(${plugin_name}-vst3 PUBLIC
      LINK iPlug2::VST3 ${ui_lib} ${base_lib}
    )
    iplug_configure_target(${plugin_name}-vst3 VST3 ${plugin_name})
    _iplug_add_resources(${plugin_name}-vst3 "${resources}")
    _iplug_add_web_resources(${plugin_name}-vst3 "${web_resources}")
  endif()

  # CLAP (conditional on SDK availability)
  if("CLAP" IN_LIST formats AND IPLUG2_CLAP_SUPPORTED)
    add_library(${plugin_name}-clap MODULE ${sources})
    iplug_add_target(${plugin_name}-clap PUBLIC
      LINK iPlug2::CLAP ${ui_lib} ${base_lib}
    )
    iplug_configure_target(${plugin_name}-clap CLAP ${plugin_name})
    _iplug_add_resources(${plugin_name}-clap "${resources}")
    _iplug_add_web_resources(${plugin_name}-clap "${web_resources}")
  endif()

  # AAX (conditional on SDK availability)
  if("AAX" IN_LIST formats AND IPLUG2_AAX_SUPPORTED)
    add_library(${plugin_name}-aax MODULE ${sources})
    iplug_add_target(${plugin_name}-aax PUBLIC
      LINK iPlug2::AAX ${ui_lib} ${base_lib}
    )
    iplug_configure_target(${plugin_name}-aax AAX ${plugin_name})
    _iplug_add_resources(${plugin_name}-aax "${resources}")
    _iplug_add_web_resources(${plugin_name}-aax "${web_resources}")
  endif()
endfunction()

# ============================================================================
# Create AUv2 target (macOS only)
# ============================================================================
function(_iplug_create_au_targets plugin_name formats sources ui_lib resources web_resources base_lib)
  # AUv2 is macOS desktop only
  if(NOT APPLE OR IOS)
    return()
  endif()

  if("AU" IN_LIST formats)
    add_library(${plugin_name}-au MODULE ${sources})
    iplug_add_target(${plugin_name}-au PUBLIC
      LINK iPlug2::AUv2 ${ui_lib} ${base_lib}
    )
    iplug_configure_target(${plugin_name}-au AUv2 ${plugin_name})
    _iplug_add_resources(${plugin_name}-au "${resources}")
    _iplug_add_web_resources(${plugin_name}-au "${web_resources}")
  endif()
endfunction()

# ============================================================================
# Create AUv3 targets (macOS only) - Framework + Appex + embedding in APP
# This is OPT-IN - only created if AUV3 is explicitly in formats
# ============================================================================
function(_iplug_create_auv3_targets plugin_name formats sources ui_lib resources web_resources base_lib)
  # AUv3 on macOS requires explicit opt-in
  if(NOT APPLE OR IOS)
    return()
  endif()

  if(NOT "AUV3" IN_LIST formats)
    return()
  endif()

  # Framework containing AUv3 plugin code
  add_library(${plugin_name}AU-framework SHARED ${sources})
  iplug_add_target(${plugin_name}AU-framework PUBLIC
    LINK iPlug2::AUv3 ${ui_lib} ${base_lib}
  )
  iplug_configure_target(${plugin_name}AU-framework AUv3Framework ${plugin_name})
  # Add resources to framework for macOS sandbox compatibility
  # (AUv3 appex cannot access container app's resources in sandbox)
  _iplug_add_resources(${plugin_name}AU-framework "${resources}")
  _iplug_add_web_resources(${plugin_name}AU-framework "${web_resources}")

  # App Extension (appex) - executable using NSExtensionMain entry point
  iplug_get_auv3appex_source(${plugin_name} APPEX_SOURCE)
  add_executable(${plugin_name}AUv3-appex ${APPEX_SOURCE})
  iplug_configure_target(${plugin_name}AUv3-appex AUv3Appex ${plugin_name})

  # Embed AUv3 appex in the APP target if it exists
  if("APP" IN_LIST formats AND TARGET ${plugin_name}-app)
    iplug_embed_auv3_in_app(${plugin_name}-app ${plugin_name})
  endif()
endfunction()

# ============================================================================
# Create iOS targets - AUv3 Framework + Appex + Standalone App
# iOS always builds AUv3 (it's the only plugin format for iOS)
# ============================================================================
function(_iplug_create_ios_targets plugin_name formats sources ui_lib resources web_resources base_lib)
  if(NOT IOS)
    return()
  endif()

  # iOS AUv3 Framework containing plugin code
  # Note: Resources are NOT added to the framework - the appex loads them
  # from the parent app's Resources folder (see IPlugPaths.mm)
  add_library(${plugin_name}AU-ios-framework SHARED ${sources})
  iplug_add_target(${plugin_name}AU-ios-framework PUBLIC
    LINK iPlug2::AUv3iOS ${ui_lib} ${base_lib}
  )
  iplug_configure_target(${plugin_name}AU-ios-framework AUv3iOSFramework ${plugin_name})

  # iOS AUv3 App Extension (appex)
  iplug_get_auv3ios_appex_source(${plugin_name} IOS_APPEX_SOURCE)
  add_executable(${plugin_name}AUv3-ios-appex ${IOS_APPEX_SOURCE})
  iplug_configure_target(${plugin_name}AUv3-ios-appex AUv3iOSAppex ${plugin_name})

  # iOS Standalone App that hosts the AUv3 for testing
  add_executable(${plugin_name}-ios-app ${sources})
  iplug_add_target(${plugin_name}-ios-app PUBLIC
    LINK iPlug2::AUv3iOS ${ui_lib} ${base_lib}
  )
  iplug_configure_target(${plugin_name}-ios-app IOSApp ${plugin_name})
  _iplug_add_resources(${plugin_name}-ios-app "${resources}")
  _iplug_add_web_resources(${plugin_name}-ios-app "${web_resources}")

  # Embed AUv3 appex and framework in the iOS app
  iplug_embed_auv3ios_in_app(${plugin_name}-ios-app ${plugin_name})
endfunction()

# ============================================================================
# Create WAM/Web targets (Emscripten only)
# ============================================================================
function(_iplug_create_wam_targets plugin_name formats sources site_origin base_lib)
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    return()
  endif()

  if(NOT "WAM" IN_LIST formats)
    return()
  endif()

  # Default site origin
  if(NOT site_origin)
    set(site_origin "/")
  endif()

  # WAM Processor target (DSP/audio worklet)
  add_executable(${plugin_name}-wam ${sources})
  iplug_add_target(${plugin_name}-wam PUBLIC
    LINK iPlug2::WAM ${base_lib}
  )
  iplug_configure_target(${plugin_name}-wam WAM ${plugin_name})

  # Web Controller target (UI/graphics)
  add_executable(${plugin_name}-web ${sources})
  iplug_add_target(${plugin_name}-web PUBLIC
    LINK iPlug2::Web ${base_lib}
  )
  iplug_configure_target(${plugin_name}-web Web ${plugin_name})

  # Combined WAM distribution target
  iplug_build_wam_dist(${plugin_name}
    WAM_TARGET ${plugin_name}-wam
    WEB_TARGET ${plugin_name}-web
    SITE_ORIGIN "${site_origin}"
  )
endfunction()

# ============================================================================
# Create Wasm Web targets (Emscripten only)
# Split architecture: DSP runs in AudioWorklet, UI runs on main thread
# ============================================================================
function(_iplug_create_wasm_targets plugin_name formats sources base_lib)
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    return()
  endif()

  if(NOT "WASM" IN_LIST formats)
    return()
  endif()

  # WasmDSP target (DSP/AudioWorklet)
  add_executable(${plugin_name}-wasm-dsp ${sources})
  iplug_add_target(${plugin_name}-wasm-dsp PUBLIC
    LINK iPlug2::WasmDSP ${base_lib}
  )
  iplug_configure_wasm_dsp(${plugin_name}-wasm-dsp ${plugin_name})

  # WasmUI target (UI/IGraphics on main thread)
  add_executable(${plugin_name}-wasm-ui ${sources})
  iplug_add_target(${plugin_name}-wasm-ui PUBLIC
    LINK iPlug2::WasmUI ${base_lib}
  )
  iplug_configure_wasm_ui(${plugin_name}-wasm-ui ${plugin_name})

  # Combined Wasm distribution target
  iplug_build_wasm_dist(${plugin_name}
    DSP_TARGET ${plugin_name}-wasm-dsp
    UI_TARGET ${plugin_name}-wasm-ui
  )
endfunction()

# ============================================================================
# Add web resources to a bundle target (preserves subdirectory structure under Resources/web/)
# ============================================================================
function(_iplug_add_web_resources target resources)
  if(NOT resources)
    return()
  endif()
  target_sources(${target} PRIVATE ${resources})
  foreach(resource ${resources})
    # Get the path relative to resources/web/ to preserve subdirectories
    get_filename_component(_abs_path "${resource}" ABSOLUTE)
    string(FIND "${_abs_path}" "/resources/web/" _web_pos)
    if(_web_pos GREATER -1)
      math(EXPR _start "${_web_pos} + 15")  # length of "/resources/web/"
      string(SUBSTRING "${_abs_path}" ${_start} -1 _rel_path)
      get_filename_component(_subdir "${_rel_path}" DIRECTORY)
      if(_subdir)
        set(_location "Resources/web/${_subdir}")
      else()
        set(_location "Resources/web")
      endif()
    else()
      set(_location "Resources/web")
    endif()
    set_source_files_properties(${resource} PROPERTIES
      MACOSX_PACKAGE_LOCATION "${_location}"
    )
  endforeach()
endfunction()

# ============================================================================
# Main macro: iplug_add_plugin
# ============================================================================
macro(iplug_add_plugin plugin_name)
  cmake_parse_arguments(PLUGIN
    ""
    "UI;WAM_SITE_ORIGIN"
    "SOURCES;RESOURCES;WEB_RESOURCES;FORMATS;EXCLUDE_FORMATS;LINK;DEFINES"
    ${ARGN}
  )

  # ---------------------------------------------------------------------------
  # Input validation
  # ---------------------------------------------------------------------------

  # Check for unparsed/unknown arguments
  if(PLUGIN_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "iplug_add_plugin(${plugin_name}): Unknown arguments: ${PLUGIN_UNPARSED_ARGUMENTS}\n"
      "Valid options: SOURCES, RESOURCES, WEB_RESOURCES, FORMATS, EXCLUDE_FORMATS, LINK, DEFINES, UI, WAM_SITE_ORIGIN")
  endif()

  # Validate plugin name
  if("${plugin_name}" STREQUAL "")
    message(FATAL_ERROR "iplug_add_plugin: Plugin name is required")
  endif()
  if("${plugin_name}" MATCHES "^[0-9]" OR "${plugin_name}" MATCHES "[^a-zA-Z0-9_-]")
    message(FATAL_ERROR "iplug_add_plugin: Invalid plugin name '${plugin_name}'\n"
      "Plugin name must start with a letter and contain only alphanumeric characters, underscores, and hyphens")
  endif()

  # Validate SOURCES (required)
  if(NOT PLUGIN_SOURCES)
    message(FATAL_ERROR "iplug_add_plugin(${plugin_name}): SOURCES is required")
  endif()

  # Validate UI type
  set(_iplug_valid_ui_types IGRAPHICS WEBVIEW NONE)
  if(PLUGIN_UI AND NOT PLUGIN_UI IN_LIST _iplug_valid_ui_types)
    message(FATAL_ERROR "iplug_add_plugin(${plugin_name}): Invalid UI type '${PLUGIN_UI}'\n"
      "Valid UI types: ${_iplug_valid_ui_types}")
  endif()

  # Validate FORMATS
  set(_iplug_valid_formats APP VST2 VST3 CLAP AAX AU AUV3 WAM WASM)
  set(_iplug_valid_format_groups ALL ALL_PLUGINS ALL_DESKTOP MINIMAL_PLUGINS DESKTOP WEB)
  if(PLUGIN_FORMATS)
    foreach(_fmt ${PLUGIN_FORMATS})
      if(NOT _fmt IN_LIST _iplug_valid_formats AND NOT _fmt IN_LIST _iplug_valid_format_groups)
        message(FATAL_ERROR "iplug_add_plugin(${plugin_name}): Invalid format '${_fmt}'\n"
          "Valid formats: ${_iplug_valid_formats}\n"
          "Valid format groups: ${_iplug_valid_format_groups}")
      endif()
    endforeach()
  endif()

  # Validate EXCLUDE_FORMATS
  if(PLUGIN_EXCLUDE_FORMATS)
    foreach(_fmt ${PLUGIN_EXCLUDE_FORMATS})
      if(NOT _fmt IN_LIST _iplug_valid_formats)
        message(FATAL_ERROR "iplug_add_plugin(${plugin_name}): Invalid exclude format '${_fmt}'\n"
          "Valid formats for exclusion: ${_iplug_valid_formats}")
      endif()
    endforeach()
  endif()

  # Set defaults
  if(NOT PLUGIN_UI)
    set(PLUGIN_UI "IGRAPHICS")
  endif()
  if(NOT PLUGIN_FORMATS)
    set(PLUGIN_FORMATS "ALL")
  endif()

  # Expand format groups and apply exclusions
  _iplug_expand_formats("${PLUGIN_FORMATS}" _iplug_formats)
  if(PLUGIN_EXCLUDE_FORMATS)
    list(REMOVE_ITEM _iplug_formats ${PLUGIN_EXCLUDE_FORMATS})
  endif()

  # Determine UI library and include necessary modules
  if(PLUGIN_UI STREQUAL "IGRAPHICS")
    set(_iplug_ui_lib ${IGRAPHICS_LIB})
  elseif(PLUGIN_UI STREQUAL "WEBVIEW")
    include(${IPLUG2_CMAKE_DIR}/WebView.cmake)
    set(_iplug_ui_lib iPlug2::WebView)
  else()
    set(_iplug_ui_lib "")
  endif()

  # Standard project setup
  set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  set(PLUG_RESOURCES_DIR ${PROJECT_DIR}/resources)

  # Create base interface library for shared configuration
  add_library(_${plugin_name}-base INTERFACE)
  iplug_add_target(_${plugin_name}-base INTERFACE
    INCLUDE ${PROJECT_DIR} ${PROJECT_DIR}/resources
    LINK iPlug2::IPlug ${PLUGIN_LINK}
    DEFINE ${PLUGIN_DEFINES}
  )

  # Create format targets
  _iplug_create_desktop_targets(${plugin_name} "${_iplug_formats}" "${PLUGIN_SOURCES}" "${_iplug_ui_lib}" "${PLUGIN_RESOURCES}" "${PLUGIN_WEB_RESOURCES}" "_${plugin_name}-base")
  _iplug_create_au_targets(${plugin_name} "${_iplug_formats}" "${PLUGIN_SOURCES}" "${_iplug_ui_lib}" "${PLUGIN_RESOURCES}" "${PLUGIN_WEB_RESOURCES}" "_${plugin_name}-base")
  _iplug_create_auv3_targets(${plugin_name} "${_iplug_formats}" "${PLUGIN_SOURCES}" "${_iplug_ui_lib}" "${PLUGIN_RESOURCES}" "${PLUGIN_WEB_RESOURCES}" "_${plugin_name}-base")
  _iplug_create_ios_targets(${plugin_name} "${_iplug_formats}" "${PLUGIN_SOURCES}" "${_iplug_ui_lib}" "${PLUGIN_RESOURCES}" "${PLUGIN_WEB_RESOURCES}" "_${plugin_name}-base")
  _iplug_create_wam_targets(${plugin_name} "${_iplug_formats}" "${PLUGIN_SOURCES}" "${PLUGIN_WAM_SITE_ORIGIN}" "_${plugin_name}-base")
  _iplug_create_wasm_targets(${plugin_name} "${_iplug_formats}" "${PLUGIN_SOURCES}" "_${plugin_name}-base")
endmacro()
