#import "IPlugWKWebViewDelegate.h"

#include "IPlugWebView.h"

#if !__has_feature(objc_arc)
#error This file must be compiled with Arc. Use -fobjc-arc flag
#endif

using namespace iplug;

@implementation IPLUG_WKWEBVIEW_DELEGATE

- (id)initWithIWebView:(IWebView*)webView;
{
  self = [super init];
  if (self)
  {
    mWebView = webView;
  }
  return self;
}

- (void)dealloc
{
  mWebView = nullptr;
}


- (void)webView:(WKWebView *)webView navigationResponse:(WKNavigationResponse *)navigationResponse didBecomeDownload:(WKDownload *)download API_AVAILABLE(macos(11.3), ios(14.5));
{
  download.delegate = self;
}


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
    bool allowMimeType = mWebView->OnCanDownloadMIMEType([navigationResponse.response.MIMEType UTF8String]);

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

- (void)download:(WKDownload *)download decideDestinationUsingResponse:(NSURLResponse *)response suggestedFilename:(NSString *)filename completionHandler:(void (^)(NSURL * _Nullable))completionHandler API_AVAILABLE(macos(11.3), ios(14.5))
{
  WDL_String localPath;
  mWebView->GetLocalDownloadPathForFile([filename UTF8String], localPath);
  downloadDestinationURL = [NSURL URLWithString:[NSString stringWithUTF8String:localPath.Get()]];
  completionHandler(downloadDestinationURL);
}

- (void)download:(WKDownload *)download didReceiveData:(int64_t)length API_AVAILABLE(macos(11.3), ios(14.5))
{
  mWebView->OnReceivedData(length, 0);
}

- (void)download:(WKDownload *)download didFailWithError:(NSError *)error API_AVAILABLE(macos(11.3), ios(14.5))
{
  mWebView->FailedToDownloadFile([[downloadDestinationURL absoluteString] UTF8String]);
  downloadDestinationURL = nil;
}

- (void)downloadDidFinish:(WKDownload *)download  API_AVAILABLE(macos(11.3), ios(14.5))
{
  mWebView->OnDownloadedFile([[downloadDestinationURL path] UTF8String]);
  downloadDestinationURL = nil;
}

@end
