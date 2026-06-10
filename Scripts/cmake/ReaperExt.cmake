#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# REAPER Extension configuration for iPlug2
# This module provides ReaperExtBase for building REAPER extension plugins

include(${CMAKE_CURRENT_LIST_DIR}/IPlug.cmake)

if(NOT TARGET iPlug2::ReaperExt)
  add_library(iPlug2::ReaperExt INTERFACE IMPORTED)

  set(REAPER_EXT_DIR ${IPLUG2_DIR}/IPlug/ReaperExt)
  set(REAPER_SDK_DIR ${IPLUG_DEPS_DIR}/REAPER_SDK)
  set(SWELL_DIR ${WDL_DIR}/swell)

  # Note: ReaperExtBase.cpp is #included by ReaperExt_include_in_plug_src.h
  # so we don't add it as a separate source file
  set(REAPER_EXT_SRC
    ${REAPER_EXT_DIR}/ReaperExtBase.h
    ${REAPER_EXT_DIR}/ReaperExt_include_in_plug_hdr.h
    ${REAPER_EXT_DIR}/ReaperExt_include_in_plug_src.h
  )

  target_sources(iPlug2::ReaperExt INTERFACE ${REAPER_EXT_SRC})

  target_include_directories(iPlug2::ReaperExt INTERFACE
    ${REAPER_EXT_DIR}
    ${REAPER_SDK_DIR}
  )

  target_compile_definitions(iPlug2::ReaperExt INTERFACE
    REAPER_EXT_API
    IPLUG_EDITOR=1
    IPLUG_DSP=1
  )

  if(WIN32)
    # Windows builds as DLL
    target_link_libraries(iPlug2::ReaperExt INTERFACE
      iPlug2::IPlug
    )
  elseif(APPLE)
    # macOS needs SWELL for dialogs and window management
    target_include_directories(iPlug2::ReaperExt INTERFACE ${SWELL_DIR})

    set(SWELL_SRC
      "${SWELL_DIR}/swell.cpp"
      "${SWELL_DIR}/swell-ini.cpp"
      "${SWELL_DIR}/swell-dlg.mm"
      "${SWELL_DIR}/swell-gdi.mm"
      "${SWELL_DIR}/swell-kb.mm"
      "${SWELL_DIR}/swell-menu.mm"
      "${SWELL_DIR}/swell-misc.mm"
      "${SWELL_DIR}/swell-miscdlg.mm"
      "${SWELL_DIR}/swell-wnd.mm"
    )

    target_sources(iPlug2::ReaperExt INTERFACE ${SWELL_SRC})

    # SWELL uses deprecated functions
    set_source_files_properties(${SWELL_SRC}
      PROPERTIES
      COMPILE_FLAGS "-Wno-deprecated-declarations"
    )

    target_compile_definitions(iPlug2::ReaperExt INTERFACE
      SWELL_PROVIDED_BY_APP
    )

    target_link_libraries(iPlug2::ReaperExt INTERFACE
      "-framework Cocoa"
      "-framework Carbon"
      "-undefined dynamic_lookup"
      iPlug2::IPlug
    )
  endif()
endif()

# Configure a REAPER extension target
function(iplug_configure_reaperext target project_name)
  target_link_libraries(${target} PUBLIC iPlug2::ReaperExt)

  set_target_properties(${target} PROPERTIES
    CXX_STANDARD ${IPLUG2_CXX_STANDARD}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
  )

  if(APPLE)
    target_compile_definitions(${target} PRIVATE OBJC_PREFIX=v${project_name}_reaperext)

    iplug_apply_objc_prefix_header(${target})
  endif()

  if(WIN32)
    set(REAPER_EXT_OUTPUT_DIR "${CMAKE_BINARY_DIR}/out")
    set_target_properties(${target} PROPERTIES
      OUTPUT_NAME "${project_name}"
      LIBRARY_OUTPUT_DIRECTORY "${REAPER_EXT_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_DEBUG "${REAPER_EXT_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_RELEASE "${REAPER_EXT_OUTPUT_DIR}"
      SUFFIX ".dll"
    )
  elseif(APPLE)
    set(REAPER_EXT_OUTPUT_DIR "${CMAKE_BINARY_DIR}/out")
    set_target_properties(${target} PROPERTIES
      OUTPUT_NAME "${project_name}"
      LIBRARY_OUTPUT_DIRECTORY "${REAPER_EXT_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_DEBUG "${REAPER_EXT_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_RELEASE "${REAPER_EXT_OUTPUT_DIR}"
      SUFFIX ".dylib"
      PREFIX ""
      # Skip code signing during build
      XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
    )
  endif()

  # Auto-deploy to REAPER UserPlugins if enabled
  if(IPLUG_DEPLOY_PLUGINS)
    iplug_deploy_target(${target} REAPEREXT ${project_name})
  endif()
endfunction()
