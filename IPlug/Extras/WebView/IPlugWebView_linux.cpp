/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Stub IWebViewImpl for Linux — WebView is not yet implemented on Linux.
// All methods are no-ops so that code linking against iPlug2::Extras::IWebViewControl
// compiles and links; attempting to use a WebView at runtime will silently do nothing.

#include "IPlugWebView.h"

BEGIN_IPLUG_NAMESPACE

class IWebViewImpl
{
public:
  IWebViewImpl(IWebView*) {}
  ~IWebViewImpl() {}

  void* OpenWebView(void*, float, float, float, float, float) { return nullptr; }
  void CloseWebView() {}
  void HideWebView(bool) {}
  void LoadHTML(const char*) {}
  void LoadURL(const char*) {}
  void LoadFile(const char*, const char*) {}
  void ReloadPageContent() {}
  void EvaluateJavaScript(const char*, IWebView::completionHandlerFunc) {}
  void EnableScroll(bool) {}
  void EnableInteraction(bool) {}
  void SetWebViewBounds(float, float, float, float, float) {}
  void GetLocalDownloadPathForFile(const char*, WDL_String&) {}
  void GetWebRoot(WDL_String&) {}
};

END_IPLUG_NAMESPACE

// Pull in the platform-independent IWebView glue code
#include "IPlugWebView.cpp"
