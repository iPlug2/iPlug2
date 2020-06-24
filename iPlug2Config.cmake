cmake_minimum_required(VERSION 3.11)

# We need this so we can find call FindFaust.cmake
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH};${CMAKE_CURRENT_LIST_DIR})
# This is used in iplug2_configure_target
set(IPLUG2_DIR ${CMAKE_CURRENT_LIST_DIR})

# Determine VST2 and VST3 directories
find_file(VST2_32_PATH
  "VstPlugins"
  PATHS "C:/Program Files (x86)"
  DOC "Path to install 32-bit VST2 plugins"
)
find_file(VST2_64_PATH
  NAMES "VstPlugins" "VST"
  PATHS "C:/Program Files" "$ENV{HOME}/Library/Audio/Plug-Ins" 
  DOC "Path to install 64-bit VST2 plugins"
)

set(IPLUG2_AAX_ICON
  "${IPLUG2_DIR}/Dependencies/IPlug/AAX_SDK/Utilities/PlugIn.ico"
  CACHE FILEPATH "Path to AAX plugin icon"
)

set(IPLUG2_CXX_STANDARD "17" CACHE STRING "The C++ standard to use")

set(IPLUG2_APP_NAME ${CMAKE_PROJECT_NAME} CACHE STRING "Name of the VST/AU/App/etc.")

if (WIN32)
  set(AAX_32_PATH "C:/Program Files (x86)/Common Files/Avid/Audio/Plug-Ins"
    CACHE PATH "Path to install 32-bit AAX plugins")
  set(AAX_64_PATH "C:/Program Files/Common Files/Avid/Audio/Plug-Ins"
    CACHE PATH "Path to install 64-bit AAX plugins")

  # Need to determine processor arch for postbuild-win.bat
  if (CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64")
    set(PROCESSOR_ARCH "x64" CACHE STRING "Processor architecture")
  else()
    set(PROCESSOR_ARCH "Win32" CACHE STRING "Processor architecture")
  endif()

elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(_tmp "$ENV{HOME}/Library/Audio/Plug-Ins")
  set(VST2_PATH "${_tmp}/VST" CACHE PATH "VST2 plugin directory.")
  set(VST3_PATH "${_tmp}/VST3" CACHE PATH "VST3 plugin directory.")

else()
  #message("Unsupported platform" FATAL_ERROR)
endif()

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_OPT_ARCH_NATIVE_SUPPORTED)
CHECK_CXX_COMPILER_FLAG("/arch:AVX" COMPILER_OPT_ARCH_AVX_SUPPORTED)

function(add_faust_target target)
  set(inc_dir "${PROJECT_BINARY_DIR}/${target}.dir")
  set("${target}_INCLUDE_DIR" ${inc_dir} PARENT_SCOPE)
  set(src_list "")
  set(out_list "")

  # Make sure the output directory exists
  file(MAKE_DIRECTORY "${${target}_INCLUDE_DIR}")

  list(LENGTH ARGN argcnt)
  math(EXPR argcnt "${argcnt} - 1")
  foreach (i1 RANGE 0 ${argcnt} 2)
    math(EXPR i2 "${i1} + 1")
    list(GET ARGN ${i1} dsp_file)
    list(GET ARGN ${i2} class_name)
    set(out_file "${inc_dir}/Faust${class_name}.hpp")
    add_custom_command(
      OUTPUT "${out_file}"
      COMMAND "${FAUST_EXECUTABLE}" -lang cpp -cn "${class_name}" -a "${IPLUG2_DIR}/IPlug/Extras/Faust/IPlugFaust_arch.cpp" -o "${out_file}" "${dsp_file}"
      DEPENDS "${dsp_file}"
    )
    list(APPEND src_list "${dsp_file}")
    list(APPEND out_list "${out_file}")
  endforeach()
  add_custom_target(${target} ALL DEPENDS ${out_list} SOURCES ${src_list})
endfunction()

