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
      ${WEBVIEW_DIR}/IPlugWebView.cpp
      ${WEBVIEW_DIR}/IPlugWebViewEditorDelegate.cpp
      ${WEBVIEW_DIR}/IPlugWebView_win.cpp
    )
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
  elseif(WIN32)
    target_link_libraries(iPlug2::WebView INTERFACE
      iPlug2::IPlug
    )
  endif()
endif()
