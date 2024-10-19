#pragma once

#import <WebKit/WebKit.h>

namespace iplug {
class IWebView;
}

@class IPLUG_WKWEBVIEW;

@interface IPLUG_WKSCRIPTMESSAGEHANDLER : NSObject <WKScriptMessageHandler, WKURLSchemeHandler>
{
  iplug::IWebView* mWebView;
  NSURL* downloadDestinationURL;
}

- (void)dealloc;

- (id)initWithIWebView:(iplug::IWebView*)webView;

- (void)userContentController:(nonnull WKUserContentController*)userContentController 
      didReceiveScriptMessage:(nonnull WKScriptMessage*)message;

- (void)webView:(IPLUG_WKWEBVIEW*)webView didFinishNavigation:(WKNavigation*)navigation;

- (NSURL*)changeURLScheme:(NSURL*)url toScheme:(NSString*)newScheme;

- (void)webView:(nonnull WKWebView *)webView 
startURLSchemeTask:(nonnull id<WKURLSchemeTask>)urlSchemeTask API_AVAILABLE(macos(10.13));

- (void)webView:(nonnull WKWebView *)webView 
stopURLSchemeTask:(nonnull id<WKURLSchemeTask>)urlSchemeTask API_AVAILABLE(macos(10.13));

@end