#! iplug2_target_add : Helper function to add sources, include directories, etc.
# 
# This helper function combines calls to target_include_directories, target_sources,
# target_compile_definitions, target_compile_options, target_link_libraries,
# add_dependencies, and target_compile_features into a single function call. 
# This means you don't have to re-type the target name so many times, and makes it
# clearer exactly what you're adding to a given target.
# 
# \arg:target The name of the target
# \arg:set_type <PUBLIC | PRIVATE | INTERFACE>
# \group:INCLUDE List of include directories
# \group:SOURCE List of source files
# \group:DEFINE Compiler definitions
# \group:OPTION Compile options
# \group:LINK Link libraries (including other targets)
# \group:DEPEND Add dependencies on other targets
# \group:FEATURE Add compile features
function(iplug2_target_add target set_type)
  cmake_parse_arguments("cfg" "" "" "INCLUDE;SOURCE;DEFINE;OPTION;LINK;DEPEND;FEATURE;RESOURCE" ${ARGN})
  #message("CALL iplug2_add_interface ${target}")
  if (cfg_INCLUDE)
    target_include_directories(${target} ${set_type} ${cfg_INCLUDE})
  endif()
  if (cfg_SOURCE)
    target_sources(${target} ${set_type} ${cfg_SOURCE})
  endif()
  if (cfg_DEFINE)
    target_compile_definitions(${target} ${set_type} ${cfg_DEFINE})
  endif()
  if (cfg_OPTION)
    target_compile_options(${target} ${set_type} ${cfg_OPTION})
  endif()
  if (cfg_LINK)
    target_link_libraries(${target} ${set_type} ${cfg_LINK})
  endif()
  if (cfg_DEPEND)
    add_dependencies(${target} ${set_type} ${cfg_DEPEND})
  endif()
  if (cfg_FEATURE)
    target_compile_features(${target} ${set_type} ${cfg_FEATURE})
  endif()
  if (cfg_RESOURCE)
    set_property(TARGET ${target} APPEND PROPERTY RESOURCE ${cfg_RESOURCE})
    target_sources(${target} ${set_type} ${cfg_RESOURCE})
  endif()
  if (cfg_UNUSED)
    message("Unused arguments ${cfg_UNUSED}" FATAL_ERROR)
  endif()
endfunction()

function(iplug2_add_interface target)
  if (NOT (TARGET ${target}))
    add_library(${target} INTERFACE)
  endif()
  iplug2_target_add(${target} INTERFACE ${ARGN})
endfunction()

############################
# General iPlug2 Interface #
############################

add_library(iPlug2_Core INTERFACE)
set(IPLUG_SRC ${IPLUG2_DIR}/IPlug)
set(IGRAPHICS_SRC ${IPLUG2_DIR}/IGraphics)
set(WDL_DIR ${IPLUG2_DIR}/WDL)
set(_DEPS ${IPLUG2_DIR}/Dependencies)

set(_def "NOMINMAX")
set(_opts "")
set(_lib "")
set(_inc
  # IGraphics
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
  # iPlug2
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

  # These should only be added if the user is using IGraphics
  # For now, we assume they are by default. This should be changed later.
  ${IGRAPHICS_SRC}/IControl.cpp
  ${IGRAPHICS_SRC}/IGraphics.cpp
  ${IGRAPHICS_SRC}/IGraphicsEditorDelegate.cpp
  ${IGRAPHICS_SRC}/Controls/IControls.cpp
  ${IGRAPHICS_SRC}/Controls/IPopupMenuControl.cpp
  ${IGRAPHICS_SRC}/Controls/ITextEntryControl.cpp
)

# Platform Settings
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
  list(APPEND _src ${IGRAPHICS_SRC}/Platforms/IGraphicsWin.cpp)
  target_link_libraries(iPlug2_Core INTERFACE "Shlwapi.lib" "comctl32.lib" "wininet.lib")
  
  # postbuild-win.bat is used by VST2/VST3/AAX on Windows, so we just always configure it on Windows
  # Note: For visual studio, we COULD use $(TargetPath) for the target, but for all other generators, no.
  set(plugin_build_dir "${CMAKE_BINARY_DIR}/out")
  set(create_bundle_script "${IPLUG2_DIR}/Scripts/create_bundle.bat")
  configure_file("${IPLUG2_DIR}/Scripts/postbuild-win.bat.in" "${CMAKE_BINARY_DIR}/postbuild-win.bat")

elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
  list(APPEND _src ${IGRAPHICS_SRC}/Platforms/IGraphicsLinux.cpp)

elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
  list(APPEND _src 
    ${IPLUG_SRC}/IPlugPaths.mm
    ${IGRAPHICS_SRC}/Platforms/IGraphicsMac.mm
    ${IGRAPHICS_SRC}/Platforms/IGraphicsMac_view.mm
    ${IGRAPHICS_SRC}/Platforms/IGraphicsCoreText.mm
  )
  list(APPEND _inc ${WDL_DIR}/swell)
  list(APPEND _lib
    "-framework Cocoa" "-framework Carbon" "-framework CoreFoundation" "-framework CoreData" "-framework Foundation" 
    "-framework CoreServices" "-framework Metal" "-framework MetalKit" "-framework QuartzCore" "-framework OpenGL"
  )
  list(APPEND _opts "-Wno-deprecated-declarations")
else()
  message("Unhandled system ${CMAKE_SYSTEM_NAME}" FATAL_ERROR)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  list(APPEND _def "_CRT_SECURE_NO_WARNINGS" "_CRT_SECURE_NO_DEPRECATE" "_CRT_NONSTDC_NO_DEPRECATE" "NOMINMAX" "_MBCS")
  list(APPEND _opts "/wd4996" "/wd4250" "/wd4018" "/wd4267" "/wd4068")
