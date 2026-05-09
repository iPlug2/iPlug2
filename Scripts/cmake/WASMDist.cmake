#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# WASMDist.cmake - Wasm Web distribution build orchestration
#
# This module provides functions for building a complete Wasm Web distribution:
# - Resource bundling (fonts, images, SVGs)
# - DSP JS post-processing for AudioWorklet scope
# - Template configuration (bundle.js, processor.js, index.html)
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

# Wasm template path
set(WASM_TEMPLATE_DIR ${IPLUG2_DIR}/IPlug/WEB/TemplateWasm)

# ============================================================================
# Parse config.h for plugin configuration
# ============================================================================
function(iplug_parse_wasm_config project_dir)
  # Parse channel I/O
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

  set(WASM_MAX_INPUTS ${max_in} PARENT_SCOPE)
  set(WASM_MAX_OUTPUTS ${max_out} PARENT_SCOPE)

  # Determine if instrument (no audio inputs)
  if(max_in EQUAL 0)
    set(WASM_IS_INSTRUMENT "true" PARENT_SCOPE)
  else()
    set(WASM_IS_INSTRUMENT "false" PARENT_SCOPE)
  endif()

  # Read config.h to check various flags
  set(CONFIG_H "${project_dir}/config.h")
  if(EXISTS ${CONFIG_H})
    file(READ ${CONFIG_H} CONFIG_CONTENT)

    # Check PLUG_HAS_UI
    string(REGEX MATCH "PLUG_HAS_UI[ \t]+1" HAS_UI_MATCH "${CONFIG_CONTENT}")
    if(HAS_UI_MATCH)
      set(WASM_HAS_UI TRUE PARENT_SCOPE)
      set(WASM_HAS_UI_STR "true" PARENT_SCOPE)
    else()
      set(WASM_HAS_UI FALSE PARENT_SCOPE)
      set(WASM_HAS_UI_STR "false" PARENT_SCOPE)
    endif()

    # Check PLUG_HOST_RESIZE
    string(REGEX MATCH "PLUG_HOST_RESIZE[ \t]+1" HOST_RESIZE_MATCH "${CONFIG_CONTENT}")
    if(HOST_RESIZE_MATCH)
      set(WASM_HOST_RESIZE "true" PARENT_SCOPE)
    else()
      set(WASM_HOST_RESIZE "false" PARENT_SCOPE)
    endif()

    # Check PLUG_DOES_MIDI_IN
    string(REGEX MATCH "PLUG_DOES_MIDI_IN[ \t]+1" MIDI_IN_MATCH "${CONFIG_CONTENT}")
    if(MIDI_IN_MATCH)
      set(WASM_DOES_MIDI_IN "true" PARENT_SCOPE)
    else()
      set(WASM_DOES_MIDI_IN "false" PARENT_SCOPE)
    endif()

    # Check PLUG_DOES_MIDI_OUT
    string(REGEX MATCH "PLUG_DOES_MIDI_OUT[ \t]+1" MIDI_OUT_MATCH "${CONFIG_CONTENT}")
    if(MIDI_OUT_MATCH)
      set(WASM_DOES_MIDI_OUT "true" PARENT_SCOPE)
    else()
      set(WASM_DOES_MIDI_OUT "false" PARENT_SCOPE)
    endif()
  else()
    message(WARNING "config.h not found at ${CONFIG_H}")
    set(WASM_HAS_UI TRUE PARENT_SCOPE)
    set(WASM_HAS_UI_STR "true" PARENT_SCOPE)
    set(WASM_HOST_RESIZE "false" PARENT_SCOPE)
    set(WASM_DOES_MIDI_IN "false" PARENT_SCOPE)
    set(WASM_DOES_MIDI_OUT "false" PARENT_SCOPE)
  endif()
endfunction()

# ============================================================================
# Wrap DSP JS for AudioWorklet scope
# ============================================================================
function(iplug_wrap_dsp_for_worklet dsp_target project_name output_dir)
  set(DSP_JS "${output_dir}/scripts/${project_name}-dsp.js")
  set(WRAPPER_SCRIPT "${IPLUG2_CMAKE_SCRIPTS_DIR}/wrap_wasm_dsp.cmake")
  set(SHIM_FILE "${WASM_TEMPLATE_DIR}/scripts/worklet-scope-shim.js")

  # SINGLE_FILE=1 embeds the wasm binary as a raw byte string in the JS —
  # CMake's file(READ)/file(WRITE) mangles it, so the wrapper script delegates
  # the prepend to Python (see wrap_wasm_dsp.cmake).
  add_custom_command(TARGET ${dsp_target} POST_BUILD
    COMMAND ${CMAKE_COMMAND}
      -DPython3_EXECUTABLE=${Python3_EXECUTABLE}
      -DDSP_JS_FILE=${DSP_JS}
      -DSHIM_FILE=${SHIM_FILE}
      -P ${WRAPPER_SCRIPT}
    COMMENT "Wrapping ${project_name}-dsp.js for AudioWorklet scope"
    VERBATIM
  )
