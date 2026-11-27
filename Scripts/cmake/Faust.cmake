#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# Faust (https://faust.grame.fr) support for iPlug2
#
# Faust is not bundled with iPlug2. libfaust (for the interpreter) and the Faust headers are found in this order:
#   1. FAUST_DIR - an install prefix, containing bin/faust, include/faust, lib/(lib)faust and share/faust
#   2. on Apple platforms, a prebuilt static libfaust in FAUST_PREBUILT_DIR (default: Dependencies/Build/faust),
#      installed with Dependencies/download-faust.sh from https://github.com/iPlug2/faust-builder.
#      It has slices for macOS, iOS and visionOS, and the matching Faust libraries, so it can be shipped
#   3. a Faust installation: the prefix of the faust executable on the PATH (e.g. `brew install faust`, or the
#      installers from https://github.com/grame-cncm/faust/releases), or standard locations
# The faust executable (for COMPILED mode) is searched for in FAUST_DIR, on the PATH and in standard locations.
#
# Two ways of running Faust DSP code are supported:
#   INTERPRETER - FaustGen compiles the .dsp file at runtime with the libfaust interpreter backend, and
#                 recompiles it when the file changes. Requires libfaust. Slower than compiled code.
#                 With BUNDLE (see below) the .dsp file and the Faust libraries are copied into the bundle,
#                 so that the interpreter can be shipped (e.g. to let users edit or load DSP code, or on iOS)
#   COMPILED    - the faust compiler generates C++ from the .dsp file at build time (with the IPlugFaust_arch.cpp
#                 architecture file), with no runtime dependencies. Requires the faust executable.
#
# Provides:
#   iPlug2::Faust - the IPlugFaust base class and the Faust architecture headers (header only Faust API)
#   iPlug2::FaustInterpreter - libfaust, for FaustGen (IPlugFaustGen.cpp)
#   iplug_add_faust(<plugin_name>
#     DSP <file.dsp>                 # The Faust DSP file
#     [CLASS <name>]                 # The class name, used in FAUST_BLOCK(). Defaults to the file name
#     [MODE AUTO|INTERPRETER|COMPILED] # AUTO (default): the interpreter for Debug, compiled for other configurations
#     [BUNDLE]                       # Copy the .dsp file and the Faust libraries into the bundle's Resources/faust folder
#     [COMPILE_OPTIONS <opt> ...]    # Extra options for the faust compiler (COMPILED mode)
#   )
#   Call it after iplug_add_plugin(<plugin_name> ...). It defines FAUST_DSP_FILE_<CLASS> as the absolute path of the
#   .dsp file, FAUST_COMPILED for configurations that use the compiled code and FAUST_BUNDLED with BUNDLE.
#
# Variables:
#   IPLUG2_FAUST_SUPPORTED             - TRUE if Faust was found (headers + the interpreter and/or the compiler)
#   IPLUG2_FAUST_INTERPRETER_SUPPORTED - TRUE if libfaust was found
#   IPLUG2_FAUST_COMPILER_SUPPORTED    - TRUE if the faust executable was found
#   IPLUG2_FAUST_INCLUDE_DIR, IPLUG2_FAUST_LIBRARY, IPLUG2_FAUST_SHARE_DIR - what was found

include(${CMAKE_CURRENT_LIST_DIR}/IPlug.cmake)