endif()

# Use advanced SIMD instructions when available.
if (COMPILER_OPT_ARCH_NATIVE_SUPPORTED)
  list(APPEND _opts "-march=native")
elseif(COMPILER_OPT_ARCH_AVX_SUPPORTED)
  list(APPEND _opts "/arch:AVX")
endif()

source_group(TREE ${IPLUG2_DIR} PREFIX "IPlug" FILES ${_src})
iplug2_add_interface(iPlug2_Core DEFINE ${_def} INCLUDE ${_inc} SOURCE ${_src} OPTION ${_opts} LINK ${_lib})


##################
# Standalone App #
##################

include("${IPLUG2_DIR}/IPlug/APP/APP.cmake")

##############
# Audio Unit #
##############

add_library(iPlug2_AU INTERFACE)
set(_sdk ${IPLUG2_DIR}/IPlug/AUv2)
set(_src
  ${_sdk}/dfx-au-utilities.c
  ${_sdk}/IPlugAU.cpp
  ${_sdk}/IPlugAU.r
  ${_sdk}/IPlugAU_view_factory.mm
)
set(_inc
  ${_sdk}
)
set(_def "AU_API" "IPLUG_EDITOR=1" "IPLUG_DSP=1" "SWELL_CLEANUP_ON_UNLOAD")

iplug2_add_interface(iPlug2_AU INCLUDE ${_inc} SOURCE ${_src} DEFINE ${_def} LINK iPlug2_Core)

#################
# Audio Unit v3 #
#################

add_library(iPlug2_AUv3 INTERFACE)
set(_sdk ${IPLUG2_DIR}/IPlug/AUv3)
set(_src
  ${_sdk}/GenericUI.mm
  ${_sdk}/IPlugAUAudioUnit.mm
  ${_sdk}/IPlugAUv3.mm
  ${_sdk}/IPlugAUv3Appex.m
  ${_sdk}/IPlugAUViewController.mm
)
set(_inc ${sdk})
set(_def "AUv3_API" "IPLUG_EDITOR=1" "IPLUG_DSP=1" "SWELL_CLEANUP_ON_UNLOAD")

iplug2_add_interface(iPlug2_AUv3 INCLUDE ${_inc} SOURCE ${_src} DEFINE ${_def} LINK iPlug2_Core)

##########################
# VST2 Interface Library #
##########################

set(sdk ${IPLUG2_DIR}/IPlug/VST2)

iplug2_add_interface(iPlug2_VST2
  INCLUDE ${sdk} ${_DEPS}/IPlug/VST2_SDK
  SOURCE ${sdk}/IPlugVST2.cpp
  DEFINE "VST2_API" "VST_FORCE_DEPRECATED" "IPLUG_EDITOR=1" "IPLUG_DSP=1"
  LINK iPlug2_Core
)


########
# VST3 #
########

include("${IPLUG2_DIR}/IPlug/VST3/VST3.cmake")

#################
# Web DSP / GUI #
#################

set(_sdk ${IPLUG2_DIR}/IPlug/WEB)

iplug2_add_interface(iPlug2_WEB_DSP
  DEFINE "WEB_API" "IPLUG_DSP=1"
  SOURCE ${_sdk}/IPlugWeb.cpp
  INCLUDE ${_sdk}
  LINK iPlug2_Core
)

iplug2_add_interface(iPlug2_WEB_GUI
  DEFINE "WEB_API" "IPLUG_EDITOR=1"
  SOURCE ${_sdk}/IPlugWeb.cpp
  INCLUDE ${_sdk}
  LINK iPlug2_Core
)

####################
# Reaper Extension #
####################

add_library(iPlug2_REAPER INTERFACE)
set(_sdk ${IPLUG2_DIR}/IPlug/ReaperExt)
iplug2_add_interface(iPlug2_REAPER
  INCLUDE "${_sdk}" "${_DEPS}/IPlug/Reaper"
  SOURCE "${_sdk}/ReaperExtBase.cpp"
  DEFINE "REAPER_PLUGIN"
  LINK iPlug2_VST2
)

###############################
# Minor Configuration Targets #
###############################

iplug2_add_interface(iPlug2_GL2
  INCLUDE  ${_DEPS}/IGraphics/glad_GL2/include ${_DEPS}/IGraphics/glad_GL2/src
  DEFINE "IGRAPHICS_GL2"
)

iplug2_add_interface(iPlug2_GL3
  INCLUDE ${_DEPS}/IGraphics/glad_GL3/include ${_DEPS}/IGraphics/glad_GL3/src
  DEFINE "IGRAPHICS_GL3"
)

