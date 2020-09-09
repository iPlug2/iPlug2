 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#include <wdlstring.h>

#if PLATFORM_MAC
  #define PLATFORM_VIEW NSView
  #define PLATFORM_RECT NSRect
  #define MAKERECT      NSMakeRect
#elif PLATFORM_IOS
  #define PLATFORM_VIEW UIView
  #define PLATFORM_RECT CGRect
  #define MAKERECT      CGRectMake
#elif PLATFORM_WINDOWS
  #include <wrl.h>
  #include <wil/com.h>
  #include <WebView2.h>
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
  
  /** Load an HTML string into the webview */
  void LoadHTML(const char* html);
  
  /** Instruct the webview to load an external URL */
  void LoadURL(const char* url);
  
  /** Load a file on disk into the web view
   * @param fileName On windows this should be an absolute path to the file you want to load. On macOS/iOS it can just be the file name if the file is packaged into a subfolder "web" of the bundle resources
   * @param bundleID The NSBundleID of the macOS/iOS bundle, not required on Windows */
  void LoadFile(const char* fileName, const char* bundleID = "");
  
  /** Runs some JavaScript in the webview
   * @param scriptStr UTF8 encoded JavaScript code to run
   * @param func A function conforming to completionHandlerFunc that should be called on successful execution of the script */
  void EvaluateJavaScript(const char* scriptStr, completionHandlerFunc func = nullptr);
  
  /** Enable scrolling on the webview. NOTE: currently only implemented for iOS */
  void EnableScroll(bool enable);
  
  /** Set the bounds of the webview in the parent window. xywh are specifed in relation to a 1:1 non retina screen. On Windows the screen scale is passed in. */
  void SetWebViewBounds(float x, float y, float w, float h, float scale = 1.); //TODO: get screen scale in impl?

  /** Called when the web view is ready to receive navigation instructions*/
  virtual void OnWebViewReady() {}
  
  /** Called after navigation instructions have been exectued and e.g. a page has loaded */
  virtual void OnWebContentLoaded() {}
  
  /** When a script in the web view posts a message, it will arrive as a UTF8 json string here */
  virtual void OnMessageFromWebView(const char* json) {}

#if PLATFORM_WINDOWS
  /** Set the paths required for the Windows ICoreWebView2 component
   * @param dllPath (Windows only) an absolute path to the WebView2Loader.dll that is required to use the WebView2 on windows
   * @param tmpPath (Windows only) an absolute path to the folder that should be used */
  void SetWebViewPaths(const char* dllPath, const char* tmpPath) { mDLLPath.Set(dllPath); mTmpPath.Set(tmpPath); }
#endif
  
private:
  bool mOpaque = true;
#if PLATFORM_MAC || PLATFORM_IOS
  void* mWKWebView = nullptr;
  void* mWebConfig = nullptr;
  void* mScriptHandler = nullptr;
#elif PLATFORM_WINDOWS
  wil::com_ptr<ICoreWebView2Controller> mWebViewCtrlr;
  wil::com_ptr<ICoreWebView2> mWebViewWnd;
  EventRegistrationToken mWebMessageReceivedToken;
  EventRegistrationToken mNavigationCompletedToken;
  WDL_String mDLLPath;
  WDL_String mTmpPath;
  HMODULE mDLLHandle = nullptr;
#endif
};

END_IPLUG_NAMESPACE
