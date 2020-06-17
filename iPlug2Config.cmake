cmake_minimum_required(VERSION 3.11)

# We need this so we can find call FindFaust.cmake
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH};${CMAKE_CURRENT_LIST_DIR})
# This is used in iplug2_configure_target
set(IPLUG2_DIR ${CMAKE_CURRENT_LIST_DIR})

# Determine VST2 and VST3 directories
find_file(VST2_32_PATH
  "VstPlugins"
  PATHS "C:/Program Files (x86)" "C:/Program Files"
  DOC "Path to install 32-bit VST2 plugins"
)
find_file(VST2_64_PATH
  "VstPlugins"
  PATHS "C:/Program Files"
  DOC "Path to install 64-bit VST2 plugins"
)
find_file(VST3_32_PATH
  "VST3"
  PATHS "C:/Program Files (x86)/Common Files" "C:/Program Files/Common Files"
  DOC "Path to install 32-bit VST3 plugins"
)
find_file(VST3_64_PATH
  "VST3"
  PATHS "C:/Program Files/Common Files"
  DOC "Path to install 64-bit VST3 plugins"
)

if (WIN32)
  set(AAX_32_PATH "C:/Program Files (x86)/Common Files/Avid/Audio/Plug-Ins"
    CACHE PATH "Path to install 32-bit AAX plugins")
  set(AAX_64_PATH "C:/Program Files/Common Files/Avid/Audio/Plug-Ins"
    CACHE PATH "Path to install 64-bit AAX plugins")
  
  # Need to determine processor arch for postbuild-win.bat
  if ("${CMAKE_GENERATOR}" MATCHES ".*Visual Studio.*")
    set(PROCESSOR_ARCH "\$(PlatformName)")
  else()
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64")
      set(PROCESSOR_ARCH "x64")
    else()
      set(PROCESSOR_ARCH "Win32")
    endif()
  endif()
else()
  message("Unsupported platform" FATAL_ERROR)
endif()


# These are all the include directories required by IGraphics. I know, it's a lot.
set(IPLUG2_IGRAPHICS_INC_PATH
  ${IPLUG2_DIR}/IGraphics
  ${IPLUG2_DIR}/IGraphics/Controls
  ${IPLUG2_DIR}/IGraphics/Drawing
  ${IPLUG2_DIR}/IGraphics/Platforms
  ${IPLUG2_DIR}/IGraphics/Extras
  ${IPLUG2_DIR}/WDL/lice
  ${IPLUG2_DIR}/Dependencies/IGraphics/NanoSVG/src
  ${IPLUG2_DIR}/Dependencies/IGraphics/NanoVG/src
  ${IPLUG2_DIR}/Dependencies/IGraphics/AGG/agg-2.4/include
  ${IPLUG2_DIR}/Dependencies/IGraphics/AGG/agg-2.4/font_freetype
  ${IPLUG2_DIR}/Dependencies/IGraphics/AGG/agg-2.4/include/util
  ${IPLUG2_DIR}/Dependencies/IGraphics/AGG/agg-2.4/include/platform/win32
  ${IPLUG2_DIR}/Dependencies/IGraphics/AGG/agg-2.4/src
  ${IPLUG2_DIR}/Dependencies/IGraphics/AGG/agg-2.4/src/platform/win32
  ${IPLUG2_DIR}/Dependencies/IGraphics/Cairo
  ${IPLUG2_DIR}/Dependencies/Build/src
  ${IPLUG2_DIR}/WDL/libpng
  ${IPLUG2_DIR}/WDL/zlib
  ${IPLUG2_DIR}/Dependencies/Build/src/freetype/include
  ${IPLUG2_DIR}/Dependencies/IGraphics/STB
  ${IPLUG2_DIR}/Dependencies/IGraphics/imgui
  ${IPLUG2_DIR}/Dependencies/IGraphics/imgui/examples
  ${IPLUG2_DIR}/Dependencies/Build/src/skia
  ${IPLUG2_DIR}/Dependencies/Build/src/skia/include/core
  ${IPLUG2_DIR}/Dependencies/Build/src/skia/include/effects
  ${IPLUG2_DIR}/Dependencies/Build/src/skia/include/config
  ${IPLUG2_DIR}/Dependencies/Build/src/skia/include/utils
  ${IPLUG2_DIR}/Dependencies/Build/src/skia/include/gpu
  ${IPLUG2_DIR}/Dependencies/Build/src/skia/experimental/svg/model
  ${IPLUG2_DIR}/Dependencies/IGraphics/yoga
  ${IPLUG2_DIR}/Dependencies/IGraphics/yoga/yoga
)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_OPT_ARCH_NATIVE_SUPPORTED)
CHECK_CXX_COMPILER_FLAG("/arch:AVX" COMPILER_OPT_ARCH_AVX_SUPPORTED)