endfunction()

# ============================================================================
# Detect which web resource categories exist under resources/
# Mirrors the EXISTS/GLOB logic in iplug_bundle_web_resources.
# Sets WEB_FOUND_FONTS / _SVGS / _IMGS / _IMGS2X in parent scope as "true"/"false".
# ============================================================================
function(iplug_detect_web_resources resource_dir)
  set(found_fonts "false")
  set(found_svgs "false")
  set(found_imgs "false")
  set(found_imgs2x "false")

  if(EXISTS "${resource_dir}/fonts")
    set(found_fonts "true")
  endif()

  file(GLOB _svgs "${resource_dir}/img/*.svg")
  if(_svgs)
    set(found_svgs "true")
  endif()

  file(GLOB _pngs "${resource_dir}/img/*.png")
  list(FILTER _pngs EXCLUDE REGEX "@2x")
  if(_pngs)
    set(found_imgs "true")
  endif()

  file(GLOB _pngs2x "${resource_dir}/img/*@2x*.png")
  if(_pngs2x)
    set(found_imgs2x "true")
  endif()

  set(WEB_FOUND_FONTS ${found_fonts} PARENT_SCOPE)
  set(WEB_FOUND_SVGS ${found_svgs} PARENT_SCOPE)
  set(WEB_FOUND_IMGS ${found_imgs} PARENT_SCOPE)
  set(WEB_FOUND_IMGS2X ${found_imgs2x} PARENT_SCOPE)
endfunction()

# ============================================================================
# Stage bundled web resources into scripts/ for Wasm.
# WAM's index.html loads resources from the output root; Wasm's loads them
# from scripts/. Resource bundling (iplug_bundle_web_resources) writes to
# the root, so for Wasm we copy the produced .js/.data pairs into scripts/.
# ============================================================================
function(iplug_stage_wasm_resources project_name output_dir)
  set(scripts_dir "${output_dir}/scripts")
  set(staged)

  foreach(pair fonts:fonts svgs:svgs imgs:imgs imgs2x:imgs@2x)
    string(REPLACE ":" ";" pair_list "${pair}")
    list(GET pair_list 0 flag_suffix)
    list(GET pair_list 1 base)

    string(TOUPPER "${flag_suffix}" flag_upper)
    set(found_var "WEB_FOUND_${flag_upper}")
    if(NOT ${found_var} STREQUAL "true")
      continue()
    endif()

    set(src_js   "${output_dir}/${base}.js")
    set(src_data "${output_dir}/${base}.data")
    set(dst_js   "${scripts_dir}/${base}.js")
    set(dst_data "${scripts_dir}/${base}.data")

    add_custom_command(
      OUTPUT ${dst_js} ${dst_data}
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${src_js}   ${dst_js}
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${src_data} ${dst_data}
      DEPENDS ${src_js} ${src_data}
      COMMENT "Staging ${base}.{js,data} into scripts/ for Wasm"
      VERBATIM
    )
    list(APPEND staged ${dst_js} ${dst_data})
  endforeach()

  if(staged)
    add_custom_target(${project_name}_wasm_resources DEPENDS ${staged})
  endif()
endfunction()

