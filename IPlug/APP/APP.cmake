cmake_minimum_required(VERSION 3.11)

add_library(iPlug2_APP INTERFACE)
set(sdk ${IPLUG2_DIR}/IPlug/APP)
set(_src
  ${sdk}/IPlugAPP.cpp 
  ${sdk}/IPlugAPP_dialog.cpp 
  ${sdk}/IPlugAPP_host.cpp 
  ${sdk}/IPlugAPP_main.cpp
  ${_DEPS}/IPlug/RTAudio/RtAudio.cpp
  ${_DEPS}/IPlug/RTMidi/RtMidi.cpp
)
set(_inc
  ${sdk} 
  ${_DEPS}/IPlug/RTAudio
  ${_DEPS}/IPlug/RTAudio/include
  ${_DEPS}/IPlug/RTMidi
  ${_DEPS}/IPlug/RTMidi/include
)
set(_def "APP_API" "IPLUG_EDITOR=1" "IPLUG_DSP=1" )

# Link Windows sound libraies if on Windows
if (WIN32)
  iplug2_target_add(iPlug2_APP INTERFACE
    DEFINE "__WINDOWS_DS__" "__WINDOWS_MM__" "__WINDOWS_ASIO__"
    SOURCE
      ${_DEPS}/IPlug/RTAudio/include/asio.cpp
      ${_DEPS}/IPlug/RTAudio/include/asiodrivers.cpp
      ${_DEPS}/IPlug/RTAudio/include/asiolist.cpp
      ${_DEPS}/IPlug/RTAudio/include/iasiothiscallresolver.cpp
      ${_DEPS}/IPlug/RTMidi/rtmidi_c.cpp
    LINK dsound.lib winmm.lib
  )

elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
  # Some source files here combine C++ and Objective-C, so we tell clang how to compile them
  set_property(SOURCE ${_src} PROPERTY LANGUAGE "OBJCXX")
  iplug2_target_add(iPlug2_APP INTERFACE
    DEFINE "__MACOSX_CORE__" "SWELL_COMPILED"
    LINK "-framework AppKit" "-framework CoreMIDI" "-framework CoreAudio"
    SOURCE 
      "${WDL_DIR}/swell/swell-appstub.mm"
      "${WDL_DIR}/swell/swellappmain.mm"
      "${WDL_DIR}/swell/swell-ini.cpp"
      "${WDL_DIR}/swell/swell-dlg.mm"
      "${WDL_DIR}/swell/swell-kb.mm"
      "${WDL_DIR}/swell/swell-miscdlg.mm"
      "${WDL_DIR}/swell/swell-menu.mm"
      "${WDL_DIR}/swell/swell-wnd.mm"
      "${WDL_DIR}/swell/swell.cpp"
      "${WDL_DIR}/swell/swell-misc.mm"
      "${WDL_DIR}/swell/swell-gdi.mm"
  )
else()
  message(FATAL_ERROR "APP not supported on platform ${CMAKE_SYSTEM_NAME}")
endif()

iplug2_add_interface(iPlug2_APP INCLUDE ${_inc} DEFINE ${_def} SOURCE ${_src} LINK iPlug2_Core)

function(iplug2_configure_app target)
  if (WIN32)
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND "${CMAKE_BINARY_DIR}/postbuild-win.bat"
      ARGS "\"$<TARGET_FILE:${target}>\"" "\".exe\""
    )
    
  elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    # Set the Info.plist file and add required resources
    set(_res 
      "${CMAKE_SOURCE_DIR}/resources/${IPLUG2_APP_NAME}.icns"
      "${CMAKE_SOURCE_DIR}/resources/${IPLUG2_APP_NAME}-macOS-MainMenu.xib")
    source_group("Resources" FILES ${res_files})
    iplug2_target_add(${target} PUBLIC RESOURCE ${_res})
    set_target_properties(${target} PROPERTIES 
      MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/resources/macOS-App-Info.plist.in")
  endif()
endfunction()