macro(begin_faust_target target_name)
  set(FAUST_TARGET_SRC_LIST "")
  set(FAUST_TARGET_DEP_LIST "")
  set(FAUST_TARGET_NAME ${target_name})
  set("${target_name}_INCLUDE_DIR" "${PROJECT_BINARY_DIR}/${FAUST_TARGET_NAME}.dir")
endmacro()

macro(end_faust_target)
  add_custom_target("${FAUST_TARGET_NAME}" ALL
    DEPENDS ${FAUST_TARGET_DEP_LIST}
    SOURCES ${FAUST_TARGET_SRC_LIST}
  )
endmacro()

function(add_faust_file dsp_file class_name)
  # Default values
  set(FAUST_DSP_OUTPUT "${${FAUST_TARGET_NAME}_INCLUDE_DIR}/${class_name}.hpp")

  add_custom_command(
    OUTPUT "${FAUST_DSP_OUTPUT}"
    COMMAND "${FAUST_EXECUTABLE}" -lang cpp -cn "${class_name}" -a "${IPLUG2_DIR}/IPlug/Extras/Faust/IPlugFaust_arch.cpp" -o "${FAUST_DSP_OUTPUT}" "${dsp_file}"
    DEPENDS "${dsp_file}"
  )
  set(FAUST_TARGET_DEP_LIST ${FAUST_TARGET_DEP_LIST} "${FAUST_DSP_OUTPUT}" PARENT_SCOPE)
  set(FAUST_TARGET_SRC_LIST ${FAUST_TARGET_SRC_LIST} "${dsp_file}" PARENT_SCOPE)
endfunction()


#! iplug2_configure : Setup various iPlug2 configuration variables.
# 
# 
# This is a helper function which makes it easy to set the correct configuration variables without having to remember
# every variable name or option yourself. It also sets sensible defaults when values are not supplied. You may call
# this function multiple times to change the configuration as desired.
#
function(iplug2_configure app_name)
  cmake_parse_arguments(PARSE_ARGV 2 "cfg"
    "FAUST NANOVG GL2 GL3"
    "VST_ICON AAX_ICON" "")
  macro(setDefault var value1 valueDefault)
    if ("${value1}")
      set(${var} "${value1}" PARENT_SCOPE)
    else() 
      set(${var} "${valueDefault}" PARENT_SCOPE)
    endif()
  endmacro()
  
  set(IPLUG2_CONFIG_APP_NAME "${app_name}" PARENT_SCOPE)
  set(IPLUG2_CONFIG_FAUST "$<DEFINED cfg_FAUST:1>" PARENT_SCOPE)
  set(IPLUG2_CONFIG_IGRAPHICS_NANOVG "$<DEFINED cfg_NANOVG:1>" PARENT_SCOPE)
  set(IPLUG2_CONFIG_IGRAPHICS_GL2 "$<DEFINED cfg_GL2:1>" PARENT_SCOPE)
  set(IPLUG2_CONFIG_IGRAPHICS_GL3 "$<DEFINED cfg_GL3:1>" PARENT_SCOPE)
  setDefault(IPLUG2_CONFIG_VST_ICON "${cfg_VST_ICON}" 
    "${IPLUG2_DIR}/Dependencies/IPlug/VST3_SDK/doc/artwork/VST_Logo_Steinberg.ico")
  setDefault(IPLUG2_CONFIG_AAX_ICON "${cfg_AAX_ICON}"
    "${IPLUG2_DIR}/Dependencies/IPlug/AAX_SDK/Utilities/PlugIn.ico")
endfunction()    


