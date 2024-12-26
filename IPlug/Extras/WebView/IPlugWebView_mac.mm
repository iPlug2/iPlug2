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

#include <objc/objc.h>
#if !__has_feature(objc_arc)
#error This file must be compiled with Arc. Use -fobjc-arc flag
#endif

#import <WebKit/WebKit.h>
#import "IPlugWKWebView.h"
#import "IPlugWKWebViewScriptMessageHandler.h"
#import "IPlugWKWebViewDelegate.h"
#import "IPlugWKWebViewUIDelegate.h"

#include "IPlugWebView.h"
#include "IPlugPaths.h"

namespace iplug {
extern bool GetResourcePathFromBundle(const char* fileName, const char* searchExt, WDL_String& fullPath, const char* bundleID);
}

BEGIN_IPLUG_NAMESPACE

class IWebViewImpl
{
public:
  IWebViewImpl(IWebView* owner);
  ~IWebViewImpl();
  
  void* OpenWebView(void* pParent, float x, float y, float w, float h, float scale);
  void CloseWebView();
  void HideWebView(bool hide);
  
  void LoadHTML(const char* html);
  void LoadURL(const char* url);
  void LoadFile(const char* fileName, const char* _Nullable bundleID);
  void ReloadPageContent();
  void EvaluateJavaScript(const char* scriptStr, IWebView::completionHandlerFunc func);
  void EnableScroll(bool enable);
  void EnableInteraction(bool enable);
  void SetWebViewBounds(float x, float y, float w, float h, float scale);
  void GetWebRoot(WDL_String& path) const { path.Set(mWebRoot.Get()); }

  void GetLocalDownloadPathForFile(const char* fileName, WDL_String& localPath);

private:
  IWebView* mIWebView;
  WDL_String mWebRoot;
  WKWebViewConfiguration* _Nullable mWebConfig;
  IPLUG_WKWEBVIEW* _Nullable mWKWebView;
  IPLUG_WKSCRIPTMESSAGEHANDLER* _Nullable mScriptMessageHandler;
  IPLUG_WKWEBVIEW_DELEGATE* _Nullable mNavigationDelegate;
  IPLUG_WKWEBVIEW_UI_DELEGATE* _Nullable mUIDelegate;
};

END_IPLUG_NAMESPACE

using namespace iplug;

#pragma mark - Impl

IWebViewImpl::IWebViewImpl(IWebView* owner)
: mIWebView(owner)
, mWebConfig(nil)
, mWKWebView(nil)
, mScriptMessageHandler(nil)
, mNavigationDelegate(nil)
{
}

IWebViewImpl::~IWebViewImpl()
{
  CloseWebView();
}

