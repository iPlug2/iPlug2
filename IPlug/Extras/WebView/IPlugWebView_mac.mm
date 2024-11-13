 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
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
  IWebViewImpl(IWebView* _Nonnull owner);
  ~IWebViewImpl();
  
  void* OpenWebView(void* _Nonnull pParent, float x, float y, float w, float h, float scale);
  void CloseWebView();
  void HideWebView(bool hide);
  
  void LoadHTML(const char* _Nonnull html);
  void LoadURL(const char* _Nonnull url);
  void LoadFile(const char* _Nonnull fileName, const char* _Nullable bundleID);
  void ReloadPageContent();
  void EvaluateJavaScript(const char* _Nonnull scriptStr, IWebView::completionHandlerFunc func);
  void EnableScroll(bool enable);
  void EnableInteraction(bool enable);
  void SetWebViewBounds(float x, float y, float w, float h, float scale);
  void GetWebRoot(WDL_String& path) const { path.Set(mWebRoot.Get()); }

  void GetLocalDownloadPathForFile(const char* _Nonnull fileName, WDL_String& localPath);

private:
  IWebView* _Nonnull mIWebView;
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

IWebViewImpl::IWebViewImpl(IWebView* _Nonnull owner)
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

void* IWebViewImpl::OpenWebView(void* _Nonnull pParent, float x, float y, float w, float h, float scale)
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
  WKUserScript* script1 = [[WKUserScript alloc] initWithSource:
                           @"function IPlugSendMsg(m) { webkit.messageHandlers.callback.postMessage(m); }" 
                           injectionTime:WKUserScriptInjectionTimeAtDocumentStart 
                           forMainFrameOnly:YES];
  [controller addUserScript:script1];

  // this script prevents view scaling on iOS
  WKUserScript* script2 = [[WKUserScript alloc] initWithSource:
                           @"var meta = document.createElement('meta'); meta.name = 'viewport'; \
                             meta.content = 'width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, shrink-to-fit=YES'; \
                             var head = document.getElementsByTagName('head')[0]; \
                             head.appendChild(meta);"
                           injectionTime:WKUserScriptInjectionTimeAtDocumentEnd 
                           forMainFrameOnly:YES];
  [controller addUserScript:script2];
  
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

#ifndef OS_IOS
  auto* uiDelegate = [[IPLUG_WKWEBVIEW_UI_DELEGATE alloc] initWithIWebView: mIWebView];
  [wkWebView setUIDelegate:uiDelegate];
#else
  auto uiDelegate = nullptr;
#endif
  
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

void IWebViewImpl::LoadHTML(const char* _Nonnull html)
{
  [mWKWebView loadHTMLString:[NSString stringWithUTF8String:html] baseURL:nil];
}

void IWebViewImpl::LoadURL(const char* _Nonnull url)
{
  NSURL* nsURL = [NSURL URLWithString:[NSString stringWithUTF8String:url] relativeToURL:nil];
  NSURLRequest* req = [[NSURLRequest alloc] initWithURL:nsURL];
  [mWKWebView loadRequest:req];
}

void IWebViewImpl::LoadFile(const char* _Nonnull fileName, const char* _Nullable bundleID)
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

void IWebViewImpl::EvaluateJavaScript(const char* _Nonnull scriptStr, IWebView::completionHandlerFunc func)
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

void IWebViewImpl::GetLocalDownloadPathForFile(const char* _Nonnull fileName, WDL_String& localPath)
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