#! iplug2_configure_target : Configure a CMake target to output one of the various plugin formats supported by iPlug2.
# 
# 
# 
function(iplug2_configure_target target target_type)
  # Because we add to lists SO MUCH, I made a macro for it.
  macro(addL list_)
    list(APPEND ${list_} ${ARGN})
  endmacro()
  
  #############
  # Constants #
  #############
  
  # These are "constants" that we don't want to export globally.
  set(IPLUG_DEPS ${IPLUG2_DIR}/Dependencies/IPlug)
  set(IPLUG_SRC ${IPLUG2_DIR}/IPlug)
  set(IGRAPHICS_SRC ${IPLUG2_DIR}/IGraphics)
  
  #############
  # Variables #
  #############
  
  # Compiler definitions
  set(_defs "")
  # List of source files required for building this configuration.
  set(_src
    ${IPLUG_SRC}/IPlugAPIBase.cpp
    ${IPLUG_SRC}/IPlugParameter.cpp
    ${IPLUG_SRC}/IPlugPaths.cpp
    ${IPLUG_SRC}/IPlugPluginBase.cpp
    ${IPLUG_SRC}/IPlugProcessor.cpp
    ${IPLUG_SRC}/IPlugTimer.cpp
    
    # These should only be added if the user is using IGraphics
    # For now, we assume they are by default. This should be changed later.
    ${IGRAPHICS_SRC}/IControl.cpp
    ${IGRAPHICS_SRC}/IGraphics.cpp
    ${IGRAPHICS_SRC}/IGraphicsEditorDelegate.cpp
    ${IGRAPHICS_SRC}/Controls/IControls.cpp
    ${IGRAPHICS_SRC}/Controls/IPopupMenuControl.cpp
    ${IGRAPHICS_SRC}/Controls/ITextEntryControl.cpp
  )
  # Include directories
  set(_incdir 
    ${IPLUG_SRC}
    ${IPLUG_SRC}/Extras
    ${IPLUG2_IGRAPHICS_INC_PATH}
    ${IPLUG2_DIR}/WDL
  )
  # Compiler options
  set(_copts "")    
  
  #####################
  # Platform Settings #
  #####################
  
  if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    addL(_src ${IGRAPHICS_SRC}/Platforms/IGraphicsWin.cpp)
    target_link_libraries(${target} "Shlwapi.lib" "comctl32.lib" "wininet.lib")
    set(post_build_script "${CMAKE_SOURCE_DIR}/scripts/postbuild-win.bat")
  elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
    addL(_src ${IGRAPHICS_SRC}/Platforms/IGraphicsLinux.cpp)
  elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    addL(_src ${IGRAPHICS_SRC}/Platforms/IGraphicsMac.mm)
  else()
    message("Unhandled system ${CMAKE_SYSTEM_NAME}" FATAL_ERROR)
  endif()
  
  ###############
  # Target Type #
  ###############

  # Here we handle the specific configuration for the various plugin formats supported by iPlug2.
  # Basically there's an if block for each format.

  ##############
  # AAX Plugin #
  ##############
  if (${target_type} STREQUAL "aax")
    addL(_defs "AAX_API" "IPLUG_EDITOR=1" "IPLUG_DSP=1")
    addL(_incdir ${IPLUG2_DIR}/IPlug/AAX ${IPLUG2_DIR}/Dependencies/IPlug/AAX_SDK/Interfaces ${IPLUG2_DIR}/Dependencies/IPlug/AAX_SDK/Interfaces/ACF )
    
  ##################
  # Standalone App #
  ##################
  elseif (${target_type} STREQUAL "app")
    set(sd ${IPLUG2_DIR}/IPlug/APP)
    addL(_defs "APP_API" "IPLUG_EDITOR=1" "IPLUG_DSP=1" "__WINDOWS_DS__" "__WINDOWS_MM__" "__WINDOWS_ASIO__")
    addL(_incdir
      ${sd} 
      ${IPLUG_DEPS}/RTAudio
      ${IPLUG_DEPS}/RTAudio/include
      ${IPLUG_DEPS}/RTMidi
      ${IPLUG_DEPS}/RTMidi/include
    )
    addL(_src
      ${sd}/IPlugAPP.cpp 
      ${sd}/IPlugAPP_dialog.cpp 
      ${sd}/IPlugAPP_host.cpp 
      ${sd}/IPlugAPP_main.cpp
      ${IPLUG_DEPS}/RTAudio/RtAudio.cpp
      ${IPLUG_DEPS}/RTAudio/include/asio.cpp
      ${IPLUG_DEPS}/RTAudio/include/asiodrivers.cpp
      ${IPLUG_DEPS}/RTAudio/include/asiolist.cpp
      ${IPLUG_DEPS}/RTAudio/include/iasiothiscallresolver.cpp
      ${IPLUG_DEPS}/RTMidi/RtMidi.cpp
      ${IPLUG_DEPS}/RTMidi/rtmidi_c.cpp
    )
    # Link Windows sound libraies
    if (WIN32)
      target_link_libraries(${target} dsound.lib winmm.lib)
    endif()

  ##############
  # Audio Unit #
  ##############
  elseif (${target_type} STREQUAL "au")
    addL(_defs "AU_API")
    addL(_incdir ${IPLUG2_DIR}/IPlug/AUv2 )

  #################
  # Audio Unit v3 #
  #################
  elseif (${target_type} STREQUAL "auv3")
    addL(_defs "AUv3_API")
    addL(_incdir ${IPLUG2_DIR}/IPlug/AUv3 )

  ###########
  # VST 1/2 #
  ###########
  elseif (${target_type} STREQUAL "vst2")
    set(sd ${IPLUG2_DIR}/IPlug/VST2)
    addL(_defs "VST2_API" "VST_FORCE_DEPRECATED" "IPLUG_EDITOR=1" "IPLUG_DSP=1")
    addL(_incdir ${sd} ${IPLUG2_DIR}/Dependencies/IPlug/VST2_SDK )
    addL(_src ${sd}/IPlugVST2.cpp)

    if (WIN32)
      # After building, we run the post-build script, but only on Windows
      set(cmd_args_1 "\".dll\"" "${IPLUG2_CONFIG_APP_NAME}" "\"${PROCESSOR_ARCH}\"" "1" "\"$<TARGET_FILE:${target}>\""
        "${VST2_32_PATH}" "${VST2_64_PATH}" "${VST3_32_PATH}" "${VST3_64_PATH}"
        "${AAX_32_PATH}" "${AAX_64_PATH}" "\"${CMAKE_BINARY_DIR}/out\""
        "\"${IPLUG2_CONFIG_VST_ICON}\"" "\"${IPLUG2_CONFIG_AAX_ICON}\""
        "\"${IPLUG2_DIR}/Scripts/create_bundle.bat\""
      )
      string(REPLACE "/" "\\" cmd_args_2 "${cmd_args_1}")
      add_custom_command(TARGET ${target} POST_BUILD COMMAND ${post_build_script} ARGS ${cmd_args_2})
    endif()
    
  #########
  # VST 3 #
  #########
  elseif (${target_type} STREQUAL "vst3")
    set(sd ${IPLUG2_DIR}/IPlug/VST3)
    set(sdk_dir ${IPLUG2_DIR}/Dependencies/IPlug/VST3_SDK)
    addL(_defs "VST3_API" "IPLUG_EDITOR=1" "IPLUG_DSP=1")
    addL(_incdir ${sd} ${sdk_dir} )
    addL(_src 
      ${sd}/IPlugVST3.cpp 
      ${sd}/IPlugVST3_Controller.cpp
      ${sd}/IPlugVST3_Processor.cpp
      ${sd}/IPlugVST3_ProcessorBase.cpp
    )
    
    # Pick up all the VST3 source files, there are a LOT of them, and in the public.sdk dir
    # we only add specific ones, depending on our platform and a few other things.
    file(GLOB_RECURSE vst3_src_base ${sdk_dir}/base/*.cpp)
    file(GLOB_RECURSE vst3_src_interfaces ${sdk_dir}/pluginterfaces/*.cpp)
    set(sdk2 ${sdk_dir}/public.sdk/source)
    string(CONCAT vst3_sdk_src
      "${sdk2}/common/commoniids.cpp;${sdk2}/common/memorystream.cpp;"
      "${sdk2}/common/pluginview.cpp;${sdk2}/vst/vstaudioeffect.cpp;"
      "${sdk2}/vst/vstbus.cpp;${sdk2}/vst/vstcomponent.cpp;"
      "${sdk2}/vst/vstcomponentbase.cpp;${sdk2}/vst/vstinitiids.cpp;"
      "${sdk2}/vst/vstparameters.cpp;${sdk2}/vst/vstsinglecomponenteffect.cpp;"
      "${sdk2}/vst/hosting/parameterchanges.cpp;${sdk2}/vst/hosting/parameterchanges.h;"
    )
    if (WIN32)
      set(vst3_sdk_src "${vst3_sdk_src};${sdk2}/main/dllmain.cpp;${sdk2}/main/pluginfactory.cpp;")

      # Note: For visual studio, we COULD use $(TargetPath) for the target, but for all other platforms, no.
      # After building, we run the post-build script
      set(cmd_args_1 "\".vst3\"" "\"${IPLUG2_CONFIG_APP_NAME}\"" "\"${PROCESSOR_ARCH}\"" "\"1\"" "\"$<TARGET_FILE:${target}>\""
        "${VST2_32_PATH}" "${VST2_64_PATH}" "${VST3_32_PATH}" "${VST3_64_PATH}"
        "${AAX_32_PATH}" "${AAX_64_PATH}" "\"${CMAKE_BINARY_DIR}/out\""
        "\"${IPLUG2_CONFIG_VST_ICON}\"" "\"${IPLUG2_CONFIG_AAX_ICON}\""
        "\"${IPLUG2_DIR}/Scripts/create_bundle.bat\""
      )
      string(REPLACE "/" "\\" cmd_args_2 "${cmd_args_1}")
      add_custom_command(TARGET ${target} POST_BUILD COMMAND ${post_build_script} ARGS ${cmd_args_2})
    endif()
    set(vst3_src "${vst3_src_base};${vst3_src_interfaces};${vst3_sdk_src}")
    target_sources(${target} PRIVATE ${vst3_src})
    source_group(TREE ${sdk_dir} PREFIX "IPlug/VST3" FILES ${vst3_src})
    
    set_target_properties(${target} PROPERTIES
      "OUTPUT_NAME" "${IPLUG2_CONFIG_APP_NAME}"
      "SUFFIX" ".vst3"
    )
    
  elseif (${target_type} STREQUAL "web")
    addL(_defs "WEB_API")
    addL(_incdir ${IPLUG2_DIR}/IPlug/WEB )
  # TODO include WAM plugins
  else()
    message("Invalid target type" FATAL_ERROR)
  endif()
  
  ###################
  # Config Settings #
  ###################
  
  # Here we deal with various configuration variables that may be set using IPLUG2_CONFIG
  # Basically we parse them and add include dirs, links, and definitions appropriately.
  if (IPLUG2_CONFIG_IGRAPHICS_GL2)
    addL(_incdir ${IPLUG2_DIR}/Dependencies/IGraphics/glad_GL2/include ${IPLUG2_DIR}/Dependencies/IGraphics/glad_GL2/src)
    addL(_defs "IGRAPHICS_GL2")
  endif()
  if (IPLUG2_CONFIG_IGRAPHICS_GL3)
    addL(_incdir ${IPLUG2_DIR}/Dependencies/IGraphics/glad_GL3/include ${IPLUG2_DIR}/Dependencies/IGraphics/glad_GL3/src)
    addL(_defs "IGRAPHICS_GL3")
  endif()
  if (IPLUG2_CONFIG_IGRAPHICS_NANOVG)
    addL(_defs "IGRAPHICS_NANOVG")
  endif()
  if (IPLUG2_CONFIG_FAUST)
    addL(_incdir ${IPLUG2_DIR}/IPlug/Extras/Faust ${FAUST_INCLUDE_DIR})
  endif()
  
  if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    addL(_defs "_CRT_SECURE_NO_WARNINGS" "_CRT_SECURE_NO_DEPRECATE" "_CRT_NONSTDC_NO_DEPRECATE" "NOMINMAX" "_MBCS")
    addL(_copts "/wd4996" "/wd4250" "/wd4018" "/wd4267" "/wd4068")
  endif()
  
  # TODO
  # * Enable "Use full paths"
  
  # Use advanced SIMD instructions when available.
  if (COMPILER_OPT_ARCH_NATIVE_SUPPORTED)
    addL(_copts "-march=native")
  elseif(COMPILER_OPT_ARCH_AVX_SUPPORTED)
    addL(_copts "/arch:AVX")
  endif()
  
  ###########################
  # Actually Set Properties #
  ###########################
  
  target_sources(${target} PRIVATE ${_src})
  target_compile_definitions(${target} PUBLIC ${_defs} )
  target_include_directories(${target} PUBLIC ${_incdir})
  target_compile_options(${target} PUBLIC ${_copts})
  source_group("IPlug" FILES ${_src})
endfunction()