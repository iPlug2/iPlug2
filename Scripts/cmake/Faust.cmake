#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# Faust (https://faust.grame.fr) support for iPlug2
#
# Faust is not bundled with iPlug2. It is found in an existing installation (e.g. `brew install faust`,
# or the installers from https://github.com/grame-cncm/faust/releases), searched for in this order:
#   1. FAUST_DIR - the install prefix, containing bin/faust, include/faust, lib/(lib)faust and share/faust
#   2. the prefix of the faust executable on the PATH
#   3. standard locations (/opt/homebrew, /usr/local, /usr, C:/Program Files/Faust)
#
# Two ways of running Faust DSP code are supported:
#   INTERPRETER - FaustGen compiles the .dsp file at runtime with the libfaust interpreter backend, and
#                 recompiles it when the file changes. Requires libfaust. Slow, for development only.
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
#     [COMPILE_OPTIONS <opt> ...]    # Extra options for the faust compiler (COMPILED mode)
#   )
#   Call it after iplug_add_plugin(<plugin_name> ...). It defines FAUST_DSP_FILE_<CLASS> as the absolute path of the
#   .dsp file, and FAUST_COMPILED for configurations that use the compiled code.
#
# Variables:
#   IPLUG2_FAUST_SUPPORTED             - TRUE if Faust was found (headers + the interpreter and/or the compiler)
#   IPLUG2_FAUST_INTERPRETER_SUPPORTED - TRUE if libfaust was found
#   IPLUG2_FAUST_COMPILER_SUPPORTED    - TRUE if the faust executable was found

include(${CMAKE_CURRENT_LIST_DIR}/IPlug.cmake)

if(NOT DEFINED IPLUG2_FAUST_SUPPORTED)
  set(FAUST_DIR "" CACHE PATH "Faust install prefix (containing bin/faust, include/faust, lib and share/faust)")

  set(_faust_hints "")
  if(FAUST_DIR)
    list(APPEND _faust_hints "${FAUST_DIR}")
  endif()

  find_program(FAUST_EXECUTABLE faust
    HINTS ${_faust_hints}
    PATH_SUFFIXES bin
    PATHS /opt/homebrew /usr/local /usr "C:/Program Files/Faust"
  )

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
  find_path(FAUST_SHARE_DIR stdfaust.lib
    HINTS ${_faust_hints} PATHS ${_faust_paths} PATH_SUFFIXES share/faust
  )
  if(WIN32)
    find_file(FAUST_DLL faust.dll
      HINTS ${_faust_hints} PATHS ${_faust_paths} PATH_SUFFIXES lib bin
    )
  endif()

  set(_faust_interpreter FALSE)
  set(_faust_compiler FALSE)

  if(FAUST_INCLUDE_DIR AND EXISTS "${FAUST_INCLUDE_DIR}/faust/midi/iplug2-midi.h")
    if(FAUST_LIBRARY AND EXISTS "${FAUST_INCLUDE_DIR}/faust/dsp/interpreter-dsp.h"
       AND NOT CMAKE_SYSTEM_NAME MATCHES "iOS|visionOS|Emscripten")
      set(_faust_interpreter TRUE)

      # libfaust has to contain all the architectures we build for
      list(LENGTH CMAKE_OSX_ARCHITECTURES _faust_n_archs)
      if(APPLE AND _faust_n_archs GREATER 0)
        execute_process(COMMAND lipo -archs "${FAUST_LIBRARY}" OUTPUT_VARIABLE _faust_lib_archs ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
        foreach(_arch ${CMAKE_OSX_ARCHITECTURES})
          if(NOT _faust_lib_archs MATCHES "${_arch}")
            message(STATUS "Faust: ${FAUST_LIBRARY} doesn't contain ${_arch} (has: ${_faust_lib_archs}), the interpreter is not available")
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
    message(STATUS "Faust: found headers in ${FAUST_INCLUDE_DIR} (interpreter: ${_faust_interpreter}, compiler: ${_faust_compiler})")
  else()
    set(IPLUG2_FAUST_SUPPORTED FALSE)
    message(STATUS "Faust not found (set FAUST_DIR to a Faust installation). Faust targets will not be available.")
  endif()
endif()

if(IPLUG2_FAUST_SUPPORTED AND NOT TARGET iPlug2::Faust)
  set(_faust_iplug_dir ${IPLUG_DIR}/Extras/Faust)

  # FAUST_INCLUDE_DIR may be a shared prefix such as /opt/homebrew/include. To avoid picking up other headers
  # from there (e.g. a different libpng), only the faust folder is exposed, via a link in the build tree
  set(_faust_inc_dir "${CMAKE_BINARY_DIR}/iplug2-faust/include")
  if(NOT EXISTS "${_faust_inc_dir}/faust")
    file(MAKE_DIRECTORY "${_faust_inc_dir}")
    file(CREATE_LINK "${FAUST_INCLUDE_DIR}/faust" "${_faust_inc_dir}/faust" SYMBOLIC COPY_ON_ERROR)
  endif()

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
    target_link_libraries(iPlug2::FaustInterpreter INTERFACE iPlug2::Faust ${FAUST_LIBRARY})

    if(FAUST_SHARE_DIR)
      target_compile_definitions(iPlug2::FaustInterpreter INTERFACE FAUST_SHARE_PATH="${FAUST_SHARE_DIR}")
    endif()

    if(WIN32 AND FAUST_DLL)
      # faust.dll is delay-loaded by FaustGen from the Faust installation, so that it doesn't need to be next to the plug-in
      target_compile_definitions(iPlug2::FaustInterpreter INTERFACE FAUST_DLL_PATH="${FAUST_DLL}")
      target_link_libraries(iPlug2::FaustInterpreter INTERFACE delayimp.lib)
      target_link_options(iPlug2::FaustInterpreter INTERFACE "/DELAYLOAD:faust.dll")
    endif()
  endif()
endif()

# ============================================================================
# iplug_add_faust(<plugin_name> DSP <file.dsp> [CLASS <name>] [MODE AUTO|INTERPRETER|COMPILED] [COMPILE_OPTIONS ...])
# ============================================================================
function(iplug_add_faust plugin_name)
  cmake_parse_arguments(FAUST "" "DSP;CLASS;MODE" "COMPILE_OPTIONS" ${ARGN})

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
