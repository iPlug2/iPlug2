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

@interface IPLUG_WKWEBVIEW : WKWebView
{
  bool mEnableInteraction;
}
- (void)setEnableInteraction:(bool)enable;

@end

@implementation IPLUG_WKWEBVIEW

- (instancetype)initWithFrame:(CGRect)frame configuration:(WKWebViewConfiguration *)configuration
{
  self = [super initWithFrame:frame configuration:configuration];
  
  if (self)
  {
    mEnableInteraction = true;
  }
  return self;
}

#ifdef OS_MAC
- (NSView *)hitTest:(NSPoint)point
{
  if (!mEnableInteraction)
  {
    return nil;
  }
  else
    return [super hitTest:point];
}

- (void)willOpenMenu:(NSMenu *)menu withEvent:(NSEvent *)event
{
  [super willOpenMenu:menu withEvent:event];
  
  NSArray<NSString *> *WKStrings = @[
   @"WKMenuItemIdentifierCopy",
   @"WKMenuItemIdentifierCopyImage",
   @"WKMenuItemIdentifierCopyLink",
   @"WKMenuItemIdentifierDownloadImage",
   @"WKMenuItemIdentifierDownloadLinkedFile",
   @"WKMenuItemIdentifierGoBack",
   @"WKMenuItemIdentifierGoForward",
//   @"WKMenuItemIdentifierInspectElement",
   @"WKMenuItemIdentifierLookUp",
   @"WKMenuItemIdentifierOpenFrameInNewWindow",
   @"WKMenuItemIdentifierOpenImageInNewWindow",
   @"WKMenuItemIdentifierOpenLink",
   @"WKMenuItemIdentifierOpenLinkInNewWindow",
   @"WKMenuItemIdentifierPaste",
//   @"WKMenuItemIdentifierReload",
   @"WKMenuItemIdentifierSearchWeb",
   @"WKMenuItemIdentifierShowHideMediaControls",
   @"WKMenuItemIdentifierToggleFullScreen",
   @"WKMenuItemIdentifierTranslate",
   @"WKMenuItemIdentifierShareMenu",
   @"WKMenuItemIdentifierSpeechMenu"
  ];
  
  for (NSInteger itemIndex = 0; itemIndex < menu.itemArray.count; itemIndex++)
  {
    if ([WKStrings containsObject:menu.itemArray[itemIndex].identifier])
    {
      [menu removeItemAtIndex:itemIndex];
    }
  }
}

#endif

- (void)setEnableInteraction:(bool)enable
{
  mEnableInteraction = enable;
  
#ifdef OS_MAC
  if (!mEnableInteraction)
  {
    for (NSTrackingArea* trackingArea in self.trackingAreas)
    {
      [self removeTrackingArea:trackingArea];
    }
  }
#else
  self.userInteractionEnabled = mEnableInteraction;
#endif
}


@end

@interface IPLUG_WKSCRIPTHANDLER : NSObject <WKScriptMessageHandler, WKNavigationDelegate>
{
  IWebView* mWebView;
}
@end

@implementation IPLUG_WKSCRIPTHANDLER

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

- (void) webView:(IPLUG_WKWEBVIEW*) webView didFinishNavigation:(WKNavigation*) navigation
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

