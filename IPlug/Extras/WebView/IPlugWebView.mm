 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#if !__has_feature(objc_arc)
#error This file must be compiled with Arc. Use -fobjc-arc flag
#endif

#import <WebKit/WebKit.h>

#include "IPlugWebView.h"
#include "IPlugPaths.h"

namespace iplug {
extern bool GetResourcePathFromBundle(const char* fileName, const char* searchExt, WDL_String& fullPath, const char* bundleID);
}

using namespace iplug;

@interface ScriptHandler : NSObject <WKScriptMessageHandler, WKNavigationDelegate>
{
  IWebView* mWebView;
}
@end

@implementation ScriptHandler

-(id) initWithIWebView:(IWebView*) webView
{
  self = [super init];
  
  if (self)
    mWebView = webView;
  
  return self;
}

- (void) userContentController:(nonnull WKUserContentController*) userContentController didReceiveScriptMessage:(nonnull WKScriptMessage*) message
{
  if ([[message name] isEqualToString:@"callback"])
  {
    NSDictionary* dict = (NSDictionary*) message.body;
    NSData* data = [NSJSONSerialization dataWithJSONObject:dict options:NSJSONWritingPrettyPrinted error:nil];
    NSString* jsonString = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    mWebView->OnMessageFromWebView([jsonString UTF8String]);
  }
}

- (void) webView:(WKWebView*) webView didFinishNavigation:(WKNavigation*) navigation
{
  mWebView->OnWebContentLoaded();
}

@end

IWebView::IWebView(bool opaque)
: mOpaque(opaque)
{
}

IWebView::~IWebView()
{
  CloseWebView();
}

void* IWebView::OpenWebView(void* pParent, float x, float y, float w, float h, float scale)
{  
  WKWebViewConfiguration* webConfig = [[WKWebViewConfiguration alloc] init];
  WKPreferences* preferences = [[WKPreferences alloc] init];
  
  WKUserContentController* controller = [[WKUserContentController alloc] init];
  webConfig.userContentController = controller;

  ScriptHandler* scriptHandler = [[ScriptHandler alloc] initWithIWebView: this];
  [controller addScriptMessageHandler: scriptHandler name:@"callback"];
  [preferences setValue:@YES forKey:@"developerExtrasEnabled"];
  webConfig.preferences = preferences;
  
  // this script adds a function IPlugSendMsg that is used to call the platform webview messaging function in JS
  WKUserScript* script1 = [[WKUserScript alloc] initWithSource:@"function IPlugSendMsg(m) { webkit.messageHandlers.callback.postMessage(m); }" injectionTime:WKUserScriptInjectionTimeAtDocumentStart forMainFrameOnly:YES];
  [controller addUserScript:script1];

  // this script prevents view scaling on iOS
  WKUserScript* script2 = [[WKUserScript alloc] initWithSource:@"var meta = document.createElement('meta'); meta.name = 'viewport'; meta.content = 'width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, shrink-to-fit=YES'; var head = document.getElementsByTagName('head')[0]; head.appendChild(meta);"
                                                 injectionTime:WKUserScriptInjectionTimeAtDocumentEnd forMainFrameOnly:YES];
  [controller addUserScript:script2];
  
  
  WKWebView* webView = [[WKWebView alloc] initWithFrame: MAKERECT(x, y, w, h) configuration:webConfig];
  

#if defined OS_IOS
  if (!mOpaque)
  {
    webView.backgroundColor = [UIColor clearColor];
    webView.scrollView.backgroundColor = [UIColor clearColor];
    webView.opaque = NO;
  }
#endif

#if defined OS_MAC
  if (!mOpaque)
    [webView setValue:@(NO) forKey:@"drawsBackground"];
//    [webView setValue:[NSNumber numberWithBool:YES]  forKey:@"drawsTransparentBackground"]; // deprecated
  
  [webView setAllowsMagnification:NO];
#endif
  
  [webView setNavigationDelegate:scriptHandler];
    
//#ifdef OS_MAC
//  [webView setAutoresizingMask: NSViewHeightSizable|NSViewWidthSizable|NSViewMinXMargin|NSViewMaxXMargin|NSViewMinYMargin|NSViewMaxYMargin ];
//#else
//  [webView setAutoresizingMask: UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleBottomMargin | UIViewAutoresizingFlexibleTopMargin];
//#endif
//  [parentView setAutoresizesSubviews:YES];
  
  mWebConfig = (__bridge void*) webConfig;
  mWKWebView = (__bridge void*) webView;
  mScriptHandler = (__bridge void*) scriptHandler;
  
  OnWebViewReady();

  return (__bridge void*) webView;
}

void IWebView::CloseWebView()
{
  mWKWebView = nullptr;
  mWKWebView = nullptr;
  mScriptHandler = nullptr;
}

void IWebView::LoadHTML(const char* html)
{
  WKWebView* webView = (__bridge WKWebView*) mWKWebView;
  [webView loadHTMLString:[NSString stringWithUTF8String:html] baseURL:nil];
}

void IWebView::LoadURL(const char* url)
{
  WKWebView* webView = (__bridge WKWebView*) mWKWebView;
  
  NSURL* nsurl = [NSURL URLWithString:[NSString stringWithUTF8String:url] relativeToURL:nil];
  NSURLRequest* req = [[NSURLRequest alloc] initWithURL:nsurl];
  [webView loadRequest:req];
}

void IWebView::LoadFile(const char* fileName, const char* bundleID)
{
  WKWebView* webView = (__bridge WKWebView*) mWKWebView;

  WDL_String fullPath;
  WDL_String fileNameWeb("web/");
  fileNameWeb.Append(fileName);

  GetResourcePathFromBundle(fileNameWeb.Get(), fileNameWeb.get_fileext() + 1 /* remove . */, fullPath, bundleID);
  
  NSString* pPath = [NSString stringWithUTF8String:fullPath.Get()];

  NSString* str = @"file:";
  NSString* webroot = [str stringByAppendingString:[pPath stringByReplacingOccurrencesOfString:[NSString stringWithUTF8String:fileName] withString:@""]];
  NSURL* pageUrl = [NSURL URLWithString:[webroot stringByAppendingString:[NSString stringWithUTF8String:fileName]] relativeToURL:nil];
  NSURL* rootUrl = [NSURL URLWithString:webroot relativeToURL:nil];

  [webView loadFileURL:pageUrl allowingReadAccessToURL:rootUrl];
}

void IWebView::EvaluateJavaScript(const char* scriptStr, completionHandlerFunc func)
{
  WKWebView* webView = (__bridge WKWebView*) mWKWebView;
  
  if (webView && ![webView isLoading])
  {
    [webView evaluateJavaScript:[NSString stringWithUTF8String:scriptStr] completionHandler:^(NSString *result, NSError *error) {
      if (error != nil)
        NSLog(@"Error %@",error);
      else if(func)
      {
        func([result UTF8String]);
      }
    }];
  }
}

void IWebView::EnableScroll(bool enable)
{
#ifdef OS_IOS
  WKWebView* webView = (__bridge WKWebView*) mWKWebView;
  [webView.scrollView setScrollEnabled:enable];
#endif
}

void IWebView::SetWebViewBounds(float x, float y, float w, float h, float scale)
{
//  [NSAnimationContext beginGrouping]; // Prevent animated resizing
//  [[NSAnimationContext currentContext] setDuration:0.0];
  [(__bridge WKWebView*) mWKWebView setFrame: MAKERECT(x, y, w, h) ];
#ifdef OS_MAC
  [(__bridge WKWebView*) mWKWebView setMagnification: scale ];
#endif
//  [NSAnimationContext endGrouping];
}

