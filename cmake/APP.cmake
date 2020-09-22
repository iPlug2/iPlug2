cmake_minimum_required(VERSION 3.11)

add_library(iPlug2_APP INTERFACE)
set(sdk ${IPLUG2_DIR}/IPlug/APP)
set(_src
  ${sdk}/IPlugAPP.cpp 
  ${sdk}/IPlugAPP_dialog.cpp 
  ${sdk}/IPlugAPP_host.cpp 
  ${sdk}/IPlugAPP_main.cpp
  ${IPLUG_DEPS}/RTAudio/RtAudio.cpp
  ${IPLUG_DEPS}/RTMidi/RtMidi.cpp
)
set(_inc
  ${sdk} 
  ${IPLUG_DEPS}/RTAudio
  ${IPLUG_DEPS}/RTAudio/include
  ${IPLUG_DEPS}/RTMidi
  ${IPLUG_DEPS}/RTMidi/include
)
set(_def "APP_API" "IPLUG_EDITOR=1" "IPLUG_DSP=1" )

# Link Windows sound libraies if on Windows
if (WIN32)
  iplug2_target_add(iPlug2_APP INTERFACE
    DEFINE "__WINDOWS_DS__" "__WINDOWS_MM__" "__WINDOWS_ASIO__"
    SOURCE
      ${IPLUG_DEPS}/RTAudio/include/asio.cpp
      ${IPLUG_DEPS}/RTAudio/include/asiodrivers.cpp
      ${IPLUG_DEPS}/RTAudio/include/asiolist.cpp
      ${IPLUG_DEPS}/RTAudio/include/iasiothiscallresolver.cpp
      ${IPLUG_DEPS}/RTMidi/rtmidi_c.cpp
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

elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
  pkg_check_modules(Glib_20 REQUIRED IMPORTED_TARGET "glib-2.0")
  pkg_check_modules(Gtk_30 REQUIRED IMPORTED_TARGET "gtk+-3.0")
  pkg_check_modules(Gdk_30 REQUIRED IMPORTED_TARGET "gdk-3.0")
  pkg_check_modules(Alsa IMPORTED_TARGET "alsa")
  pkg_check_modules(Jack IMPORTED_TARGET "jack")
  pkg_check_modules(PulseAudio IMPORTED_TARGET "libpulse")
  pkg_check_modules(PulseAudioSimple IMPORTED_TARGET "libpulse-simple")

  set(IPLUG_APP_ALSA 1 CACHE BOOL "Use ALSA on Linux")
  set(IPLUG_APP_JACK 1 CACHE BOOL "Use JACK on Linux")
  set(IPLUG_APP_PULSE 1 CACHE BOOL "Use Pulse Audio on Linux")

  # Build and link Swell properly on Linux. This uses GTK+ 3.0 and X11
  set(swell_src
    swell.h
    swell.cpp
    swell-appstub-generic.cpp
    swell-dlg-generic.cpp
    swell-gdi-generic.cpp
    swell-gdi-lice.cpp
    swell-ini.cpp
    swell-kb-generic.cpp
    swell-menu-generic.cpp
    swell-miscdlg-generic.cpp
    swell-misc-generic.cpp
    swell-wnd-generic.cpp
    swell-generic-gdk.cpp
  )
  list(TRANSFORM swell_src PREPEND "${WDL_DIR}/swell/")

  iplug2_target_add(iPlug2_APP INTERFACE
    DEFINE "SWELL_COMPILED" "SWELL_SUPPORT_GTK" "SWELL_TARGET_GDK=3" SWELL_LICE_GDI SWELL_FREETYPE
    INCLUDE 
      "${WDL_DIR}/swell/"
      "${WDL_DIR}/lice/"
    LINK 
      LICE_Core LICE_PNG PkgConfig::Gtk_30 PkgConfig::Gdk_30 PkgConfig::Glib_20 "X11" "Xi"
    SOURCE
      ${swell_src}
      ${CMAKE_SOURCE_DIR}/resources/main.rc_mac_dlg
      ${CMAKE_SOURCE_DIR}/resources/main.rc_mac_menu
  )

  # RtAudio
  if (IPLUG_APP_ALSA)
    iplug2_target_add(iPlug2_APP INTERFACE
      DEFINE "__LINUX_ALSA__" LINK PkgConfig::Alsa)
  endif()
  if (IPLUG_APP_JACK)
    iplug2_target_add(iPlug2_APP INTERFACE
      DEFINE "__UNIX_JACK__" LINK PkgConfig::Jack)
  endif()
  if (IPLUG_APP_PULSE)
    iplug2_target_add(iPlug2_APP INTERFACE
      DEFINE "__LINUX_PULSE__" LINK PkgConfig::PulseAudio PkgConfig::PulseAudioSimple)
  endif()
  
else()
  message(FATAL_ERROR "APP not supported on platform ${CMAKE_SYSTEM_NAME}")
endif()

iplug2_target_add(iPlug2_APP INTERFACE INCLUDE ${_inc} DEFINE ${_def} SOURCE ${_src} LINK iPlug2_Core)

function(iplug2_configure_app target)
  set(res_dir "${CMAKE_BINARY_DIR}/${IPLUG_APP_NAME}-app/resources")

  if (WIN32)
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND "${CMAKE_BINARY_DIR}/postbuild-win.bat"
      ARGS "\"$<TARGET_FILE:${target}>\"" "\".exe\""
    )
    iplug_target_bundle_resources(${target} "${res_dir}")
    
  elseif (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    # Set the Info.plist file and add required resources
    set(_res 
      "${CMAKE_SOURCE_DIR}/resources/${IPLUG_APP_NAME}.icns"
      "${CMAKE_SOURCE_DIR}/resources/${IPLUG_APP_NAME}-macOS-MainMenu.xib")
    source_group("Resources" FILES ${res_files})
    iplug2_target_add(${target} PUBLIC RESOURCE ${_res})
    set_target_properties(${target} PROPERTIES 
      MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/resources/macOS-App-Info.plist.in")

  elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set_target_properties(${target} PROPERTIES
      OUTPUT_NAME "${IPLUG_APP_NAME}"
      RUNTIME_OUTPUT_DIRECTORY "${IPLUG_APP_NAME}-app"
    )
    iplug2_target_bundle_resources(${target} "${res_dir}")

  endif()
endfunction()
