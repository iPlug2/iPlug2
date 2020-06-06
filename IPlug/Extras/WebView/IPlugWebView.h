 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#include "IPlugPlatform.h"
#include "wdlstring.h"
#include <functional>

#if defined OS_MAC
  #define PLATFORM_VIEW NSView
  #define PLATFORM_RECT NSRect
  #define MAKERECT NSMakeRect
#elif defined OS_IOS
  #define PLATFORM_VIEW UIView
  #define PLATFORM_RECT CGRect
  #define MAKERECT CGRectMake
#elif defined OS_WIN
  #include <wrl.h>
  #include <wil/com.h>
  #include "WebView2.h"
#endif

BEGIN_IPLUG_NAMESPACE

using completionHandlerFunc = std::function<void(const char* result)>;

/** IWebView is a base interface for hosting a platform web view inside an IPlug plug-in's UI */
class IWebView
{
public:
  IWebView(bool opaque = true);
  virtual ~IWebView();
  
  void* OpenWebView(void* pParent, float x, float y, float w, float h, float scale = 1.);
  void CloseWebView();
  
  void LoadHTML(const char* html);
  void LoadURL(const char* url);
  void LoadFile(const char* fileName, const char* bundleID);
  void EvaluateJavaScript(const char* scriptStr, completionHandlerFunc func = nullptr);
  void EnableScroll(bool enable);
  void SetWebViewBounds(float x, float y, float w, float h, float scale = 1.);

  /** Called when the web view is ready to receive navigation instructions*/
  virtual void OnWebViewReady() {}
  
  /** Called after navigation instructions have been exectued and e.g. a page has loaded */
  virtual void OnWebContentLoaded() {}
  
  /** When a script in the web view posts a message, it will arrive as a json string here */
  virtual void OnMessageFromWebView(const char* json) {}

#if defined OS_WIN
  void SetWebViewPaths(const char* dllPath, const char* tmpPath) { mDLLPath.Set(dllPath); mTmpPath.Set(tmpPath); }
#endif
private:
  bool mOpaque = true;
#if defined OS_MAC || defined OS_IOS
  void* mWKWebView = nullptr;
  void* mWebConfig = nullptr;
  void* mScriptHandler = nullptr;
#elif defined OS_WIN
  wil::com_ptr<ICoreWebView2Controller> mWebViewCtrlr;
  wil::com_ptr<ICoreWebView2> mWebViewWnd;
  EventRegistrationToken mWebMessageReceivedToken;
  EventRegistrationToken mNavigationCompletedToken;
  WDL_String mDLLPath;
  WDL_String mTmpPath;
#endif
};

END_IPLUG_NAMESPACE
