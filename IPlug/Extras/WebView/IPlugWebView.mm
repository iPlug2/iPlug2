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

@interface IPLUG_WKSCRIPTHANDLER : NSObject <WKScriptMessageHandler, WKNavigationDelegate, WKURLSchemeHandler, WKDownloadDelegate>
{
  IWebView* mWebView;
  NSURL* downloadDestinationURL;
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

- (NSURL*) changeURLScheme:(NSURL*) url toScheme:(NSString*) newScheme
{
  NSURLComponents* components = [NSURLComponents componentsWithURL:url resolvingAgainstBaseURL:YES];
  components.scheme = newScheme;
  return components.URL;
}

- (void) webView:(nonnull WKWebView *)webView startURLSchemeTask:(nonnull id<WKURLSchemeTask>)urlSchemeTask  API_AVAILABLE(macos(10.13)){

  if ([urlSchemeTask.request.URL.absoluteString containsString:@"iplug2:"])
  {
    NSURL* customFileURL = urlSchemeTask.request.URL;
    NSURL* fileURL = [self changeURLScheme:customFileURL toScheme:@"file"];
    NSHTTPURLResponse* response = NULL;
    
    if ([[NSFileManager defaultManager] fileExistsAtPath:fileURL.path])
    {
      NSData* data = [[NSData alloc] initWithContentsOfURL:fileURL];
      NSString* contentLengthStr = [[NSString alloc] initWithFormat:@"%lu", [data length]];
      NSString* contentTypeStr = @"text/plain";
      NSString* extStr = [[fileURL path] pathExtension];
      if ([extStr isEqualToString:@"html"]) contentTypeStr = @"text/html";
      else if ([extStr isEqualToString:@"css"]) contentTypeStr = @"text/css";
      else if ([extStr isEqualToString:@"js"]) contentTypeStr = @"text/javascript";
      else if ([extStr isEqualToString:@"jpg"]) contentTypeStr = @"image/jpeg";
      else if ([extStr isEqualToString:@"jpeg"]) contentTypeStr = @"image/jpeg";
      else if ([extStr isEqualToString:@"svg"]) contentTypeStr = @"image/svg+xml";

      NSDictionary* headerFields = [NSDictionary dictionaryWithObjects:@[contentLengthStr, contentTypeStr] forKeys:@[@"Content-Length", @"Content-Type"]];
      response = [[NSHTTPURLResponse alloc] initWithURL:customFileURL statusCode:200 HTTPVersion:@"HTTP/1.1" headerFields:headerFields];
      [urlSchemeTask didReceiveResponse:response];
      [urlSchemeTask didReceiveData:data];
    }
    else
    {
      response = [[NSHTTPURLResponse alloc] initWithURL:customFileURL statusCode:404 HTTPVersion:@"HTTP/1.1" headerFields:nil];
      [urlSchemeTask didReceiveResponse:response];
    }
    [urlSchemeTask didFinish];
  }
}

- (void) webView:(nonnull WKWebView *)webView stopURLSchemeTask:(nonnull id<WKURLSchemeTask>)urlSchemeTask  API_AVAILABLE(macos(10.13)){
  
}

#pragma mark - WKNavigationDelegate

- (void)webView:(WKWebView *)webView decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler
{
  bool allow = mWebView->CanNavigateToURL([[[[navigationAction request] URL] absoluteString] UTF8String]);
  
  if (allow)
  {
    decisionHandler(WKNavigationActionPolicyAllow);
  }
  else
  {
    decisionHandler(WKNavigationActionPolicyCancel);
  }
}

- (void)webView:(WKWebView *)webView decidePolicyForNavigationResponse:(WKNavigationResponse *)navigationResponse decisionHandler:(void (^)(WKNavigationResponsePolicy))decisionHandler
{
  bool allow = mWebView->CanNavigateToURL([[[[navigationResponse response] URL] absoluteString] UTF8String]);
  
  if (!allow)
  {
    decisionHandler(WKNavigationResponsePolicyCancel);
  }
  
  if (@available(macOS 11.3, iOS 14.5, *))
  {
    bool allowMimeType = mWebView->CanDownloadMIMEType([navigationResponse.response.MIMEType UTF8String]);

    if (allowMimeType)
    {
      decisionHandler(WKNavigationResponsePolicyDownload);
    }
    else
    {
      decisionHandler(WKNavigationResponsePolicyAllow);
    }
  }
  else
  {
    decisionHandler(WKNavigationResponsePolicyAllow);
  }
}

- (void)webView:(WKWebView *)webView navigationResponse:(WKNavigationResponse *)navigationResponse didBecomeDownload:(WKDownload *)download API_AVAILABLE(macos(11.3), ios(14.5));
{
  download.delegate = self;
}

#pragma mark - WKDownloadDelegate


- (void)download:(WKDownload *)download decideDestinationUsingResponse:(NSURLResponse *)response suggestedFilename:(NSString *)filename completionHandler:(void (^)(NSURL * _Nullable))completionHandler API_AVAILABLE(macos(11.3), ios(14.5))
{
  WDL_String localPath;
  mWebView->GetLocalDownloadPathForFile([filename UTF8String], localPath);
  downloadDestinationURL = [NSURL URLWithString:[NSString stringWithUTF8String:localPath.Get()]];
  completionHandler(downloadDestinationURL);
}

- (void)download:(WKDownload *)download didReceiveData:(int64_t)length API_AVAILABLE(macos(11.3), ios(14.5))
{
  mWebView->DidReceiveBytes(length, 0);
}

- (void)download:(WKDownload *)download didFailWithError:(NSError *)error API_AVAILABLE(macos(11.3), ios(14.5))
{
  mWebView->FailedToDownloadFile([[downloadDestinationURL absoluteString] UTF8String]);
  downloadDestinationURL = nil;
}

- (void)downloadDidFinish:(WKDownload *)download  API_AVAILABLE(macos(11.3), ios(14.5))
{
  mWebView->DidDownloadFile([[downloadDestinationURL path] UTF8String]);
  downloadDestinationURL = nil;
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

  [webConfig setValue:@YES forKey:@"allowUniversalAccessFromFileURLs"];
  IPLUG_WKSCRIPTHANDLER* scriptHandler = [[IPLUG_WKSCRIPTHANDLER alloc] initWithIWebView: this];
  [controller addScriptMessageHandler: scriptHandler name:@"callback"];
  
  if (enableDevTools)
  {
    [preferences setValue:@YES forKey:@"developerExtrasEnabled"];
  }
  
  [preferences setValue:@YES forKey:@"DOMPasteAllowed"];
  [preferences setValue:@YES forKey:@"javaScriptCanAccessClipboard"];
  
  webConfig.preferences = preferences;
  if (@available(macOS 10.13, *)) {
    [webConfig setURLSchemeHandler:scriptHandler forURLScheme:@"iplug2"];
  } else {
    // Fallback on earlier versions
  }
  
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
  
  NSURL* nsURL = [NSURL URLWithString:[NSString stringWithUTF8String:url] relativeToURL:nil];
  NSURLRequest* req = [[NSURLRequest alloc] initWithURL:nsURL];
  [webView loadRequest:req];
}

void IWebView::LoadFile(const char* fileName, const char* bundleID, bool useCustomScheme)
{
  IPLUG_WKWEBVIEW* webView = (__bridge IPLUG_WKWEBVIEW*) mWKWebView;

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

  NSString* urlScheme = useCustomScheme ? @"iplug2:" : @"file:";
  NSString* webroot = [urlScheme stringByAppendingString:[pPath stringByReplacingOccurrencesOfString:[NSString stringWithUTF8String:fileName] withString:@""]];

  NSURL* pageUrl = [NSURL URLWithString:[webroot stringByAppendingString:[NSString stringWithUTF8String:fileName]] relativeToURL:nil];

  if (useCustomScheme)
  {
    NSURLRequest* req = [[NSURLRequest alloc] initWithURL:pageUrl];
    [webView loadRequest:req];
  }
  else
  {
    NSURL* rootUrl = [NSURL URLWithString:webroot relativeToURL:nil];
    [webView loadFileURL:pageUrl allowingReadAccessToURL:rootUrl];
  }
}

void IWebView::ReloadPageContent()
{
  IPLUG_WKWEBVIEW* webView = (__bridge IPLUG_WKWEBVIEW*) mWKWebView;
  
  if (webView)
  {
    [webView reload];
  }
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

void IWebView::GetLocalDownloadPathForFile(const char* fileName, WDL_String& localPath)
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