# ============================================================================
# Configure wasm template files
# ============================================================================
function(iplug_configure_wasm_templates project_name output_dir)
  # Get lowercase project name
  string(TOLOWER ${project_name} project_name_lc)

  set(SCRIPTS_DIR "${output_dir}/scripts")
  set(STYLES_DIR "${output_dir}/styles")

  # Create output directories
  file(MAKE_DIRECTORY ${SCRIPTS_DIR})
  file(MAKE_DIRECTORY ${STYLES_DIR})

  # Configure bundle.js template
  set(BUNDLE_TEMPLATE "${WASM_TEMPLATE_DIR}/scripts/IPlugWasmBundle.js.template")
  set(BUNDLE_OUTPUT "${SCRIPTS_DIR}/${project_name}-bundle.js")

  add_custom_command(
    OUTPUT ${BUNDLE_OUTPUT}
    COMMAND ${CMAKE_COMMAND}
      -DINPUT_FILE=${BUNDLE_TEMPLATE}
      -DOUTPUT_FILE=${BUNDLE_OUTPUT}
      -DNAME_PLACEHOLDER=${project_name}
      -DNAME_PLACEHOLDER_LC=${project_name_lc}
      -DMAXNINPUTS_PLACEHOLDER=${WASM_MAX_INPUTS}
      -DMAXNOUTPUTS_PLACEHOLDER=${WASM_MAX_OUTPUTS}
      -DIS_INSTRUMENT_PLACEHOLDER=${WASM_IS_INSTRUMENT}
      -DHOST_RESIZE_PLACEHOLDER=${WASM_HOST_RESIZE}
      -DHAS_UI_PLACEHOLDER=${WASM_HAS_UI_STR}
      -DDOES_MIDI_IN_PLACEHOLDER=${WASM_DOES_MIDI_IN}
      -DDOES_MIDI_OUT_PLACEHOLDER=${WASM_DOES_MIDI_OUT}
      -P ${IPLUG2_CMAKE_SCRIPTS_DIR}/configure_wasm_template.cmake
    DEPENDS ${BUNDLE_TEMPLATE}
    COMMENT "Configuring ${project_name}-bundle.js"
    VERBATIM
  )

  # Configure processor.js template
  set(PROCESSOR_TEMPLATE "${WASM_TEMPLATE_DIR}/scripts/IPlugWasmProcessor.js.template")
  set(PROCESSOR_OUTPUT "${SCRIPTS_DIR}/${project_name}-processor.js")

  add_custom_command(
    OUTPUT ${PROCESSOR_OUTPUT}
    COMMAND ${CMAKE_COMMAND}
      -DINPUT_FILE=${PROCESSOR_TEMPLATE}
      -DOUTPUT_FILE=${PROCESSOR_OUTPUT}
      -DNAME_PLACEHOLDER=${project_name}
      -DNAME_PLACEHOLDER_LC=${project_name_lc}
      -P ${IPLUG2_CMAKE_SCRIPTS_DIR}/configure_wasm_template.cmake
    DEPENDS ${PROCESSOR_TEMPLATE}
    COMMENT "Configuring ${project_name}-processor.js"
    VERBATIM
  )

  # Configure index.html template
  set(INDEX_TEMPLATE "${WASM_TEMPLATE_DIR}/index.html")
  set(INDEX_OUTPUT "${output_dir}/index.html")

  # Default FOUND_* to "true" so the template loads all resource tags if the
  # caller hasn't run detection. iplug_build_wasm_dist always sets them.
  if(NOT DEFINED WEB_FOUND_FONTS)
    set(WEB_FOUND_FONTS "true")
  endif()
  if(NOT DEFINED WEB_FOUND_SVGS)
    set(WEB_FOUND_SVGS "true")
  endif()
  if(NOT DEFINED WEB_FOUND_IMGS)
    set(WEB_FOUND_IMGS "true")
  endif()
  if(NOT DEFINED WEB_FOUND_IMGS2X)
    set(WEB_FOUND_IMGS2X "true")
  endif()

  add_custom_command(
    OUTPUT ${INDEX_OUTPUT}
    COMMAND ${CMAKE_COMMAND}
      -DINPUT_FILE=${INDEX_TEMPLATE}
      -DOUTPUT_FILE=${INDEX_OUTPUT}
      -DNAME_PLACEHOLDER=${project_name}
      -DNAME_PLACEHOLDER_LC=${project_name_lc}
      -DFOUND_FONTS=${WEB_FOUND_FONTS}
      -DFOUND_SVGS=${WEB_FOUND_SVGS}
      -DFOUND_IMGS=${WEB_FOUND_IMGS}
      -DFOUND_IMGS2X=${WEB_FOUND_IMGS2X}
      -P ${IPLUG2_CMAKE_SCRIPTS_DIR}/configure_wasm_template.cmake
    DEPENDS ${INDEX_TEMPLATE}
    COMMENT "Configuring index.html for ${project_name}"
    VERBATIM
  )

  # Copy style.css
  set(STYLE_TEMPLATE "${WASM_TEMPLATE_DIR}/styles/style.css")
  set(STYLE_OUTPUT "${STYLES_DIR}/style.css")

  add_custom_command(
    OUTPUT ${STYLE_OUTPUT}
    COMMAND ${CMAKE_COMMAND}
      -DINPUT_FILE=${STYLE_TEMPLATE}
      -DOUTPUT_FILE=${STYLE_OUTPUT}
      -DNAME_PLACEHOLDER_LC=${project_name_lc}
      -P ${IPLUG2_CMAKE_SCRIPTS_DIR}/configure_wasm_template.cmake
    DEPENDS ${STYLE_TEMPLATE}
    COMMENT "Configuring style.css for ${project_name}"
    VERBATIM
  )

  # Create custom target for templates
  add_custom_target(${project_name}_wasm_templates
    DEPENDS
      ${BUNDLE_OUTPUT}
      ${PROCESSOR_OUTPUT}
      ${INDEX_OUTPUT}
      ${STYLE_OUTPUT}
  )

  set(${project_name}_WASM_TEMPLATES_TARGET ${project_name}_wasm_templates PARENT_SCOPE)