void* IWebView::OpenWebView(void* pParent, float x, float y, float w, float h, float scale, bool enableDevTools)
{  
  WKWebViewConfiguration* webConfig = [[WKWebViewConfiguration alloc] init];
  WKPreferences* preferences = [[WKPreferences alloc] init];
  
  WKUserContentController* controller = [[WKUserContentController alloc] init];
  webConfig.userContentController = controller;

  IPLUG_WKSCRIPTHANDLER* scriptHandler = [[IPLUG_WKSCRIPTHANDLER alloc] initWithIWebView: this];
  [controller addScriptMessageHandler: scriptHandler name:@"callback"];
  
  if (enableDevTools)
  {
    [preferences setValue:@YES forKey:@"developerExtrasEnabled"];
  }
  
  [preferences setValue:@YES forKey:@"DOMPasteAllowed"];
  [preferences setValue:@YES forKey:@"javaScriptCanAccessClipboard"];
  
  webConfig.preferences = preferences;
  
  // this script adds a function IPlugSendMsg that is used to call the platform webview messaging function in JS
  WKUserScript* script1 = [[WKUserScript alloc] initWithSource:
                           @"function IPlugSendMsg(m) { webkit.messageHandlers.callback.postMessage(m); }" injectionTime:WKUserScriptInjectionTimeAtDocumentStart forMainFrameOnly:YES];
  [controller addUserScript:script1];

  // this script prevents view scaling on iOS
  WKUserScript* script2 = [[WKUserScript alloc] initWithSource:
                           @"var meta = document.createElement('meta'); meta.name = 'viewport'; \
                             meta.content = 'width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, shrink-to-fit=YES'; \
                             var head = document.getElementsByTagName('head')[0]; \
                             head.appendChild(meta);"
                           injectionTime:WKUserScriptInjectionTimeAtDocumentEnd forMainFrameOnly:YES];
  [controller addUserScript:script2];
  
  
  IPLUG_WKWEBVIEW* webView = [[IPLUG_WKWEBVIEW alloc] initWithFrame: MAKERECT(x, y, w, h) configuration:webConfig];

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
  
  [webView setAllowsMagnification:NO];
#endif
  
  [webView setNavigationDelegate:scriptHandler];

  mWebConfig = (__bridge void*) webConfig;
  mWKWebView = (__bridge void*) webView;
  mScriptHandler = (__bridge void*) scriptHandler;
  
  OnWebViewReady();

  return (__bridge void*) webView;
}

void IWebView::CloseWebView()
{
  IPLUG_WKWEBVIEW* webView = (__bridge IPLUG_WKWEBVIEW*) mWKWebView;
  [webView removeFromSuperview];
  
  mWebConfig = nullptr;
  mWKWebView = nullptr;
  mScriptHandler = nullptr;
}

void IWebView::HideWebView(bool hide)
{
  /* NO-OP */
}

void IWebView::LoadHTML(const char* html)
{
  IPLUG_WKWEBVIEW* webView = (__bridge IPLUG_WKWEBVIEW*) mWKWebView;
  [webView loadHTMLString:[NSString stringWithUTF8String:html] baseURL:nil];
}

void IWebView::LoadURL(const char* url)
{
  IPLUG_WKWEBVIEW* webView = (__bridge IPLUG_WKWEBVIEW*) mWKWebView;
  
  NSURL* nsurl = [NSURL URLWithString:[NSString stringWithUTF8String:url] relativeToURL:nil];
  NSURLRequest* req = [[NSURLRequest alloc] initWithURL:nsurl];
  [webView loadRequest:req];
}

void IWebView::LoadFile(const char* fileName, const char* bundleID)
{
  IPLUG_WKWEBVIEW* webView = (__bridge IPLUG_WKWEBVIEW*) mWKWebView;

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
  IPLUG_WKWEBVIEW* webView = (__bridge IPLUG_WKWEBVIEW*) mWKWebView;
  
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
  IPLUG_WKWEBVIEW* webView = (__bridge IPLUG_WKWEBVIEW*) mWKWebView;
  [webView.scrollView setScrollEnabled:enable];
#endif
}

void IWebView::EnableInteraction(bool enable)
{
  IPLUG_WKWEBVIEW* webView = (__bridge IPLUG_WKWEBVIEW*) mWKWebView;
  [webView setEnableInteraction:enable];
}

void IWebView::SetWebViewBounds(float x, float y, float w, float h, float scale)
{
//  [NSAnimationContext beginGrouping]; // Prevent animated resizing
//  [[NSAnimationContext currentContext] setDuration:0.0];
  [(__bridge IPLUG_WKWEBVIEW*) mWKWebView setFrame: MAKERECT(x, y, w, h) ];

#ifdef OS_MAC
  if (@available(macOS 11.0, *)) {
    [(__bridge IPLUG_WKWEBVIEW*) mWKWebView setPageZoom:scale ];
  }
#endif

//  [NSAnimationContext endGrouping];
}

