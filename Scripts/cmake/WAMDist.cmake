#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# WAMDist.cmake - WAM distribution build orchestration
#
# This module provides functions for building a complete WAM distribution:
# - Resource bundling (fonts, images, SVGs)
# - JavaScript post-processing
# - Template copying and configuration
# - Build orchestration

# Only available with Emscripten toolchain
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  return()
endif()

# Find Python (required for file_packager.py and template configuration)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Path to iPlug2 helper scripts
set(IPLUG2_SCRIPTS_DIR ${IPLUG2_DIR}/Scripts)
set(IPLUG2_CMAKE_SCRIPTS_DIR ${IPLUG2_SCRIPTS_DIR}/cmake)

# WAM template and SDK paths
set(WAM_TEMPLATE_DIR ${IPLUG2_DIR}/IPlug/WEB/Template)
set(WAM_SDK_DIR ${IPLUG_DEPS_DIR}/WAM_SDK/wamsdk)
set(WAM_AWP_DIR ${IPLUG_DEPS_DIR}/WAM_AWP)

# ============================================================================
# Parse channel I/O from config.h
# ============================================================================
function(iplug_parse_channel_io project_dir out_max_inputs out_max_outputs)
  execute_process(
    COMMAND ${Python3_EXECUTABLE} ${IPLUG2_SCRIPTS_DIR}/parse_iostr.py ${project_dir} inputs
    OUTPUT_VARIABLE max_in
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE result_in
  )
  execute_process(
    COMMAND ${Python3_EXECUTABLE} ${IPLUG2_SCRIPTS_DIR}/parse_iostr.py ${project_dir} outputs
    OUTPUT_VARIABLE max_out
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE result_out
  )

  if(NOT result_in EQUAL 0)
    message(WARNING "Failed to parse inputs from config.h, defaulting to 0")
    set(max_in 0)
  endif()
  if(NOT result_out EQUAL 0)
    message(WARNING "Failed to parse outputs from config.h, defaulting to 2")
    set(max_out 2)
  endif()

  set(${out_max_inputs} ${max_in} PARENT_SCOPE)
  set(${out_max_outputs} ${max_out} PARENT_SCOPE)
endfunction()

# ============================================================================
# Bundle resources using Emscripten's file_packager.py
# ============================================================================
function(iplug_bundle_web_resources project_name resource_dir output_dir)
  # Find file_packager.py from EMSDK
  if(NOT DEFINED ENV{EMSDK})
    message(WARNING "EMSDK environment variable not set - resource bundling may fail")
    set(FILE_PACKAGER "file_packager.py")
  else()
    set(FILE_PACKAGER "$ENV{EMSDK}/upstream/emscripten/tools/file_packager.py")
  endif()

  # Output files
  set(FONTS_DATA "${output_dir}/fonts.data")
  set(FONTS_JS "${output_dir}/fonts.js")
  set(IMGS_DATA "${output_dir}/imgs.data")
  set(IMGS_JS "${output_dir}/imgs.js")
  set(SVGS_DATA "${output_dir}/svgs.data")
  set(SVGS_JS "${output_dir}/svgs.js")
  set(IMGS2X_DATA "${output_dir}/imgs@2x.data")
  set(IMGS2X_JS "${output_dir}/imgs@2x.js")

  # Create resource bundling commands
  set(RESOURCE_OUTPUTS "")
  set(RESOURCE_COMMANDS "")

  # Bundle fonts
  if(EXISTS "${resource_dir}/fonts")
    list(APPEND RESOURCE_OUTPUTS ${FONTS_DATA} ${FONTS_JS})
    add_custom_command(
      OUTPUT ${FONTS_DATA} ${FONTS_JS}
      COMMAND ${Python3_EXECUTABLE} ${FILE_PACKAGER} ${FONTS_DATA}
        --preload "${resource_dir}/fonts/"
        --exclude "*DS_Store"
        --js-output=${FONTS_JS}
      WORKING_DIRECTORY ${output_dir}
      COMMENT "Bundling fonts for ${project_name}"
      VERBATIM
    )
  endif()

  # Bundle SVGs
  file(GLOB SVG_FILES "${resource_dir}/img/*.svg")
  if(SVG_FILES)
    list(APPEND RESOURCE_OUTPUTS ${SVGS_DATA} ${SVGS_JS})
    add_custom_command(
      OUTPUT ${SVGS_DATA} ${SVGS_JS}
      COMMAND ${Python3_EXECUTABLE} ${FILE_PACKAGER} ${SVGS_DATA}
        --preload "${resource_dir}/img/"
        --exclude "*.png"
        --exclude "*DS_Store"
        --js-output=${SVGS_JS}
      WORKING_DIRECTORY ${output_dir}
      COMMENT "Bundling SVGs for ${project_name}"
      VERBATIM
    )
  endif()

  # Bundle @1x PNGs
  file(GLOB PNG_FILES "${resource_dir}/img/*.png")
  # Filter out @2x files
  list(FILTER PNG_FILES EXCLUDE REGEX "@2x")
  if(PNG_FILES)
    list(APPEND RESOURCE_OUTPUTS ${IMGS_DATA} ${IMGS_JS})
    add_custom_command(
      OUTPUT ${IMGS_DATA} ${IMGS_JS}
      COMMAND ${Python3_EXECUTABLE} ${FILE_PACKAGER} ${IMGS_DATA}
        --use-preload-plugins
        --preload "${resource_dir}/img/"
        --use-preload-cache
        --indexedDB-name="/${project_name}_pkg"
        --exclude "*DS_Store"
        --exclude "*@2x.png"
        --exclude "*.svg"
        --js-output=${IMGS_JS}
      WORKING_DIRECTORY ${output_dir}
      COMMENT "Bundling images for ${project_name}"
      VERBATIM
    )
  endif()

  # Bundle @2x PNGs
  file(GLOB PNG2X_FILES "${resource_dir}/img/*@2x*.png")
  if(PNG2X_FILES)
    list(APPEND RESOURCE_OUTPUTS ${IMGS2X_DATA} ${IMGS2X_JS})
    add_custom_command(
      OUTPUT ${IMGS2X_DATA} ${IMGS2X_JS}
      COMMAND ${CMAKE_COMMAND} -E make_directory "${output_dir}/2x"
      COMMAND ${CMAKE_COMMAND} -E copy ${PNG2X_FILES} "${output_dir}/2x/"
      COMMAND ${Python3_EXECUTABLE} ${FILE_PACKAGER} ${IMGS2X_DATA}
        --use-preload-plugins
        --preload "${output_dir}/2x@/resources/img/"
        --use-preload-cache
        --indexedDB-name="/${project_name}_pkg"
        --exclude "*DS_Store"
        --js-output=${IMGS2X_JS}
      COMMAND ${CMAKE_COMMAND} -E remove_directory "${output_dir}/2x"
      WORKING_DIRECTORY ${output_dir}
      COMMENT "Bundling @2x images for ${project_name}"
      VERBATIM
    )
  endif()

  # Create custom target for resource bundling
  if(RESOURCE_OUTPUTS)
    add_custom_target(${project_name}_wam_resources
      DEPENDS ${RESOURCE_OUTPUTS}
    )
    set(${project_name}_WAM_RESOURCES_TARGET ${project_name}_wam_resources PARENT_SCOPE)
  endif()