endfunction()

# ============================================================================
# Build complete Wasm distribution
# ============================================================================
function(iplug_build_wasm_dist project_name)
  cmake_parse_arguments(PARSE_ARGV 1 WASM_DIST
    ""
    "DSP_TARGET;UI_TARGET;OUTPUT_DIR"
    ""
  )

  # Default output directory
  if(NOT WASM_DIST_OUTPUT_DIR)
    set(WASM_DIST_OUTPUT_DIR "${CMAKE_BINARY_DIR}/out/${project_name}")
  endif()

  set(OUTPUT_DIR ${WASM_DIST_OUTPUT_DIR})
  set(SCRIPTS_DIR "${OUTPUT_DIR}/scripts")

  # Create output directories at configure time
  file(MAKE_DIRECTORY ${OUTPUT_DIR})
  file(MAKE_DIRECTORY ${SCRIPTS_DIR})

  # Parse config.h for plugin configuration
  iplug_parse_wasm_config(${PROJECT_DIR})

  message(STATUS "${project_name} Wasm: inputs=${WASM_MAX_INPUTS}, outputs=${WASM_MAX_OUTPUTS}, hasUI=${WASM_HAS_UI_STR}")

  # 1. Set up DSP target output
  set_target_properties(${WASM_DIST_DSP_TARGET} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SCRIPTS_DIR}
    OUTPUT_NAME "${project_name}-dsp"
    SUFFIX ".js"
  )

  # 2. Set up UI target output (if it exists)
  if(TARGET ${WASM_DIST_UI_TARGET})
    set_target_properties(${WASM_DIST_UI_TARGET} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${SCRIPTS_DIR}
      OUTPUT_NAME "${project_name}-ui"
      SUFFIX ".js"
    )
  endif()

  # 3. Bundle resources (reuse from WAMDist if available, skip if already created by WAM)
  if(PLUG_HAS_RESOURCES)
    if(COMMAND iplug_bundle_web_resources AND NOT TARGET ${project_name}_wam_resources)
      iplug_bundle_web_resources(${project_name} ${PLUG_RESOURCES_DIR} ${OUTPUT_DIR})
    endif()

    # 3b. Detect which resources were bundled so we can stage them into scripts/
    # and generate a correct index.html.
    iplug_detect_web_resources(${PLUG_RESOURCES_DIR})

    # 3c. Stage resource .js/.data into scripts/ for Wasm
    iplug_stage_wasm_resources(${project_name} ${OUTPUT_DIR})
  endif()

  # 4. Post-process DSP JS with AudioWorklet scope wrapper
  iplug_wrap_dsp_for_worklet(${WASM_DIST_DSP_TARGET} ${project_name} ${OUTPUT_DIR})

  # 5. Configure template files (index.html uses FOUND_* to comment out
  # missing resource tags — see iplug_detect_web_resources above)
  iplug_configure_wasm_templates(${project_name} ${OUTPUT_DIR})

  # Create aggregate target
  set(DIST_DEPENDS ${WASM_DIST_DSP_TARGET})
  if(TARGET ${WASM_DIST_UI_TARGET})
    list(APPEND DIST_DEPENDS ${WASM_DIST_UI_TARGET})
  endif()

  add_custom_target(${project_name}-wasm-dist ALL
    DEPENDS ${DIST_DEPENDS}
    COMMENT "Building complete Wasm distribution for ${project_name}"
  )

  # Add template dependency
  if(TARGET ${project_name}_wasm_templates)
    add_dependencies(${project_name}-wasm-dist ${project_name}_wasm_templates)
  endif()

  # Add resource dependencies if they exist
  if(TARGET ${project_name}_wam_resources)
    add_dependencies(${project_name}-wasm-dist ${project_name}_wam_resources)
  endif()
  if(TARGET ${project_name}_wasm_resources)
    add_dependencies(${project_name}-wasm-dist ${project_name}_wasm_resources)
  endif()

  message(STATUS "Wasm distribution target: ${project_name}-wasm-dist")
  message(STATUS "Wasm output directory: ${OUTPUT_DIR}")
endfunction()