iplug2_add_interface(iPlug2_NANOVG
  DEFINE "IGRAPHICS_NANOVG"
)
if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(_src ${_DEPS}/IGraphics/NanoVG/src/nanovg.c)
  iplug2_target_add(iPlug2_NANOVG INTERFACE SOURCE ${_src})
  set_property(SOURCE ${_src} PROPERTY LANGUAGE C)
endif()




add_library(iPlug2_Skia INTERFACE)
if (WIN32)
  set(sdk "${_DEPS}/Build/win/${PROCESSOR_ARCH}/$<IF:$<CONFIG:DEBUG>,Debug,Release>")
  iplug2_target_add(iPlug2_Skia INTERFACE
    DEFINE "IGRAPHICS_SKIA"
    LINK 
      "${sdk}/libpng.lib"
      "${sdk}/pixman.lib"
      "${sdk}/skia.lib"
      "${sdk}/skottie.lib"
      "${sdk}/skparagraph.lib"
      "${sdk}/sksg.lib"
      "${sdk}/skshaper.lib"
      "${sdk}/zlib.lib"
  )
else()
  set(iPlug2_Skia_NOTFOUND "TRUE")
endif()


iplug2_add_interface(iPlug2_Faust
  INCLUDE "${IPLUG2_DIR}/IPlug/Extras/Faust" "${FAUST_INCLUDE_DIR}"
)

iplug2_add_interface(iPlug2_FaustGen
  SOURCE "${IPLUG_SRC}/Extras/Faust/IPlugFaustGen.cpp"
  LINK iPlug2_Faust
)

iplug2_add_interface(iPlug2_HIIR
  INCLUDE ${IPLUG_SRC}/Extras/HIIR
  SOURCE "${IPLUG_SRC}/Extras/HIIR/PolyphaseIIR2Designer.cpp"
)

iplug2_add_interface(iPlug2_OSC
  INCLUDE ${IPLUG_SRC}/Extras/OSC
  SOURCE ${IPLUG_SRC}/Extras/OSC/IPlugOSC_msg.cpp
)

iplug2_add_interface(iPlug2_Synth
  INCLUDE ${IPLUG_SRC}/Extras/Synth
  SOURCE ${IPLUG_SRC}/Extras/Synth/MidiSynth.cpp ${IPLUG_SRC}/Extras/Synth/VoiceAllocator.cpp
)


set(IPLUG2_TARGETS
  iPlug2_Core iPlug2_APP iPlug2_AU iPlug2_AUv3 iPlug2_VST2 iPlug2_VST3 iPlug2_WEB_DSP iPlug2_WEB_GUI iPlug2_REAPER
  iPlug2_Faust iPlug2_FaustGen iPlug2_HIIR iPlug2_OSC iPlug2_Synth iPlug2_NANOVG
)
foreach(target IN ITEMS ${IPLUG2_TARGETS})
  get_target_property(_src ${target} INTERFACE_SOURCES)
  if (NOT "${_src}" STREQUAL "_src-NOTFOUND")
    source_group(TREE ${IPLUG2_DIR} PREFIX "IPlug" FILES ${_src})
  endif()
endforeach()

unset(_inc)
unset(_def)
unset(_src)
unset(_opts)
unset(tmp)
unset(_DEPS)


#! iplug2_configure_target : Configure a target for the given output type
#
function(iplug2_configure_target target target_type)
  set_property(TARGET ${target} PROPERTY CXX_STANDARD ${IPLUG2_CXX_STANDARD})

  # ALL Configurations
  if (WIN32)
    # On Windows ours fonts are included in the RC file, meaning we need to include main.rc
    # in ALL our builds. Yay for platform-specific bundling!
    set(_res "${CMAKE_SOURCE_DIR}/resources/main.rc")
    iplug2_target_add(${target} PUBLIC RESOURCE ${_res})
    source_group("Resources" FILES ${_res})
    
  elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin") 
    # For MacOS we make sure the output name is the same as the app name.
    # This is basically required for bundles.
    set_property(TARGET ${target} PROPERTY OUTPUT_NAME "${IPLUG2_APP_NAME}")
  endif() 
  
  if ("${target_type}" MATCHES "app")
    iplug2_configure_app(${target})

  # VST2
  elseif ("${target_type}" MATCHES "vst2")
    if (WIN32)
      # After building, we run the post-build script
      add_custom_command(TARGET ${target} POST_BUILD 
        COMMAND "${CMAKE_BINARY_DIR}/postbuild-win.bat" 
        ARGS "\"$<TARGET_FILE:${target}>\"" "\".dll\""
      )
    endif()

  elseif ("${target_type}" MATCHES "vst3")
    iplug2_configure_vst3(${target})

  else()
    message("Unknown target type \'${target_type}\' for target '${target}'" FATAL_ERROR)
  endif()
endfunction()