endfunction()

# ============================================================================
# Post-process WAM JS with AudioWorkletGlobalScope wrapper
# ============================================================================
function(iplug_postprocess_wam_js wam_target project_name output_dir)
  set(WAM_JS "${output_dir}/scripts/${project_name}-wam.js")
  set(WAM_JS_TMP "${WAM_JS}.tmp")

  # The WAM JS needs to be wrapped with AudioWorkletGlobalScope setup
  # This prefix is prepended to the emscripten output
  set(AWG_PREFIX "AudioWorkletGlobalScope.WAM = AudioWorkletGlobalScope.WAM || {}; AudioWorkletGlobalScope.WAM.${project_name} = { ENVIRONMENT: 'WEB' }; const ModuleFactory = AudioWorkletGlobalScope.WAM.${project_name};")

  add_custom_command(TARGET ${wam_target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "${AWG_PREFIX}" > "${WAM_JS_TMP}"
    COMMAND ${CMAKE_COMMAND} -E cat "${WAM_JS}" >> "${WAM_JS_TMP}"
    COMMAND ${CMAKE_COMMAND} -E rename "${WAM_JS_TMP}" "${WAM_JS}"
    COMMENT "Post-processing ${project_name}-wam.js for AudioWorkletGlobalScope"
    VERBATIM
  )
endfunction()

# ============================================================================
# Copy WAM SDK and polyfill scripts
# ============================================================================
function(iplug_copy_wam_sdk_scripts web_target output_dir)
  add_custom_command(TARGET ${web_target} POST_BUILD
    # WAM SDK scripts
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${WAM_SDK_DIR}/wam-controller.js"
      "${output_dir}/scripts/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${WAM_SDK_DIR}/wam-processor.js"
      "${output_dir}/scripts/"
    # AudioWorklet polyfill
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${WAM_AWP_DIR}/audioworklet.js"
      "${output_dir}/scripts/"
    COMMENT "Copying WAM SDK scripts"
    VERBATIM
  )
endfunction()

# ============================================================================
# Copy and configure template files (HTML, JS, CSS)
# ============================================================================
function(iplug_copy_web_templates web_target project_name output_dir site_origin max_inputs max_outputs)
  set(CONFIGURE_SCRIPT "${IPLUG2_CMAKE_SCRIPTS_DIR}/configure_web_template.py")

  # Configure index.html
  add_custom_command(TARGET ${web_target} POST_BUILD
    COMMAND ${Python3_EXECUTABLE} "${CONFIGURE_SCRIPT}"
      --input "${WAM_TEMPLATE_DIR}/index.html"
      --output "${output_dir}/index.html"
      --name "${project_name}"
      --origin "${site_origin}"
      --max-inputs "${max_inputs}"
      --max-outputs "${max_outputs}"
    COMMENT "Configuring index.html for ${project_name}"
    VERBATIM
  )

  # Configure AWN (Audio Worklet Node) script
  add_custom_command(TARGET ${web_target} POST_BUILD
    COMMAND ${Python3_EXECUTABLE} "${CONFIGURE_SCRIPT}"
      --input "${WAM_TEMPLATE_DIR}/scripts/IPlugWAM-awn.js"
      --output "${output_dir}/scripts/${project_name}-awn.js"
      --name "${project_name}"
      --origin "${site_origin}"
    COMMENT "Configuring ${project_name}-awn.js"
    VERBATIM
  )

  # Configure AWP (Audio Worklet Processor) script
  add_custom_command(TARGET ${web_target} POST_BUILD
    COMMAND ${Python3_EXECUTABLE} "${CONFIGURE_SCRIPT}"
      --input "${WAM_TEMPLATE_DIR}/scripts/IPlugWAM-awp.js"
      --output "${output_dir}/scripts/${project_name}-awp.js"
      --name "${project_name}"
    COMMENT "Configuring ${project_name}-awp.js"
    VERBATIM
  )

  # Copy styles and favicon
  add_custom_command(TARGET ${web_target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${output_dir}/styles"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${WAM_TEMPLATE_DIR}/styles/style.css"
      "${output_dir}/styles/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${WAM_TEMPLATE_DIR}/favicon.ico"
      "${output_dir}/"
    COMMENT "Copying styles and favicon"
    VERBATIM
  )
endfunction()

# ============================================================================
# Build complete WAM distribution
# ============================================================================
function(iplug_build_wam_dist project_name)
  cmake_parse_arguments(PARSE_ARGV 1 WAM_DIST
    ""
    "WAM_TARGET;WEB_TARGET;SITE_ORIGIN;OUTPUT_DIR"
    ""
  )

  # Default values
  if(NOT WAM_DIST_SITE_ORIGIN)
    set(WAM_DIST_SITE_ORIGIN "/")
  endif()
  if(NOT WAM_DIST_OUTPUT_DIR)
    # Use project-specific subdirectory to avoid conflicts when building multiple plugins
    set(WAM_DIST_OUTPUT_DIR "${CMAKE_BINARY_DIR}/out/${project_name}")
  endif()

  set(OUTPUT_DIR ${WAM_DIST_OUTPUT_DIR})
  set(SCRIPTS_DIR "${OUTPUT_DIR}/scripts")

  # Create output directories at configure time
  file(MAKE_DIRECTORY ${OUTPUT_DIR})
  file(MAKE_DIRECTORY ${SCRIPTS_DIR})

  # Get channel I/O info from config.h
  iplug_parse_channel_io(${PROJECT_DIR} MAX_INPUTS MAX_OUTPUTS)
  message(STATUS "${project_name} WAM: max inputs=${MAX_INPUTS}, max outputs=${MAX_OUTPUTS}")

  # 1. Set up WAM processor target output
  set_target_properties(${WAM_DIST_WAM_TARGET} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SCRIPTS_DIR}
    OUTPUT_NAME "${project_name}-wam"
    SUFFIX ".js"
  )

  # 2. Set up Web controller target output
  set_target_properties(${WAM_DIST_WEB_TARGET} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SCRIPTS_DIR}
    OUTPUT_NAME "${project_name}-web"
    SUFFIX ".js"
  )

  # 3. Bundle resources (fonts, images, SVGs)
  iplug_bundle_web_resources(${project_name} ${PLUG_RESOURCES_DIR} ${OUTPUT_DIR})

  # 4. Post-process WAM JS with AudioWorkletGlobalScope wrapper
  iplug_postprocess_wam_js(${WAM_DIST_WAM_TARGET} ${project_name} ${OUTPUT_DIR})

  # 5. Copy WAM SDK scripts
  iplug_copy_wam_sdk_scripts(${WAM_DIST_WEB_TARGET} ${OUTPUT_DIR})

  # 6. Copy and configure template files
  iplug_copy_web_templates(${WAM_DIST_WEB_TARGET} ${project_name} ${OUTPUT_DIR}
    "${WAM_DIST_SITE_ORIGIN}" ${MAX_INPUTS} ${MAX_OUTPUTS})

  # Create aggregate target for convenience
  add_custom_target(${project_name}-wam-dist ALL
    DEPENDS ${WAM_DIST_WAM_TARGET} ${WAM_DIST_WEB_TARGET}
    COMMENT "Building complete WAM distribution for ${project_name}"
  )

  # Add resource dependency if resources exist
  if(TARGET ${project_name}_wam_resources)
    add_dependencies(${project_name}-wam-dist ${project_name}_wam_resources)
  endif()

  message(STATUS "WAM distribution target: ${project_name}-wam-dist")
  message(STATUS "WAM output directory: ${OUTPUT_DIR}")
endfunction()
