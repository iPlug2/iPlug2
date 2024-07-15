#  ==============================================================================
#  
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

cmake_minimum_required(VERSION 3.11)

# Create the iPlug2_APP interface library
add_library(iPlug2_APP INTERFACE)

# Set common variables
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
set(_def "APP_API" "IPLUG_EDITOR=1" "IPLUG_DSP=1")

# Platform-specific configurations
if(WIN32)
  iplug_target_add(iPlug2_APP INTERFACE
    DEFINE "__WINDOWS_DS__" "__WINDOWS_MM__" "__WINDOWS_ASIO__"
    SOURCE
      ${IPLUG_DEPS}/RTAudio/include/asio.cpp
      ${IPLUG_DEPS}/RTAudio/include/asiodrivers.cpp
      ${IPLUG_DEPS}/RTAudio/include/asiolist.cpp
      ${IPLUG_DEPS}/RTAudio/include/iasiothiscallresolver.cpp
      ${IPLUG_DEPS}/RTMidi/rtmidi_c.cpp
    LINK dsound.lib winmm.lib
  )
elseif(APPLE)
  # Set language for files combining C++ and Objective-C
  set_property(SOURCE ${_src} PROPERTY LANGUAGE "OBJCXX")
  iplug_target_add(iPlug2_APP INTERFACE
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
elseif(UNIX AND NOT APPLE)
  # Linux-specific configuration
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(DEPS REQUIRED IMPORTED_TARGET
    glib-2.0
    gtk+-3.0
    gdk-3.0
  )
  pkg_check_modules(ALSA IMPORTED_TARGET "alsa")
  pkg_check_modules(JACK IMPORTED_TARGET "jack")
  pkg_check_modules(PULSEAUDIO IMPORTED_TARGET "libpulse" "libpulse-simple")

  # Audio backend options
  option(IPLUG_APP_ALSA "Use ALSA on Linux" ON)
  option(IPLUG_APP_JACK "Use JACK on Linux" ON)
  option(IPLUG_APP_PULSE "Use Pulse Audio on Linux" ON)

  # SWELL configuration for Linux
  set(swell_src
    swell.h swell.cpp swell-appstub-generic.cpp swell-dlg-generic.cpp
    swell-gdi-generic.cpp swell-gdi-lice.cpp swell-ini.cpp swell-kb-generic.cpp
    swell-menu-generic.cpp swell-miscdlg-generic.cpp swell-misc-generic.cpp
    swell-wnd-generic.cpp swell-generic-gdk.cpp
  )
  list(TRANSFORM swell_src PREPEND "${WDL_DIR}/swell/")

  iplug_target_add(iPlug2_APP INTERFACE
    DEFINE 
      SWELL_COMPILED SWELL_SUPPORT_GTK SWELL_TARGET_GDK=3 SWELL_LICE_GDI 
      SWELL_FREETYPE _FILE_OFFSET_BITS=64 WDL_ALLOW_UNSIGNED_DEFAULT_CHAR
    INCLUDE 
      "${WDL_DIR}/swell/"
      "${WDL_DIR}/lice/"
    LINK 
      LICE_Core LICE_PNG LICE_ZLIB 
      PkgConfig::DEPS "X11" "Xi"
    SOURCE
      ${swell_src}
      ${PLUG_RESOURCES_DIR}/main.rc_mac_dlg
      ${PLUG_RESOURCES_DIR}/main.rc_mac_menu
  )

  # RtAudio configuration
  if(IPLUG_APP_ALSA)
    iplug_target_add(iPlug2_APP INTERFACE
      DEFINE "__LINUX_ALSA__" LINK PkgConfig::ALSA)
  endif()
  if(IPLUG_APP_JACK)
    iplug_target_add(iPlug2_APP INTERFACE
      DEFINE "__UNIX_JACK__" LINK PkgConfig::JACK)
  endif()
  if(IPLUG_APP_PULSE)
    iplug_target_add(iPlug2_APP INTERFACE
      DEFINE "__LINUX_PULSE__" LINK PkgConfig::PULSEAUDIO)
  endif()
else()
  message(FATAL_ERROR "APP not supported on platform ${CMAKE_SYSTEM_NAME}")
endif()

# Configure iPlug2_APP
iplug_target_add(iPlug2_APP INTERFACE 
  INCLUDE ${_inc} 
  DEFINE ${_def} 
  SOURCE ${_src} 
  LINK iPlug2_Core
)
iplug_source_tree(iPlug2_APP)

# Function to configure APP targets
function(iplug_configure_app target)
  iplug_target_add(${target} PUBLIC LINK iPlug2_APP)

  set(out_dir "${CMAKE_BINARY_DIR}/out")

  if(WIN32)
    set(res_dir "${CMAKE_BINARY_DIR}/${PLUG_NAME}-app/resources")
    
    set_target_properties(${target} PROPERTIES
      OUTPUT_NAME "${PLUG_NAME}"
      RUNTIME_OUTPUT_DIRECTORY "${PLUG_NAME}-app"
    )
    
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND "${CMAKE_BINARY_DIR}/postbuild-win.bat"
      ARGS "\"$<TARGET_FILE:${target}>\"" "\".exe\""
    )
  elseif(APPLE)
    set(app_out_dir "${out_dir}/${PLUG_NAME}.app")
    set(res_dir "${app_out_dir}/Contents/Resources")

    set_target_properties(${target} PROPERTIES
      MACOSX_BUNDLE TRUE
      MACOSX_BUNDLE_INFO_PLIST ${PLUG_RESOURCES_DIR}/${PLUG_NAME}-macOS-Info.plist
      RUNTIME_OUTPUT_DIRECTORY "${out_dir}"
      XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym"
      XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES"
    )
    
    if(CMAKE_GENERATOR STREQUAL "Xcode")
      set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${out_dir}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${out_dir}"
      )
    endif()

    target_compile_options(${target} PRIVATE -g)

    set(_res 
      "${PLUG_RESOURCES_DIR}/${PLUG_NAME}.icns"
      "${PLUG_RESOURCES_DIR}/${PLUG_NAME}-macOS-MainMenu.xib"
    )
    source_group("Resources" FILES ${_res})
    iplug_target_add(${target} PUBLIC SOURCE ${_res} RESOURCE ${_res})

    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "${out_dir}"
      COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_BUNDLE_DIR:${target}>" "${app_out_dir}"
      COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_BUNDLE_DIR:${target}>.dSYM" "${out_dir}/${PLUG_NAME}.app.dSYM" || ${CMAKE_COMMAND} -E echo "No .dSYM found, possibly a non-Xcode generator"
      COMMAND ${CMAKE_COMMAND} -E echo "Attempting to generate dSYM file..."
      COMMAND dsymutil "$<TARGET_BUNDLE_DIR:${target}>/Contents/MacOS/$<TARGET_FILE_NAME:${target}>" -o "${out_dir}/${PLUG_NAME}.app.dSYM" || ${CMAKE_COMMAND} -E echo "Failed to generate dSYM, continuing build..."
      COMMAND ${CMAKE_COMMAND} -E echo "App bundle and dSYM processing completed"
    )

  endif()

  iplug_target_bundle_resources(${target} "${res_dir}")
endfunction()
