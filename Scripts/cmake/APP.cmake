#  ==============================================================================
#  
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# APP target configuration for iPlug2

include(${CMAKE_CURRENT_LIST_DIR}/IPlug.cmake)

if(NOT TARGET iPlug2::APP)
  add_library(iPlug2::APP INTERFACE IMPORTED)

  set(SWELL_DIR ${WDL_DIR}/swell)

  if(NOT PLUG_RESOURCES_DIR)
    set(PLUG_RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources)
    message(STATUS "Setting PLUG_RESOURCES_DIR to ${PLUG_RESOURCES_DIR}")
  endif()

  # RTAudio and RTMidi directories
  set(RTAUDIO_DIR ${IPLUG_DEPS_DIR}/RTAudio)
  set(RTMIDI_DIR ${IPLUG_DEPS_DIR}/RTMidi)
  
  set(APP_SRC
    ${IPLUG2_DIR}/IPlug/APP/IPlugAPP.cpp 
    ${IPLUG2_DIR}/IPlug/APP/IPlugAPP_dialog.cpp 
    ${IPLUG2_DIR}/IPlug/APP/IPlugAPP_host.cpp 
    ${IPLUG2_DIR}/IPlug/APP/IPlugAPP_main.cpp
    ${RTAUDIO_DIR}/RtAudio.cpp
    ${RTMIDI_DIR}/RtMidi.cpp
  )
  
  target_sources(iPlug2::APP INTERFACE ${APP_SRC})
  
  target_include_directories(iPlug2::APP INTERFACE 
    ${IPLUG2_DIR}/IPlug/APP
    ${RTAUDIO_DIR}
    ${RTAUDIO_DIR}/include
    ${RTMIDI_DIR}
  )
  
  target_compile_definitions(iPlug2::APP INTERFACE 
    APP_API 
    IPLUG_EDITOR=1 
    IPLUG_DSP=1
  )
  
  if(WIN32)
    target_sources(iPlug2::APP INTERFACE
      ${RTAUDIO_DIR}/include/asio.cpp
      ${RTAUDIO_DIR}/include/asiodrivers.cpp
      ${RTAUDIO_DIR}/include/asiolist.cpp
      ${RTAUDIO_DIR}/include/iasiothiscallresolver.cpp
    )
    target_compile_definitions(iPlug2::APP INTERFACE 
      __WINDOWS_DS__ 
      __WINDOWS_MM__ 
      __WINDOWS_ASIO__
    )
    target_link_libraries(iPlug2::APP INTERFACE 
      dsound.lib 
      winmm.lib
    )
  elseif(APPLE)
    set_source_files_properties(${APP_SRC} PROPERTIES LANGUAGE OBJCXX)
    target_include_directories(iPlug2::APP INTERFACE ${SWELL_DIR})

    set(SWELL_SRC
      "${SWELL_DIR}/swell-appstub.mm"
      "${SWELL_DIR}/swellappmain.mm"
      "${SWELL_DIR}/swell-ini.cpp"
      "${SWELL_DIR}/swell-dlg.mm"
      "${SWELL_DIR}/swell-kb.mm"
      "${SWELL_DIR}/swell-miscdlg.mm"
      "${SWELL_DIR}/swell-menu.mm"
      "${SWELL_DIR}/swell-wnd.mm"
      "${SWELL_DIR}/swell.cpp"
      "${SWELL_DIR}/swell-misc.mm"
      "${SWELL_DIR}/swell-gdi.mm"
    )

    target_sources(iPlug2::APP INTERFACE ${SWELL_SRC})

    # swell uses a lot of deprecated functions
    set_source_files_properties(${SWELL_SRC}
      PROPERTIES
      COMPILE_FLAGS "-Wno-deprecated-declarations"
    )

    target_compile_definitions(iPlug2::APP INTERFACE 
      __MACOSX_CORE__
      SWELL_COMPILED
    )
    target_link_libraries(iPlug2::APP INTERFACE
      "-framework AppKit"
      "-framework Carbon"
      "-framework CoreMIDI"
      "-framework CoreAudio"
    )
  elseif(UNIX AND NOT APPLE)
    message("Error - Linux not yet supported")
  endif()
  
  target_link_libraries(iPlug2::APP INTERFACE iPlug2::IPlug)
endif()

function(iplug_configure_app target project_name)
  target_link_libraries(${target} PUBLIC iPlug2::APP)
  
  if(WIN32)
    set_target_properties(${target} PROPERTIES
      OUTPUT_NAME "${PLUG_NAME}"
      RUNTIME_OUTPUT_DIRECTORY "${PLUG_NAME}-app"
    )
  elseif(APPLE)
    set_target_properties(${target} PROPERTIES
      MACOSX_BUNDLE TRUE
      MACOSX_BUNDLE_INFO_PLIST ${PLUG_RESOURCES_DIR}/${project_name}-macOS-Info.plist
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out"
    )
  endif()
endfunction()