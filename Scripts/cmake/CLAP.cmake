#  ==============================================================================
#  
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

cmake_minimum_required(VERSION 3.11)

set(CLAP_SDK "${IPLUG2_DIR}/Dependencies/IPlug/CLAP_SDK" CACHE PATH "CLAP SDK directory.")
set(CLAP_HELPERS "${IPLUG2_DIR}/Dependencies/IPlug/CLAP_HELPERS" CACHE PATH "CLAP HELPERS directory.")

if (WIN32)
  if (CMAKE_SYSTEM_PROCESSOR MATCHES "X86")
    set(_paths "C:/Program Files (x86)/Common Files/CLAP" "C:/Program Files/Common Files/CLAP")
  elseif ((CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "AMD64") OR (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "IA64"))
    set(_paths "C:/Program Files/Common Files/CLAP")
  endif()
elseif (APPLE)
  set(_paths "$ENV{HOME}/Library/Audio/Plug-Ins/CLAP" "/Library/Audio/Plug-Ins/CLAP")
elseif (UNIX AND NOT APPLE)
  set(_paths "$ENV{HOME}/.clap" "/usr/local/lib/clap" "/usr/local/clap")
endif()

iplug_find_path(CLAP_INSTALL_PATH REQUIRED DIR DEFAULT_IDX 0 
  DOC "Path to install CLAP plugins"
  PATHS ${_paths})

set(sdk ${IPLUG2_DIR}/IPlug/CLAP)
add_library(iPlug2_CLAP INTERFACE)
iplug_target_add(iPlug2_CLAP INTERFACE
  INCLUDE ${sdk} ${CLAP_SDK}/include ${CLAP_HELPERS} ${CLAP_HELPERS}/include/clap/helpers
  SOURCE ${sdk}/IPlugCLAP.cpp
  DEFINE "CLAP_API" "IPLUG_DSP=1" "IPLUG_EDITOR=1"
  LINK iPlug2_Core
)

list(APPEND IPLUG2_TARGETS iPlug2_CLAP)

function(iplug_configure_clap target)
  iplug_target_add(${target} PUBLIC LINK iPlug2_CLAP)

  set(out_dir "${CMAKE_BINARY_DIR}/out/${PLUG_NAME}.clap")
  set(install_dir "${CLAP_INSTALL_PATH}/${PLUG_NAME}.clap")

  if (WIN32)
    set(res_dir "${CMAKE_BINARY_DIR}/clap-resources")

    set_target_properties(${target} PROPERTIES
      OUTPUT_NAME "${PLUG_NAME}"
      RUNTIME_OUTPUT_DIRECTORY "${out_dir}"
      LIBRARY_OUTPUT_DIRECTORY "${out_dir}"
      PREFIX ""
      SUFFIX ".clap"
    )

    add_custom_command(TARGET ${target} POST_BUILD 
      COMMAND "${CMAKE_BINARY_DIR}/postbuild-win.bat" 
      ARGS "\"$<TARGET_FILE:${target}>\"" "\".clap\""
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_PDB_FILE:${target}>" "${CMAKE_BINARY_DIR}/out/${PLUG_NAME}-clap.pdb" || echo "No PDB found for CLAP"
    )
    
  elseif (APPLE)
    set(res_dir "${out_dir}/Contents/Resources")

    set_target_properties(${target} PROPERTIES
      BUNDLE TRUE
      MACOSX_BUNDLE TRUE
      MACOSX_BUNDLE_INFO_PLIST ${PLUG_RESOURCES_DIR}/${PLUG_NAME}-CLAP-Info.plist
      BUNDLE_EXTENSION "clap"
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
      COMMAND dsymutil "$<TARGET_FILE:${target}>" -o "${CMAKE_BINARY_DIR}/out/${PLUG_NAME}.clap.dSYM"
      COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_BUNDLE_DIR:${target}>.dSYM" "${CMAKE_BINARY_DIR}/out/${PLUG_NAME}.clap.dSYM" || echo "No dSYM found for CLAP"
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
