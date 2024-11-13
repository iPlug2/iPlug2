 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#import <WebKit/WebKit.h>

namespace iplug {
class IWebView;
}

@interface IPLUG_WKWEBVIEW_DELEGATE : NSObject <WKNavigationDelegate, WKDownloadDelegate>
{
  iplug::IWebView* _Nonnull mIWebView;
  NSURL* _Nullable downloadDestinationURL;
}

- (id _Nonnull)initWithIWebView:(iplug::IWebView* _Nonnull)webView;

- (void)dealloc;

- (void)webView:(WKWebView * _Nonnull)webView 
decidePolicyForNavigationAction:(WKNavigationAction * _Nonnull)navigationAction 
decisionHandler:(void (^ _Nonnull)(WKNavigationActionPolicy))decisionHandler;

- (void)webView:(WKWebView * _Nonnull)webView 
decidePolicyForNavigationResponse:(WKNavigationResponse * _Nonnull)navigationResponse 
decisionHandler:(void (^ _Nonnull)(WKNavigationResponsePolicy))decisionHandler;

#pragma mark WKDownloadDelegate

- (void)webView:(WKWebView * _Nonnull)webView 
navigationResponse:(WKNavigationResponse * _Nonnull)navigationResponse 
didBecomeDownload:(WKDownload * _Nonnull)download API_AVAILABLE(macos(11.3), ios(14.5));

- (void)download:(WKDownload * _Nonnull)download 
decideDestinationUsingResponse:(NSURLResponse * _Nonnull)response 
suggestedFilename:(NSString * _Nonnull)filename 
completionHandler:(void (^ _Nonnull)(NSURL * _Nullable))completionHandler API_AVAILABLE(macos(11.3), ios(14.5));

- (void)download:(WKDownload * _Nonnull)download 
didReceiveData:(int64_t)length API_AVAILABLE(macos(11.3), ios(14.5));

- (void)download:(WKDownload * _Nonnull)download 
didFailWithError:(NSError * _Nonnull)error API_AVAILABLE(macos(11.3), ios(14.5));

- (void)downloadDidFinish:(WKDownload * _Nonnull)download API_AVAILABLE(macos(11.3), ios(14.5));

@end
