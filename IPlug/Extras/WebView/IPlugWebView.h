 /*
 ==============================================================================
 
  MIT License

  iPlug2 WebView Library
  Copyright (c) 2024 Oliver Larkin

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
 
 ==============================================================================
*/

#pragma once

#include "IPlugPlatform.h"
#include "IPlugLogger.h"
#include "wdlstring.h"
#include <functional>
#include <memory>

BEGIN_IPLUG_NAMESPACE

class IWebViewImpl;

/** IWebView is a base interface for hosting a platform web view inside an IPlug plug-in's UI */
class IWebView
{
public:
  using completionHandlerFunc = std::function<void(const char* result)>;

  /** Constructs an IWebView
  * @param opaque. Is the WebView opaque or does it have a transparent background
  * @param enableDevTools. Should the WebView's developer tools panel be enabled
  * @param customUrlScheme If set, the string used as a custom url scheme. It should not include a colon.
  * This means that the webview content is served as if it was on a web server (required for some web frameworks */
  IWebView(bool opaque = true, bool enableDevTools = false, const char* customUrlScheme = "");
  virtual ~IWebView();
  
  void* OpenWebView(void* pParent, float x, float y, float w, float h, float scale = 1.0f);
  void CloseWebView();
  void HideWebView(bool hide);
  
  /** Load an HTML string into the webview */
  void LoadHTML(const char* html);
  
  /** Instruct the webview to load an external URL */
  void LoadURL(const char* url);
  
  /** Load a file on disk into the web view
   * @param fileName On windows this should be an absolute path to the file you want to load. 
   * On macOS/iOS it can just be the file name if the file is packaged into a subfolder "web" of the bundle resources
   * @param bundleID The NSBundleID of the macOS/iOS bundle, not required on Windows */
  void LoadFile(const char* fileName, const char* bundleID = "");
  
  /** Trigger a reload of the webview's content */
  void ReloadPageContent();
  
  /** Runs some JavaScript in the webview
   * @param scriptStr UTF8 encoded JavaScript code to run
   * @param func A function conforming to completionHandlerFunc that should be called on successful execution of the script */
  void EvaluateJavaScript(const char* scriptStr, completionHandlerFunc func = nullptr);
  
  /** Enable scrolling on the webview. NOTE: currently only implemented for iOS */
  void EnableScroll(bool enable);
  
  /** Sets whether the webview is interactive */
  void EnableInteraction(bool enable);
  
  /** Set the bounds of the webview in the parent window. xywh are specifed in relation to a 1:1 non retina screen */
  void SetWebViewBounds(float x, float y, float w, float h, float scale = 1.);
  
  /** Fills the path where web content is being served from, when LoadFile() is used */
  void GetWebRoot(WDL_String& path) const;
  
  /** Returns the custom URL scheme, if set */
  const char* GetCustomUrlScheme() const { return mCustomUrlScheme.Get(); }
  
  /** Are developer tools enabled on this webview */
  bool GetEnableDevTools() const { return mEnableDevTools; }

  /** True if the webview was configured opaque */
  bool IsOpaque() const { return mOpaque; }

  /** Used to set the URL scheme after the IWebView has been contstructed; */
  void SetCustomUrlScheme(const char* customUrlScheme) { mCustomUrlScheme.Set(customUrlScheme); }
  
  /** Used to toggle devtools after the IWebView has been contstructed. Will only have an effect after close/reopen */
  void SetEnableDevTools(bool enable) { mEnableDevTools = enable; }
  
#pragma mark -
  
  /** Called when the web view is ready to receive navigation instructions */
  virtual void OnWebViewReady() {}
  
  /** Called after navigation instructions have been exectued and e.g. a page has loaded */
  virtual void OnWebContentLoaded() {}
  
  /** When a script in the web view posts a message, it will arrive as a UTF8 json string here */
  virtual void OnMessageFromWebView(const char* json) {}
  
  /** Override to filter URLs */
  virtual bool OnCanNavigateToURL(const char* url) { return true; }
  
  /** Override to filter MIME types that should be downloaded */
  virtual bool OnCanDownloadMIMEType(const char* mimeType) { return false; }
  
  /** Override to download the file to a specific location other than e.g. NSTemporaryDirectory */
  virtual void OnGetLocalDownloadPathForFile(const char* fileName, WDL_String& localPath);

  /** Override to handle file download success */
  virtual void OnDownloadedFile(const char* path) { DBGMSG("Downloaded %s\n", path);}
  
  /** Override to handle file download failure */
  virtual void OnFailedToDownloadFile(const char* path) { DBGMSG("Downloading %s failed\n", path); }

  /** Override to handle file download progress */
  virtual void OnReceivedData(size_t numBytesReceived, size_t totalNumBytes) {}

private:
  std::unique_ptr<IWebViewImpl> mpImpl;
  bool mOpaque;
  bool mEnableDevTools = false;
  WDL_String mCustomUrlScheme;
};

END_IPLUG_NAMESPACE
