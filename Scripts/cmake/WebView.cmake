#  ==============================================================================
#
#  This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
#
#  See LICENSE.txt for  more info.
#
#  ==============================================================================

# WebView configuration for iPlug2
# This module provides WebViewEditorDelegate for plugins that use WebView UI instead of IGraphics

include(${CMAKE_CURRENT_LIST_DIR}/IPlug.cmake)

if(NOT TARGET iPlug2::WebView)
  add_library(iPlug2::WebView INTERFACE IMPORTED)

  set(WEBVIEW_DIR ${IPLUG2_DIR}/IPlug/Extras/WebView)

  # Platform-specific WebView sources
  # Note: IPlugWebView.cpp is #included by IPlugWebView_mac.mm (unity build style)
  # IPlugWebView_mac.mm also #includes IPlugWKWebView*.mm files
  if(APPLE)
    # Files that require ARC
    set(WEBVIEW_SRC_ARC
      ${WEBVIEW_DIR}/IPlugWebView_mac.mm
    )
    # Files that must NOT use ARC
    set(WEBVIEW_SRC_NO_ARC
      ${WEBVIEW_DIR}/IPlugWebViewEditorDelegate.mm
    )
    set(WEBVIEW_SRC ${WEBVIEW_SRC_ARC} ${WEBVIEW_SRC_NO_ARC})

    # Set ARC compile option for files that need it
    set_source_files_properties(${WEBVIEW_SRC_ARC} PROPERTIES
      COMPILE_FLAGS "-fobjc-arc"
    )
  elseif(WIN32)
    set(WEBVIEW_SRC
      ${WEBVIEW_DIR}/IPlugWebViewEditorDelegate.cpp
      ${WEBVIEW_DIR}/IPlugWebView_win.cpp
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
  endif()

  target_sources(iPlug2::WebView INTERFACE ${WEBVIEW_SRC})

  target_include_directories(iPlug2::WebView INTERFACE
    ${WEBVIEW_DIR}
    ${DEPS_DIR}/Extras/nlohmann
  )

  target_compile_definitions(iPlug2::WebView INTERFACE
    WEBVIEW_EDITOR_DELEGATE
    NO_IGRAPHICS
  )

  if(APPLE)
    target_link_libraries(iPlug2::WebView INTERFACE
      "-framework WebKit"
      iPlug2::IPlug
    )
    # WebView uses std::filesystem::path which requires macOS 10.15+
    # Set compile definition so code can check, and require 10.15 deployment target
    target_compile_definitions(iPlug2::WebView INTERFACE
      IPLUG_WEBVIEW_REQUIRES_10_15
    )
  elseif(WIN32)
    target_include_directories(iPlug2::WebView INTERFACE
      ${wil_SOURCE_DIR}/include
      ${WEBVIEW2_INCLUDE_DIR}
    )
    target_link_libraries(iPlug2::WebView INTERFACE
      iPlug2::IPlug
      "${WEBVIEW2_LIB_DIR}/WebView2LoaderStatic.lib"
    )
  endif()
endif()