void* IWebViewImpl::OpenWebView(void* pParent, float x, float y, float w, float h, float scale)
{
  WKWebViewConfiguration* webConfig = [[WKWebViewConfiguration alloc] init];
  WKPreferences* preferences = [[WKPreferences alloc] init];
  
  WKUserContentController* controller = [[WKUserContentController alloc] init];
  webConfig.userContentController = controller;

  [webConfig setValue:@YES forKey:@"allowUniversalAccessFromFileURLs"];
  auto* scriptMessageHandler = [[IPLUG_WKSCRIPTMESSAGEHANDLER alloc] initWithIWebView: mIWebView];
  [controller addScriptMessageHandler: scriptMessageHandler name:@"callback"];

  if (mIWebView->GetEnableDevTools())
  {
    [preferences setValue:@YES forKey:@"developerExtrasEnabled"];
  }
  
  [preferences setValue:@YES forKey:@"DOMPasteAllowed"];
  [preferences setValue:@YES forKey:@"javaScriptCanAccessClipboard"];
  
  webConfig.preferences = preferences;
  if (@available(macOS 10.13, *))
  {
    NSString* customUrlScheme = [NSString stringWithUTF8String:mIWebView->GetCustomUrlScheme()];
    const BOOL useCustomUrlScheme = [customUrlScheme length];

    if (useCustomUrlScheme)
    {
      [webConfig setURLSchemeHandler:scriptMessageHandler forURLScheme:[NSString stringWithUTF8String:mIWebView->GetCustomUrlScheme()]];
    }
  }
  
  // this script adds a function IPlugSendMsg that is used to call the platform webview messaging function in JS
  [controller addUserScript:[[WKUserScript alloc] initWithSource:
                             @"function IPlugSendMsg(m) { webkit.messageHandlers.callback.postMessage(m); }"
                             injectionTime:WKUserScriptInjectionTimeAtDocumentStart
                             forMainFrameOnly:YES]];

  // this script prevents view scaling on iOS
  [controller addUserScript:[[WKUserScript alloc] initWithSource:
                             @"var meta = document.createElement('meta'); meta.name = 'viewport'; \
                               meta.content = 'width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, shrink-to-fit=YES'; \
                               var head = document.getElementsByTagName('head')[0]; \
                               head.appendChild(meta);"
                             injectionTime:WKUserScriptInjectionTimeAtDocumentEnd
                             forMainFrameOnly:YES]];
  
  // this script receives global key down events and forwards them to the C++ side
  [controller addUserScript:[[WKUserScript alloc] initWithSource:
                             @"document.addEventListener('keydown', function(e) { if(document.activeElement.type != \"text\") { IPlugSendMsg({'msg': 'SKPFUI', 'keyCode': e.keyCode, 'utf8': e.key, 'S': e.shiftKey, 'C': e.ctrlKey, 'A': e.altKey, 'isUp': false}); e.preventDefault(); }});"
                             injectionTime:WKUserScriptInjectionTimeAtDocumentStart
                             forMainFrameOnly:YES]];
  
  // this script receives global key up events and forwards them to the C++ side
  [controller addUserScript:[[WKUserScript alloc] initWithSource:
                             @"document.addEventListener('keyup', function(e) { if(document.activeElement.type != \"text\") { IPlugSendMsg({'msg': 'SKPFUI', 'keyCode': e.keyCode, 'utf8': e.key, 'S': e.shiftKey, 'C': e.ctrlKey, 'A': e.altKey, 'isUp': true}); e.preventDefault(); }});"
                             injectionTime:WKUserScriptInjectionTimeAtDocumentStart
                             forMainFrameOnly:YES]];
  
  IPLUG_WKWEBVIEW* wkWebView = [[IPLUG_WKWEBVIEW alloc] initWithFrame: CGRectMake(x, y, w, h) configuration:webConfig];
  
  const auto isTransparent = !mIWebView->IsOpaque();

#if defined OS_IOS
  if (isTransparent)
  {
    wkWebView.backgroundColor = [UIColor clearColor];
    wkWebView.scrollView.backgroundColor = [UIColor clearColor];
    wkWebView.opaque = NO;
  }
#endif

#if defined OS_MAC
  if (isTransparent)
  {
    [wkWebView setValue:@(NO) forKey:@"drawsBackground"];
  }
  
  [wkWebView setAllowsMagnification:NO];
#endif
  
  auto* navigationDelegate = [[IPLUG_WKWEBVIEW_DELEGATE alloc] initWithIWebView: mIWebView];
  [wkWebView setNavigationDelegate:navigationDelegate];

  auto* uiDelegate = [[IPLUG_WKWEBVIEW_UI_DELEGATE alloc] initWithIWebView: mIWebView];
  [wkWebView setUIDelegate:uiDelegate];

  mWebConfig = webConfig;
  mWKWebView = wkWebView;
  mScriptMessageHandler = scriptMessageHandler;
  mNavigationDelegate = navigationDelegate;
  mUIDelegate = uiDelegate;
  
  mIWebView->OnWebViewReady();

  return (__bridge void*) wkWebView;
}

void IWebViewImpl::CloseWebView()
{
  [mWKWebView removeFromSuperview];
  
  mWebConfig = nil;
  mWKWebView = nil;
  mScriptMessageHandler = nil;
  mNavigationDelegate = nil;
}

void IWebViewImpl::HideWebView(bool hide)
{
  mWKWebView.hidden = hide;
}

void IWebViewImpl::LoadHTML(const char* html)
{
  [mWKWebView loadHTMLString:[NSString stringWithUTF8String:html] baseURL:nil];
}

void IWebViewImpl::LoadURL(const char* url)
{
  NSURL* nsURL = [NSURL URLWithString:[NSString stringWithUTF8String:url] relativeToURL:nil];
  NSURLRequest* req = [[NSURLRequest alloc] initWithURL:nsURL];
  [mWKWebView loadRequest:req];
}

