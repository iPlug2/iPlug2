#pragma once

#import <WebKit/WebKit.h>

namespace iplug {
class IWebView;
}

@class IPLUG_WKWEBVIEW;

@interface IPLUG_WKSCRIPTMESSAGEHANDLER : NSObject <WKScriptMessageHandler, WKURLSchemeHandler>
{
  iplug::IWebView* _Nonnull mIWebView;
  NSURL* _Nullable downloadDestinationURL;
}

- (void)dealloc;

- (id _Nonnull)initWithIWebView:(iplug::IWebView* _Nonnull)webView;

- (void)userContentController:(nonnull WKUserContentController*)userContentController 
      didReceiveScriptMessage:(nonnull WKScriptMessage*)message;

#pragma mark - WKURLSchemeHandler

- (void)webView:(IPLUG_WKWEBVIEW* _Nonnull)webView didFinishNavigation:(WKNavigation* _Nullable)navigation;

- (NSURL* _Nullable)changeURLScheme:(NSURL* _Nonnull)url toScheme:(NSString* _Nonnull)newScheme;

- (void)webView:(nonnull WKWebView *)webView 
startURLSchemeTask:(nonnull id<WKURLSchemeTask>)urlSchemeTask API_AVAILABLE(macos(10.13));

- (void)webView:(nonnull WKWebView *)webView 
stopURLSchemeTask:(nonnull id<WKURLSchemeTask>)urlSchemeTask API_AVAILABLE(macos(10.13));

@end