if(NOT DEFINED IPLUG2_FAUST_SUPPORTED)
  set(FAUST_DIR "" CACHE PATH "Faust install prefix (containing bin/faust, include/faust, lib and share/faust)")
  set(FAUST_PREBUILT_DIR "${DEPS_DIR}/Build/faust" CACHE PATH "Prebuilt libfaust from Dependencies/download-faust.sh")

  set(_faust_hints "")
  if(FAUST_DIR)
    list(APPEND _faust_hints "${FAUST_DIR}")
  endif()

  find_program(FAUST_EXECUTABLE faust
    HINTS ${_faust_hints}
    PATH_SUFFIXES bin
    PATHS /opt/homebrew /usr/local /usr "C:/Program Files/Faust"
  )

  set(IPLUG2_FAUST_INCLUDE_DIR "")
  set(IPLUG2_FAUST_LIBRARY "")
  set(IPLUG2_FAUST_SHARE_DIR "")
  set(_faust_dll "")

  # The prebuilt xcframework slice for the platform we are building for
  set(_faust_xcframework "${FAUST_PREBUILT_DIR}/LibFaust.xcframework")
  set(_faust_prebuilt_lib "")
  if(APPLE AND NOT FAUST_DIR AND EXISTS "${_faust_xcframework}" AND EXISTS "${FAUST_PREBUILT_DIR}/include/faust")
    if(CMAKE_SYSTEM_NAME MATCHES "iOS|visionOS")
      if(IPLUG2_IOS_PLATFORM MATCHES "SIMULATOR" OR CMAKE_OSX_SYSROOT MATCHES "simulator")
        set(_faust_simulator TRUE)
      else()
        set(_faust_simulator FALSE)
      endif()

      if(IPLUG2_IOS_PLATFORM MATCHES "VISIONOS" OR CMAKE_SYSTEM_NAME STREQUAL "visionOS")
        set(_faust_slice xros-arm64)
        set(_faust_lib_name xros)
      else()
        set(_faust_slice ios-arm64)
        set(_faust_lib_name ios)
      endif()

      if(_faust_simulator)
        set(_faust_prebuilt_lib "${_faust_xcframework}/${_faust_slice}-simulator/${_faust_lib_name}_sim_libfaust.a")
      else()
        set(_faust_prebuilt_lib "${_faust_xcframework}/${_faust_slice}/${_faust_lib_name}_libfaust.a")
      endif()
    else()
      set(_faust_prebuilt_lib "${_faust_xcframework}/macos-arm64_x86_64/macos_libfaust.a")
    endif()

    if(NOT EXISTS "${_faust_prebuilt_lib}")
      message(STATUS "Faust: ${_faust_xcframework} has no slice for this platform (${_faust_prebuilt_lib})")
      set(_faust_prebuilt_lib "")
    endif()
  endif()

  if(_faust_prebuilt_lib)
    set(IPLUG2_FAUST_INCLUDE_DIR "${FAUST_PREBUILT_DIR}/include")
    set(IPLUG2_FAUST_LIBRARY "${_faust_prebuilt_lib}")

    # With Xcode, the SDK (device or simulator) is chosen at build time, so the slice is picked then (see below)
    if(CMAKE_SYSTEM_NAME MATCHES "iOS|visionOS" AND CMAKE_GENERATOR STREQUAL "Xcode")
      set(IPLUG2_FAUST_LIBRARY "${_faust_xcframework}")

      # The simulator slices are arm64 only, but iOS.cmake builds the simulator for arm64 and x86_64
      if(CMAKE_OSX_ARCHITECTURES MATCHES "x86_64")
        set(CMAKE_XCODE_ATTRIBUTE_EXCLUDED_ARCHS[sdk=iphonesimulator*] "x86_64")
      endif()
    endif()
    if(EXISTS "${FAUST_PREBUILT_DIR}/share/faust/stdfaust.lib")
      set(IPLUG2_FAUST_SHARE_DIR "${FAUST_PREBUILT_DIR}/share/faust")
    endif()
  else()
    if(FAUST_EXECUTABLE)
      get_filename_component(_faust_exe_dir "${FAUST_EXECUTABLE}" DIRECTORY)
      get_filename_component(_faust_exe_prefix "${_faust_exe_dir}" DIRECTORY)
      list(APPEND _faust_hints "${_faust_exe_prefix}")
    endif()

    set(_faust_paths /opt/homebrew /usr/local /usr "C:/Program Files/Faust")

    find_path(FAUST_INCLUDE_DIR faust/dsp/dsp.h
      HINTS ${_faust_hints} PATHS ${_faust_paths} PATH_SUFFIXES include
    )
    find_library(FAUST_LIBRARY NAMES faust libfaust
      HINTS ${_faust_hints} PATHS ${_faust_paths} PATH_SUFFIXES lib
    )
    if(WIN32)
      find_file(FAUST_DLL faust.dll
        HINTS ${_faust_hints} PATHS ${_faust_paths} PATH_SUFFIXES lib bin
      )
      set(_faust_dll "${FAUST_DLL}")
    endif()

    set(IPLUG2_FAUST_INCLUDE_DIR "${FAUST_INCLUDE_DIR}")
    # A libfaust for the host can't be used when cross-compiling for iOS/visionOS
    if(NOT CMAKE_SYSTEM_NAME MATCHES "iOS|visionOS")
      set(IPLUG2_FAUST_LIBRARY "${FAUST_LIBRARY}")
    endif()
  endif()

  if(NOT IPLUG2_FAUST_SHARE_DIR)
    find_path(FAUST_SHARE_DIR stdfaust.lib
      HINTS ${_faust_hints} PATHS /opt/homebrew /usr/local /usr "C:/Program Files/Faust" PATH_SUFFIXES share/faust
    )
    if(FAUST_SHARE_DIR)
      set(IPLUG2_FAUST_SHARE_DIR "${FAUST_SHARE_DIR}")
    endif()
  endif()

  set(_faust_interpreter FALSE)
  set(_faust_compiler FALSE)

  if(IPLUG2_FAUST_INCLUDE_DIR AND EXISTS "${IPLUG2_FAUST_INCLUDE_DIR}/faust/midi/iplug2-midi.h")
    if(IPLUG2_FAUST_LIBRARY AND EXISTS "${IPLUG2_FAUST_INCLUDE_DIR}/faust/dsp/interpreter-dsp.h"
       AND NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
      set(_faust_interpreter TRUE)

      # libfaust has to contain all the architectures we build for (an xcframework is checked by Xcode)
      list(LENGTH CMAKE_OSX_ARCHITECTURES _faust_n_archs)
      if(APPLE AND _faust_n_archs GREATER 0 AND NOT IPLUG2_FAUST_LIBRARY MATCHES "\\.xcframework$")
        execute_process(COMMAND lipo -archs "${IPLUG2_FAUST_LIBRARY}" OUTPUT_VARIABLE _faust_lib_archs ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
        foreach(_arch ${CMAKE_OSX_ARCHITECTURES})
          if(NOT _faust_lib_archs MATCHES "${_arch}")
            message(STATUS "Faust: ${IPLUG2_FAUST_LIBRARY} doesn't contain ${_arch} (has: ${_faust_lib_archs}), the interpreter is not available")
            set(_faust_interpreter FALSE)
          endif()
        endforeach()
      endif()
    endif()

    if(FAUST_EXECUTABLE)
      set(_faust_compiler TRUE)
    endif()
  endif()

  set(IPLUG2_FAUST_INTERPRETER_SUPPORTED ${_faust_interpreter})
  set(IPLUG2_FAUST_COMPILER_SUPPORTED ${_faust_compiler})

  if(_faust_interpreter OR _faust_compiler)
    set(IPLUG2_FAUST_SUPPORTED TRUE)
    if(_faust_prebuilt_lib)
      set(_faust_source "prebuilt ${IPLUG2_FAUST_LIBRARY}")
    else()
      set(_faust_source "headers in ${IPLUG2_FAUST_INCLUDE_DIR}")
    endif()
    message(STATUS "Faust: ${_faust_source} (interpreter: ${_faust_interpreter}, compiler: ${_faust_compiler}, libraries: ${IPLUG2_FAUST_SHARE_DIR})")
  else()
    set(IPLUG2_FAUST_SUPPORTED FALSE)
    message(STATUS "Faust not found (run Dependencies/download-faust.sh, install Faust or set FAUST_DIR). Faust targets will not be available.")
  endif()
endif()

if(IPLUG2_FAUST_SUPPORTED AND NOT TARGET iPlug2::Faust)
  set(_faust_iplug_dir ${IPLUG_DIR}/Extras/Faust)

  # FAUST_INCLUDE_DIR may be a shared prefix such as /opt/homebrew/include. To avoid picking up other headers
  # from there (e.g. a different libpng), only the faust folder is exposed, via a link in the build tree
  set(_faust_inc_dir "${CMAKE_BINARY_DIR}/iplug2-faust/include")
  file(REMOVE_RECURSE "${_faust_inc_dir}")
  file(MAKE_DIRECTORY "${_faust_inc_dir}")
  file(CREATE_LINK "${IPLUG2_FAUST_INCLUDE_DIR}/faust" "${_faust_inc_dir}/faust" SYMBOLIC COPY_ON_ERROR)

  add_library(iPlug2::Faust INTERFACE IMPORTED)
  target_sources(iPlug2::Faust INTERFACE
    ${_faust_iplug_dir}/IPlugFaust.h
    ${_faust_iplug_dir}/IPlugFaust.cpp
    ${_faust_iplug_dir}/IPlugFaustGen.h
    ${_faust_iplug_dir}/IGraphicsFaustUI.h
  )
  target_include_directories(iPlug2::Faust INTERFACE ${_faust_iplug_dir})
  target_include_directories(iPlug2::Faust SYSTEM INTERFACE ${_faust_inc_dir})
  target_link_libraries(iPlug2::Faust INTERFACE iPlug2::IPlug)

  if(IPLUG2_FAUST_INTERPRETER_SUPPORTED)
    # libfaust and its settings. IPlugFaustGen.cpp is added separately by iplug_add_faust(), because sources
    # can't vary by configuration with the Xcode generator (it compiles to nothing when FAUST_COMPILED is defined)
    add_library(iPlug2::FaustInterpreter INTERFACE IMPORTED)

    if(IPLUG2_FAUST_LIBRARY MATCHES "\\.xcframework$")
      # Link the xcframework slice for the SDK Xcode builds for: each slice is linked as libiplug2faust.a in a folder named
      # after the Xcode PLATFORM_NAME (iphoneos, iphonesimulator, xros, xrsimulator), which is expanded at build time
      set(_faust_slices_dir "${CMAKE_BINARY_DIR}/iplug2-faust/lib")
      file(REMOVE_RECURSE "${_faust_slices_dir}")
      foreach(_slice "ios-arm64/ios_libfaust.a:iphoneos" "ios-arm64-simulator/ios_sim_libfaust.a:iphonesimulator"
                     "xros-arm64/xros_libfaust.a:xros" "xros-arm64-simulator/xros_sim_libfaust.a:xrsimulator")
        string(REPLACE ":" ";" _slice "${_slice}")
        list(GET _slice 0 _slice_lib)
        list(GET _slice 1 _platform_name)
        if(EXISTS "${IPLUG2_FAUST_LIBRARY}/${_slice_lib}")
          file(MAKE_DIRECTORY "${_faust_slices_dir}/${_platform_name}")
          file(CREATE_LINK "${IPLUG2_FAUST_LIBRARY}/${_slice_lib}" "${_faust_slices_dir}/${_platform_name}/libiplug2faust.a" SYMBOLIC COPY_ON_ERROR)
        endif()
      endforeach()
      target_link_options(iPlug2::FaustInterpreter INTERFACE "-L${_faust_slices_dir}/$(PLATFORM_NAME)" "-liplug2faust")
      target_link_libraries(iPlug2::FaustInterpreter INTERFACE iPlug2::Faust)
    else()
      target_link_libraries(iPlug2::FaustInterpreter INTERFACE iPlug2::Faust "${IPLUG2_FAUST_LIBRARY}")
    endif()

    if(IPLUG2_FAUST_SHARE_DIR)
      target_compile_definitions(iPlug2::FaustInterpreter INTERFACE FAUST_SHARE_PATH="${IPLUG2_FAUST_SHARE_DIR}")
    endif()

    if(WIN32 AND _faust_dll)
      # faust.dll is delay-loaded by FaustGen from the Faust installation, so that it doesn't need to be next to the plug-in
      target_compile_definitions(iPlug2::FaustInterpreter INTERFACE FAUST_DLL_PATH="${_faust_dll}")
      target_link_libraries(iPlug2::FaustInterpreter INTERFACE delayimp.lib)
      target_link_options(iPlug2::FaustInterpreter INTERFACE "/DELAYLOAD:faust.dll")
    endif()
  endif()
endif()

# ============================================================================
# Copies the .dsp file and the Faust libraries into the Resources/faust folder of the plug-in's bundles, so that
# FaustGen can compile the DSP at runtime in a shipped build (see FaustGen::SetBundleResourcePath())
# ============================================================================
function(_iplug_faust_bundle_files plugin_name dsp_file)
  if(NOT APPLE)
    # Resources are only copied into bundles on Apple platforms. Elsewhere, ship the files with your installer
    message(WARNING "iplug_add_faust(${plugin_name}): BUNDLE is only supported on Apple platforms")
    return()
  endif()

  set(base_target _${plugin_name}-base)
  target_compile_definitions(${base_target} INTERFACE FAUST_BUNDLED)

  target_sources(${base_target} INTERFACE "${dsp_file}")
  set_source_files_properties("${dsp_file}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/faust" HEADER_FILE_ONLY TRUE)

  if(NOT IPLUG2_FAUST_SHARE_DIR)
    message(WARNING "iplug_add_faust(${plugin_name}): the Faust libraries (stdfaust.lib etc.) were not found, so they won't be bundled")
    return()
  endif()

  file(GLOB_RECURSE faust_libs RELATIVE "${IPLUG2_FAUST_SHARE_DIR}" "${IPLUG2_FAUST_SHARE_DIR}/*.lib")

  foreach(lib ${faust_libs})
    get_filename_component(lib_dir "${lib}" DIRECTORY)
    set(location "Resources/faust")
    if(lib_dir)
      string(APPEND location "/${lib_dir}")
    endif()
    set(lib_path "${IPLUG2_FAUST_SHARE_DIR}/${lib}")
    target_sources(${base_target} INTERFACE "${lib_path}")
    set_source_files_properties("${lib_path}" PROPERTIES MACOSX_PACKAGE_LOCATION "${location}" HEADER_FILE_ONLY TRUE)
  endforeach()
endfunction()

# ============================================================================
# iplug_add_faust(<plugin_name> DSP <file.dsp> [CLASS <name>] [MODE AUTO|INTERPRETER|COMPILED] [BUNDLE] [COMPILE_OPTIONS ...])
# ============================================================================
function(iplug_add_faust plugin_name)
  cmake_parse_arguments(FAUST "BUNDLE" "DSP;CLASS;MODE" "COMPILE_OPTIONS" ${ARGN})

  if(FAUST_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "iplug_add_faust(${plugin_name}): Unknown arguments: ${FAUST_UNPARSED_ARGUMENTS}")
  endif()

  if(NOT IPLUG2_FAUST_SUPPORTED)
    message(FATAL_ERROR "iplug_add_faust(${plugin_name}): Faust was not found. Install it or set FAUST_DIR.")
  endif()

  set(base_target _${plugin_name}-base)
  if(NOT TARGET ${base_target})
    message(FATAL_ERROR "iplug_add_faust(${plugin_name}): call iplug_add_plugin(${plugin_name} ...) first")
  endif()

  if(NOT FAUST_DSP)
    message(FATAL_ERROR "iplug_add_faust(${plugin_name}): DSP is required")
  endif()

  get_filename_component(dsp_file "${FAUST_DSP}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
  if(NOT EXISTS "${dsp_file}")
    message(FATAL_ERROR "iplug_add_faust(${plugin_name}): ${dsp_file} does not exist")
  endif()
  get_filename_component(dsp_dir "${dsp_file}" DIRECTORY)

  if(NOT FAUST_CLASS)
    get_filename_component(FAUST_CLASS "${dsp_file}" NAME_WE)
  endif()
  string(MAKE_C_IDENTIFIER "${FAUST_CLASS}" class_name)

  if(NOT FAUST_MODE)
    set(FAUST_MODE AUTO)
  endif()

  # Work out which configurations use the interpreter and which use compiled code
  if(FAUST_MODE STREQUAL "INTERPRETER")
    if(NOT IPLUG2_FAUST_INTERPRETER_SUPPORTED)
      message(FATAL_ERROR "iplug_add_faust(${plugin_name}): MODE INTERPRETER requires libfaust, which was not found")
    endif()
    set(interpreter_condition 1)
    set(compiled_condition 0)
  elseif(FAUST_MODE STREQUAL "COMPILED")
    if(NOT IPLUG2_FAUST_COMPILER_SUPPORTED)
      message(FATAL_ERROR "iplug_add_faust(${plugin_name}): MODE COMPILED requires the faust executable, which was not found")
    endif()
    set(interpreter_condition 0)
    set(compiled_condition 1)
  elseif(FAUST_MODE STREQUAL "AUTO")
    if(IPLUG2_FAUST_INTERPRETER_SUPPORTED AND IPLUG2_FAUST_COMPILER_SUPPORTED)
      set(interpreter_condition "$<CONFIG:Debug>")
      set(compiled_condition "$<NOT:$<CONFIG:Debug>>")
    elseif(IPLUG2_FAUST_COMPILER_SUPPORTED)
      set(interpreter_condition 0)
      set(compiled_condition 1)
    else()
      message(WARNING "iplug_add_faust(${plugin_name}): the faust executable was not found, so all configurations will use the (slow) interpreter")
      set(interpreter_condition 1)
      set(compiled_condition 0)
    endif()
  else()
    message(FATAL_ERROR "iplug_add_faust(${plugin_name}): invalid MODE ${FAUST_MODE}, use AUTO, INTERPRETER or COMPILED")
  endif()

  target_link_libraries(${base_target} INTERFACE iPlug2::Faust)
  target_compile_definitions(${base_target} INTERFACE "FAUST_DSP_FILE_${class_name}=\"${dsp_file}\"")

  if(NOT interpreter_condition STREQUAL "0")
    target_sources(${base_target} INTERFACE ${IPLUG_DIR}/Extras/Faust/IPlugFaustGen.cpp)
    target_link_libraries(${base_target} INTERFACE "$<${interpreter_condition}:iPlug2::FaustInterpreter>")
  endif()

  if(FAUST_BUNDLE)
    if(interpreter_condition STREQUAL "0")
      message(WARNING "iplug_add_faust(${plugin_name}): BUNDLE has no effect without the interpreter (MODE ${FAUST_MODE})")
    else()
      _iplug_faust_bundle_files(${plugin_name} "${dsp_file}")
    endif()
  endif()

  if(NOT compiled_condition STREQUAL "0")
    set(gen_dir "${CMAKE_CURRENT_BINARY_DIR}/${plugin_name}-faust")
    set(gen_file "${gen_dir}/${class_name}.hpp")
    set(arch_file "${IPLUG_DIR}/Extras/Faust/IPlugFaust_arch.cpp")

    add_custom_command(
      OUTPUT "${gen_file}"
      COMMAND "${FAUST_EXECUTABLE}" -i -double -cn ${class_name} -a "${arch_file}" -I "${dsp_dir}" ${FAUST_COMPILE_OPTIONS} -o "${gen_file}" "${dsp_file}"
      DEPENDS "${dsp_file}" "${arch_file}"
      COMMENT "Compiling ${dsp_file} with faust"
      VERBATIM
    )
    add_custom_target(${plugin_name}-faust-${class_name} DEPENDS "${gen_file}")
    add_dependencies(${base_target} ${plugin_name}-faust-${class_name})

    # FaustCode.hpp includes the generated code of all the plug-in's Faust classes (it is included by IPlugFaustGen.h)
    set_property(GLOBAL APPEND PROPERTY IPLUG_FAUST_CLASSES_${plugin_name} ${class_name})
    get_property(faust_classes GLOBAL PROPERTY IPLUG_FAUST_CLASSES_${plugin_name})
    list(REMOVE_DUPLICATES faust_classes)
    set(faust_code "// Generated by iplug_add_faust(), do not edit\n#pragma once\n")
    foreach(cls ${faust_classes})
      string(APPEND faust_code "#include \"${cls}.hpp\"\n")
    endforeach()

    # Only write it if it changed, to avoid rebuilds
    set(faust_code_file "${gen_dir}/FaustCode.hpp")
    set(existing_faust_code "")
    if(EXISTS "${faust_code_file}")
      file(READ "${faust_code_file}" existing_faust_code)
    endif()
    if(NOT existing_faust_code STREQUAL faust_code)
      file(WRITE "${faust_code_file}" "${faust_code}")
    endif()

    target_include_directories(${base_target} INTERFACE "$<${compiled_condition}:${gen_dir}>")
    target_compile_definitions(${base_target} INTERFACE "$<${compiled_condition}:FAUST_COMPILED>")
  endif()
endfunction()