void IWebViewImpl::LoadFile(const char* fileName, const char* _Nullable bundleID)
{
  WDL_String fullPath;
  
  if (bundleID != nullptr && strlen(bundleID) != 0)
  {
    WDL_String fileNameWeb("web/");
    fileNameWeb.Append(fileName);
    
    GetResourcePathFromBundle(fileNameWeb.Get(), fileNameWeb.get_fileext() + 1 /* remove . */, fullPath, bundleID);
  }
  else
  {
    fullPath.Set(fileName);
  }
  
  NSString* pPath = [NSString stringWithUTF8String:fullPath.Get()];
  
  fullPath.remove_filepart();
  mWebRoot.Set(fullPath.Get());

  // If a custom url scheme is provided use it, otherwise use a file Url
  NSString* customUrlScheme = [NSString stringWithUTF8String:mIWebView->GetCustomUrlScheme()];
  const BOOL useCustomUrlScheme = [customUrlScheme length];
  NSString* urlScheme = @"file:";
  
  if (useCustomUrlScheme)
  {
    urlScheme = [urlScheme stringByReplacingOccurrencesOfString:@"file" withString:customUrlScheme];
  }
  
  NSString* webroot = [urlScheme stringByAppendingString:[pPath stringByReplacingOccurrencesOfString:[NSString stringWithUTF8String:fileName] withString:@""]];

  NSURL* pageUrl = [NSURL URLWithString:[webroot stringByAppendingString:[NSString stringWithUTF8String:fileName]] relativeToURL:nil];

  if (useCustomUrlScheme)
  {
#if defined OS_MAC && defined _DEBUG
    NSString* homeDir = NSHomeDirectory();
    
    if ([homeDir containsString:@"Library/Containers/"])
    {
      NSString* absolutePath = [[pageUrl path] stringByStandardizingPath];
      if (![absolutePath hasPrefix:homeDir]) {
        NSLog(@"Warning: Attempting to load URL outside container directory in sandboxed app: %@", absolutePath);
      }
    }
#endif
    
    NSURLRequest* req = [[NSURLRequest alloc] initWithURL:pageUrl];
    [mWKWebView loadRequest:req];
  }
  else
  {
    NSURL* rootUrl = [NSURL URLWithString:webroot relativeToURL:nil];
    [mWKWebView loadFileURL:pageUrl allowingReadAccessToURL:rootUrl];
  }
}

void IWebViewImpl::ReloadPageContent()
{
  [mWKWebView reload];
}

void IWebViewImpl::EvaluateJavaScript(const char* scriptStr, IWebView::completionHandlerFunc func)
{
  if (mWKWebView && ![mWKWebView isLoading])
  {
    [mWKWebView evaluateJavaScript:[NSString stringWithUTF8String:scriptStr] completionHandler:^(NSString *result, NSError *error) {
      if (error != nil)
        NSLog(@"Error %@",error);
      else if(func)
      {
        func([result UTF8String]);
      }
    }];
  }
}

void IWebViewImpl::EnableScroll(bool enable)
{
#ifdef OS_IOS
  [mWKWebView.scrollView setScrollEnabled:enable];
#endif
}

void IWebViewImpl::EnableInteraction(bool enable)
{
  [mWKWebView setEnableInteraction:enable];
}

void IWebViewImpl::SetWebViewBounds(float x, float y, float w, float h, float scale)
{
  [mWKWebView setFrame: CGRectMake(x, y, w, h) ];

#ifdef OS_MAC
  if (@available(macOS 11.0, *)) {
    [mWKWebView setPageZoom:scale ];
  }
#endif
}

void IWebViewImpl::GetLocalDownloadPathForFile(const char* fileName, WDL_String& localPath)
{
  NSURL* url = [[NSURL fileURLWithPath:NSTemporaryDirectory()] URLByAppendingPathComponent:[[NSUUID UUID] UUIDString] isDirectory:YES];
  NSError *error = nil;
  @try {
    [[NSFileManager defaultManager] createDirectoryAtURL:url withIntermediateDirectories:YES attributes:nil error:&error];
    url = [url URLByAppendingPathComponent:[NSString stringWithUTF8String:fileName]];
    localPath.Set([[url absoluteString] UTF8String]);
  } @catch (NSException *exception)
  {
    NSLog(@"Error %@",error);
  }
}

#include "IPlugWebView.cpp"

#include "IPlugWKWebView.mm"
#include "IPlugWKWebViewScriptMessageHandler.mm"
#include "IPlugWKWebViewDelegate.mm"
#include "IPlugWKWebViewUIDelegate.mm"
