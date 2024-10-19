#pragma once

#import <WebKit/WebKit.h>

namespace iplug {
class IWebView;
}

@interface IPLUG_WKWEBVIEW_DELEGATE : NSObject <WKNavigationDelegate, WKDownloadDelegate>
{
  iplug::IWebView* mWebView;
  NSURL* downloadDestinationURL;
}

- (id)initWithIWebView:(iplug::IWebView*)webView;

- (void)dealloc;

- (void)webView:(WKWebView *)webView 
decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction 
decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler;

- (void)webView:(WKWebView *)webView 
decidePolicyForNavigationResponse:(WKNavigationResponse *)navigationResponse 
decisionHandler:(void (^)(WKNavigationResponsePolicy))decisionHandler;

#pragma mark WKDownloadDelegate

- (void)webView:(WKWebView *)webView
navigationResponse:(WKNavigationResponse *)navigationResponse
didBecomeDownload:(WKDownload *)download;

- (void)download:(WKDownload *)download
decideDestinationUsingResponse:(NSURLResponse *)response
suggestedFilename:(NSString *)filename
completionHandler:(void (^)(NSURL * _Nullable))completionHandler;

- (void)download:(WKDownload *)download didReceiveData:(int64_t)length;

- (void)download:(WKDownload *)download didFailWithError:(NSError *)error;

- (void)downloadDidFinish:(WKDownload *)download;

@end
