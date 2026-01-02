#  ==============================================================================
#  
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# Configuration for IPlug Core

# C++ standard - iPlug2 requires C++17
if(NOT DEFINED IPLUG2_CXX_STANDARD)
  set(IPLUG2_CXX_STANDARD 17 CACHE STRING "C++ standard for iPlug2")
endif()

# Option to disable deprecation warnings (useful for CI)
option(IPLUG2_DISABLE_DEPRECATION_WARNINGS "Disable deprecation warnings" ON)

# Output directory for built plugins (allows override for FetchContent usage)
set(IPLUG2_OUTPUT_DIR "${CMAKE_BINARY_DIR}/out" CACHE PATH "Output directory for built plugins")

# Generated files directory (PkgInfo, etc.) - scoped to iPlug2's binary dir
set(IPLUG2_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/iplug2_generated" CACHE INTERNAL "iPlug2 generated files directory")

if(NOT TARGET iPlug2::IPlug)
  add_library(iPlug2::IPlug INTERFACE IMPORTED)

  set(IPLUG_DIR ${IPLUG2_DIR}/IPlug)
  set(WDL_DIR ${IPLUG2_DIR}/WDL)
  set(DEPS_DIR ${IPLUG2_DIR}/Dependencies)
  set(IPLUG_DEPS_DIR ${DEPS_DIR}/IPlug)

  set(IPLUG_SRC
    ${IPLUG_DIR}/IPlugAPIBase.h
    ${IPLUG_DIR}/IPlugAPIBase.cpp
    ${IPLUG_DIR}/IPlugConstants.h
    ${IPLUG_DIR}/IPlugEditorDelegate.h
    ${IPLUG_DIR}/IPlugLogger.h
    ${IPLUG_DIR}/IPlugMidi.h
    ${IPLUG_DIR}/IPlugParameter.h
    ${IPLUG_DIR}/IPlugParameter.cpp
    ${IPLUG_DIR}/IPlugPaths.h
    ${IPLUG_DIR}/IPlugPaths.cpp
    ${IPLUG_DIR}/IPlugPlatform.h
    ${IPLUG_DIR}/IPlugPluginBase.h
    ${IPLUG_DIR}/IPlugPluginBase.cpp
    ${IPLUG_DIR}/IPlugProcessor.h
    ${IPLUG_DIR}/IPlugProcessor.cpp
    ${IPLUG_DIR}/IPlugQueue.h
    ${IPLUG_DIR}/IPlugStructs.h
    ${IPLUG_DIR}/IPlugTimer.h
    ${IPLUG_DIR}/IPlugTimer.cpp
    ${IPLUG_DIR}/IPlugUtilities.h
  )

  if(APPLE)
    list(APPEND IPLUG_SRC ${IPLUG_DIR}/IPlugPaths.mm)
  endif()
  
  target_sources(iPlug2::IPlug INTERFACE ${IPLUG_SRC})
  
  target_include_directories(iPlug2::IPlug INTERFACE
    ${IPLUG_DIR}
    ${WDL_DIR}
    ${WDL_DIR}/libpng
    ${WDL_DIR}/zlib
    ${IPLUG_DIR}/Extras
  )
  
  target_compile_definitions(iPlug2::IPlug INTERFACE
    NOMINMAX  # Prevent min/max macros from Windows.h and SWELL
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Debug>:_DEBUG>
  )
  
  if(MSVC)
    target_compile_definitions(iPlug2::IPlug INTERFACE
      _CRT_SECURE_NO_WARNINGS
      _CRT_SECURE_NO_DEPRECATE
      _CRT_NONSTDC_NO_DEPRECATE
      _MBCS
    )
    target_compile_options(iPlug2::IPlug INTERFACE
      /wd4250 /wd4018 /wd4267 /wd4068
      /MT$<$<CONFIG:Debug>:d>
    )
  endif()

  # Suppress deprecation warnings if requested (useful for CI)
  if(IPLUG2_DISABLE_DEPRECATION_WARNINGS)
    if(MSVC)
      target_compile_options(iPlug2::IPlug INTERFACE /wd4996)
      target_compile_definitions(iPlug2::IPlug INTERFACE
        _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
      )
    else()
      # Use generator expression to only apply to C/C++/ObjC (not Swift)
      target_compile_options(iPlug2::IPlug INTERFACE
        $<$<COMPILE_LANGUAGE:C,CXX,OBJC,OBJCXX>:-Wno-deprecated-declarations>
      )
    endif()
  endif()
  
  if(WIN32)
    target_link_libraries(iPlug2::IPlug INTERFACE 
      Shlwapi.lib
      comctl32.lib
      wininet.lib
    )
  elseif(APPLE)
    target_link_libraries(iPlug2::IPlug INTERFACE
      "-framework CoreData"
      "-framework CoreFoundation"
      "-framework CoreServices"
      "-framework Foundation"
    )
  elseif(UNIX AND NOT APPLE)
    message("Error - Linux not yet supported")
  endif()

  # Generate PkgInfo file for macOS bundles (used by VST2, CLAP, etc.)
  # This file is created once at configure time and copied to each bundle at build time
  if(APPLE)
    file(MAKE_DIRECTORY "${IPLUG2_GENERATED_DIR}")
    set(IPLUG2_PKGINFO_FILE "${IPLUG2_GENERATED_DIR}/PkgInfo" CACHE INTERNAL "Path to PkgInfo file for macOS bundles")
    file(WRITE "${IPLUG2_PKGINFO_FILE}" "BNDL????")
  endif()
endif()

# =============================================================================
# IPlug Extras - Optional modules
# =============================================================================

# OSC (Open Sound Control) support
if(NOT TARGET iPlug2::Extras::OSC)
  add_library(iPlug2::Extras::OSC INTERFACE IMPORTED)

  target_sources(iPlug2::Extras::OSC INTERFACE
    ${IPLUG_DIR}/Extras/OSC/IPlugOSC.cpp
    ${IPLUG_DIR}/Extras/OSC/IPlugOSC_internal.cpp
    ${IPLUG_DIR}/Extras/OSC/IPlugOSC_msg.cpp
    ${WDL_DIR}/jnetlib/asyncdns.cpp
    ${WDL_DIR}/jnetlib/connection.cpp
    ${WDL_DIR}/jnetlib/listen.cpp
    ${WDL_DIR}/jnetlib/util.cpp
  )

  target_include_directories(iPlug2::Extras::OSC INTERFACE
    ${IPLUG_DIR}/Extras/OSC
  )

  target_link_libraries(iPlug2::Extras::OSC INTERFACE iPlug2::IPlug)

  if(WIN32)
    target_link_libraries(iPlug2::Extras::OSC INTERFACE ws2_32.lib)
  endif()
endif()

# Synth utilities (MidiSynth, VoiceAllocator)
if(NOT TARGET iPlug2::Extras::Synth)
  add_library(iPlug2::Extras::Synth INTERFACE IMPORTED)

  target_sources(iPlug2::Extras::Synth INTERFACE
    ${IPLUG_DIR}/Extras/Synth/MidiSynth.cpp
    ${IPLUG_DIR}/Extras/Synth/VoiceAllocator.cpp
  )

  target_include_directories(iPlug2::Extras::Synth INTERFACE
    ${IPLUG_DIR}/Extras/Synth
  )

  target_link_libraries(iPlug2::Extras::Synth INTERFACE iPlug2::IPlug)
endif()

# HIIR oversampling/downsampling
if(NOT TARGET iPlug2::Extras::HIIR)
  add_library(iPlug2::Extras::HIIR INTERFACE IMPORTED)

  target_sources(iPlug2::Extras::HIIR INTERFACE
    ${IPLUG_DIR}/Extras/HIIR/PolyphaseIIR2Designer.cpp
  )

  target_include_directories(iPlug2::Extras::HIIR INTERFACE
    ${IPLUG_DIR}/Extras/HIIR
  )

  target_link_libraries(iPlug2::Extras::HIIR INTERFACE iPlug2::IPlug)
endif()

# IWebViewControl support for IGraphics plugins (minimal - no EditorDelegate)
# Use this when embedding IWebViewControl in an IGraphics UI
# Note: IPlugWebView.cpp and IPlugWK*.mm are #included by platform-specific files (unity build)

# Object library for macOS WebView (requires ARC)
if(NOT TARGET iPlug2_Extras_IWebViewControl_obj)
  if(APPLE)
    add_library(iPlug2_Extras_IWebViewControl_obj OBJECT
      ${IPLUG_DIR}/Extras/WebView/IPlugWebView_mac.mm
    )

    target_include_directories(iPlug2_Extras_IWebViewControl_obj PRIVATE
      ${IPLUG_DIR}
      ${IPLUG_DIR}/Extras/WebView
      ${WDL_DIR}
    )

    target_compile_options(iPlug2_Extras_IWebViewControl_obj PRIVATE -fobjc-arc)

    set_target_properties(iPlug2_Extras_IWebViewControl_obj PROPERTIES
      POSITION_INDEPENDENT_CODE ON
      CXX_STANDARD ${IPLUG2_CXX_STANDARD}
      CXX_STANDARD_REQUIRED ON
    )
  endif()
endif()

if(NOT TARGET iPlug2::Extras::IWebViewControl)
  add_library(iPlug2::Extras::IWebViewControl INTERFACE IMPORTED)

  if(WIN32)
    target_sources(iPlug2::Extras::IWebViewControl INTERFACE
      ${IPLUG_DIR}/Extras/WebView/IPlugWebView_win.cpp
    )

    # Fetch WIL (Windows Implementation Libraries) - header-only
    include(FetchContent)
    FetchContent_Declare(
      wil
      GIT_REPOSITORY https://github.com/microsoft/wil.git
      GIT_TAG v1.0.240803.1
      GIT_SHALLOW TRUE
    )
    # Disable WIL tests (they require Detours and other dependencies)
    set(WIL_BUILD_PACKAGING OFF CACHE BOOL "" FORCE)
    set(WIL_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(wil)

    # Fetch WebView2 NuGet package
    set(WEBVIEW2_VERSION "1.0.2903.40")
    set(WEBVIEW2_DIR "${CMAKE_BINARY_DIR}/_deps/webview2")
    set(WEBVIEW2_NUGET_URL "https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/${WEBVIEW2_VERSION}")

    if(NOT EXISTS "${WEBVIEW2_DIR}/build/native/include/WebView2.h")
      message(STATUS "Downloading WebView2 SDK ${WEBVIEW2_VERSION}...")
      file(DOWNLOAD
        "${WEBVIEW2_NUGET_URL}"
        "${WEBVIEW2_DIR}/webview2.zip"
        SHOW_PROGRESS
        STATUS DOWNLOAD_STATUS
      )
      list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
      if(NOT STATUS_CODE EQUAL 0)
        message(FATAL_ERROR "Failed to download WebView2 SDK")
      endif()

      file(ARCHIVE_EXTRACT
        INPUT "${WEBVIEW2_DIR}/webview2.zip"
        DESTINATION "${WEBVIEW2_DIR}"
      )
      file(REMOVE "${WEBVIEW2_DIR}/webview2.zip")
    endif()

    set(WEBVIEW2_INCLUDE_DIR "${WEBVIEW2_DIR}/build/native/include")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(WEBVIEW2_LIB_DIR "${WEBVIEW2_DIR}/build/native/x64")
    else()
      set(WEBVIEW2_LIB_DIR "${WEBVIEW2_DIR}/build/native/x86")
    endif()

    target_include_directories(iPlug2::Extras::IWebViewControl INTERFACE
      ${wil_SOURCE_DIR}/include
      ${WEBVIEW2_INCLUDE_DIR}
    )
    target_link_libraries(iPlug2::Extras::IWebViewControl INTERFACE
      "${WEBVIEW2_LIB_DIR}/WebView2LoaderStatic.lib"
    )
  elseif(APPLE)
    target_sources(iPlug2::Extras::IWebViewControl INTERFACE
      $<TARGET_OBJECTS:iPlug2_Extras_IWebViewControl_obj>
    )
  endif()

  target_include_directories(iPlug2::Extras::IWebViewControl INTERFACE
    ${IPLUG_DIR}/Extras/WebView
  )

  if(APPLE)
    target_link_libraries(iPlug2::Extras::IWebViewControl INTERFACE
      "-framework WebKit"
    )
  endif()

  target_link_libraries(iPlug2::Extras::IWebViewControl INTERFACE iPlug2::IPlug)
endif()