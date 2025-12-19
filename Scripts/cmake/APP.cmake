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

  # RTAudio and RTMidi directories
  set(RTAUDIO_DIR ${IPLUG_DEPS_DIR}/RTAudio)
  set(RTMIDI_DIR ${IPLUG_DEPS_DIR}/RTMidi)
  
  set(IPLUG2_APP_SRC
    ${IPLUG2_DIR}/IPlug/APP/IPlugAPP.cpp
    ${IPLUG2_DIR}/IPlug/APP/IPlugAPP_dialog.cpp
    ${IPLUG2_DIR}/IPlug/APP/IPlugAPP_host.cpp
    ${IPLUG2_DIR}/IPlug/APP/IPlugAPP_main.cpp
    ${RTAUDIO_DIR}/RtAudio.cpp
    ${RTMIDI_DIR}/RtMidi.cpp
    CACHE INTERNAL "APP source files"
  )

  target_sources(iPlug2::APP INTERFACE ${IPLUG2_APP_SRC})
  
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
    # Note: OBJCXX language is set on actual target sources in iplug_configure_app()
    # because set_source_files_properties on INTERFACE library sources doesn't propagate
    target_include_directories(iPlug2::APP INTERFACE ${SWELL_DIR})

    set(IPLUG2_SWELL_SRC
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
      CACHE INTERNAL "SWELL source files for APP"
    )

    target_sources(iPlug2::APP INTERFACE ${IPLUG2_SWELL_SRC})

    # Note: swell warning suppression is set on actual target sources in iplug_configure_app()
    # because set_source_files_properties on INTERFACE library sources doesn't propagate

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

  # On Apple platforms, set language properties on the actual target sources
  # (set_source_files_properties on INTERFACE library sources doesn't propagate)
  if(APPLE)
    set_source_files_properties(${IPLUG2_APP_SRC}
      TARGET_DIRECTORY ${target}
      PROPERTIES LANGUAGE OBJCXX
    )
    # swell sources need Cocoa imported before swell-internal.h.
    # Force-include the prefix pch which imports Cocoa (with #ifdef __OBJC__ guard)
    set_source_files_properties(${IPLUG2_SWELL_SRC}
      TARGET_DIRECTORY ${target}
      PROPERTIES
      LANGUAGE OBJCXX
      COMPILE_FLAGS "-Wno-deprecated-declarations -include ${IPLUG_DIR}/IPlugOBJCPrefix.pch"
    )
  endif()

  if(WIN32)
    set(APP_OUTPUT_DIR "${CMAKE_BINARY_DIR}/out")
    set_target_properties(${target} PROPERTIES
      OUTPUT_NAME "${project_name}"
      RUNTIME_OUTPUT_DIRECTORY "${APP_OUTPUT_DIR}"
      RUNTIME_OUTPUT_DIRECTORY_DEBUG "${APP_OUTPUT_DIR}"
      RUNTIME_OUTPUT_DIRECTORY_RELEASE "${APP_OUTPUT_DIR}"
      RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${APP_OUTPUT_DIR}"
      RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${APP_OUTPUT_DIR}"
      WIN32_EXECUTABLE TRUE  # Use WinMain entry point
    )
  elseif(APPLE)
    set_target_properties(${target} PROPERTIES
      MACOSX_BUNDLE TRUE
      MACOSX_BUNDLE_INFO_PLIST ${PLUG_RESOURCES_DIR}/${project_name}-macOS-Info.plist
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out"
      OUTPUT_NAME "${project_name}"
      # Skip code signing during build - sign manually later if needed
      XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
    )

    # Compile XIB to NIB and add to bundle Resources
    set(MAIN_MENU_XIB ${PLUG_RESOURCES_DIR}/${project_name}-macOS-MainMenu.xib)
    if(EXISTS ${MAIN_MENU_XIB})
      set(MAIN_MENU_NIB ${CMAKE_CURRENT_BINARY_DIR}/${project_name}-macOS-MainMenu.nib)
      add_custom_command(
        OUTPUT ${MAIN_MENU_NIB}
        COMMAND ibtool --compile ${MAIN_MENU_NIB} ${MAIN_MENU_XIB}
        DEPENDS ${MAIN_MENU_XIB}
        COMMENT "Compiling ${project_name}-macOS-MainMenu.xib"
      )
      target_sources(${target} PRIVATE ${MAIN_MENU_NIB})
      set_source_files_properties(${MAIN_MENU_NIB} PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
      )
    endif()

    # Add icon to bundle Resources
    set(APP_ICON ${PLUG_RESOURCES_DIR}/${project_name}.icns)
    if(EXISTS ${APP_ICON})
      target_sources(${target} PRIVATE ${APP_ICON})
      set_source_files_properties(${APP_ICON} PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
      )
    endif()

    # Create PkgInfo for non-Xcode generators (Xcode creates it automatically)
    if(NOT XCODE)
      set(PKGINFO_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/write_pkginfo_${target}.cmake")
      file(WRITE ${PKGINFO_SCRIPT} "file(WRITE \"\${PKGINFO_PATH}\" \"APPL????\")")
      add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -DPKGINFO_PATH="$<TARGET_BUNDLE_DIR:${target}>/Contents/PkgInfo"
          -P "${PKGINFO_SCRIPT}"
        COMMENT "Creating PkgInfo for ${project_name}.app"
      )
    endif()
  endif()
endfunction()