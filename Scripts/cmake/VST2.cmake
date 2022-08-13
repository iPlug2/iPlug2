#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

cmake_minimum_required(VERSION 3.11)

set(VST2_SDK "${IPLUG2_DIR}/Dependencies/IPlug/VST2_SDK" CACHE PATH "VST2 SDK directory.")

if (WIN32)
  if (PROCESSOR_ARCH STREQUAL "Win32")
    set(_paths "$ENV{ProgramFiles(x86)}/${fn}" "$ENV{ProgramFiles(x86)}/Steinberg/VstPlugins")
  endif()
  list(APPEND _paths "'$ENV{ProgramFiles}/${fn}'" "'$ENV{ProgramFiles}/Steinberg/VstPlugins'")
elseif (APPLE)
  set(fn "VST")
  set(_paths "$ENV{HOME}/Library/Audio/Plug-Ins/${fn}" "/Library/Audio/Plug-Ins/${fn}")
elseif (UNIX AND NOT APPLE)
  set(_paths "$ENV{HOME}/.vst" "/usr/local/lib/vst" "/usr/local/vst")
endif()

iplug_find_path(VST2_INSTALL_PATH REQUIRED DIR DEFAULT_IDX 0
  DOC "Path to install VST2 plugins"
  PATHS ${_paths})

set(sdk ${IPLUG2_DIR}/IPlug/VST2)
add_library(iPlug2_VST2 INTERFACE)
iplug_target_add(iPlug2_VST2 INTERFACE
  INCLUDE ${sdk} ${VST2_SDK}
  SOURCE ${sdk}/IPlugVST2.cpp
  DEFINE "VST2_API" "VST_FORCE_DEPRECATED" "IPLUG_DSP=1" "IPLUG_EDITOR=1"
  LINK iPlug2_Core
)

list(APPEND IPLUG2_TARGETS iPlug2_VST2)

function(iplug_configure_vst2 target)
  iplug_target_add(${target} PUBLIC LINK iPlug2_VST2)

  set(out_dir "${CMAKE_BINARY_DIR}/out/${PLUG_NAME}.vst")
  set(install_dir "${VST2_INSTALL_PATH}/${PLUG_NAME}.vst")

  if (WIN32)
    set(res_dir "${CMAKE_BINARY_DIR}/vst2-resources")

    set_target_properties(${target} PROPERTIES
      OUTPUT_NAME "${PLUG_NAME}"
      RUNTIME_OUTPUT_DIRECTORY "${out_dir}"
      LIBRARY_OUTPUT_DIRECTORY "${out_dir}"
      PREFIX ""
      SUFFIX ".dll"
    )

    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND "${CMAKE_BINARY_DIR}/postbuild-win.bat"
      ARGS "\"$<TARGET_FILE:${target}>\"" "\".dll\""
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_PDB_FILE:${target}>" "${CMAKE_BINARY_DIR}/out/${PLUG_NAME}-vst2.pdb" || echo "No PDB found for VST2"
    )

  elseif (APPLE)
    set(res_dir "${out_dir}/Contents/Resources")

    set_target_properties(${target} PROPERTIES
      BUNDLE TRUE
      MACOSX_BUNDLE TRUE
      MACOSX_BUNDLE_INFO_PLIST ${PLUG_RESOURCES_DIR}/${PLUG_NAME}-VST2-Info.plist
      BUNDLE_EXTENSION "vst"
      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out"
      PREFIX ""
      SUFFIX ""
      XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym"
      XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES"
    )

    # Make sure Xcode generator uses the same output directories
    if(CMAKE_GENERATOR STREQUAL "Xcode")
      set_target_properties(${target} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/out"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/out"
      )
    endif()

    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "${out_dir}/Contents/MacOS"
      COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${target}>" "${out_dir}/Contents/MacOS"
      COMMAND ${CMAKE_COMMAND} -E copy_directory "${out_dir}" "${install_dir}"
      COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/out"
      COMMAND dsymutil "$<TARGET_FILE:${target}>" -o "${CMAKE_BINARY_DIR}/out/${PLUG_NAME}.vst2.dSYM"
      COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_BUNDLE_DIR:${target}>.dSYM" "${CMAKE_BINARY_DIR}/out/${PLUG_NAME}.vst2.dSYM" || echo "No dSYM found for VST2"
    )

    set(PKGINFO_FILE "${out_dir}/Contents/PkgInfo")
    file(WRITE ${PKGINFO_FILE} "BNDL????")
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E touch ${PKGINFO_FILE})

  elseif (UNIX AND NOT APPLE)
    set_target_properties(${target} PROPERTIES
      OUTPUT_NAME "${PLUG_NAME}"
      LIBRARY_OUTPUT_DIRECTORY "${out_dir}"
      PREFIX ""
      SUFFIX ".so"
    )
    set(res_dir "${out_dir}/resources")

    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory "${out_dir}" "${install_dir}"
    )
  endif()

  iplug_target_bundle_resources(${target} "${res_dir}")

endfunction()