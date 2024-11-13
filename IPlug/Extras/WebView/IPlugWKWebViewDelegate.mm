 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#import "IPlugWKWebViewDelegate.h"

#include "IPlugWebView.h"

#if !__has_feature(objc_arc)
#error This file must be compiled with Arc. Use -fobjc-arc flag
#endif

using namespace iplug;

@implementation IPLUG_WKWEBVIEW_DELEGATE

- (id _Nonnull)initWithIWebView:(iplug::IWebView* _Nonnull)webView;
{
  self = [super init];
  if (self)
  {
    mIWebView = webView;
  }
  return self;
}

- (void)dealloc
{
  mIWebView = nullptr;
}

- (void)webView:(WKWebView * _Nonnull)webView 
navigationResponse:(WKNavigationResponse * _Nonnull)navigationResponse 
didBecomeDownload:(WKDownload * _Nonnull)download API_AVAILABLE(macos(11.3), ios(14.5))
{
  download.delegate = self;
}

- (void)webView:(WKWebView * _Nonnull)webView 
decidePolicyForNavigationAction:(WKNavigationAction * _Nonnull)navigationAction 
decisionHandler:(void (^ _Nonnull)(WKNavigationActionPolicy))decisionHandler
{
  bool allow = mIWebView->OnCanNavigateToURL([[[[navigationAction request] URL] absoluteString] UTF8String]);
  
  if (allow)
  {
    decisionHandler(WKNavigationActionPolicyAllow);
  }
  else
  {
    decisionHandler(WKNavigationActionPolicyCancel);
  }
}

- (void)webView:(WKWebView * _Nonnull)webView 
decidePolicyForNavigationResponse:(WKNavigationResponse * _Nonnull)navigationResponse 
decisionHandler:(void (^ _Nonnull)(WKNavigationResponsePolicy))decisionHandler
{
  bool allow = mIWebView->OnCanNavigateToURL([[[[navigationResponse response] URL] absoluteString] UTF8String]);
  
  if (!allow)
  {
    decisionHandler(WKNavigationResponsePolicyCancel);
  }
  
  if (@available(macOS 11.3, iOS 14.5, *))
  {
    bool allowMimeType = mIWebView->OnCanDownloadMIMEType([navigationResponse.response.MIMEType UTF8String]);

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

- (void)download:(WKDownload * _Nonnull)download 
decideDestinationUsingResponse:(NSURLResponse * _Nonnull)response 
suggestedFilename:(NSString * _Nonnull)filename 
completionHandler:(void (^ _Nonnull)(NSURL * _Nullable))completionHandler API_AVAILABLE(macos(11.3), ios(14.5))
{
  WDL_String localPath;
  mIWebView->OnGetLocalDownloadPathForFile([filename UTF8String], localPath);
  downloadDestinationURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:localPath.Get()]];
  completionHandler(downloadDestinationURL);
}

- (void)download:(WKDownload * _Nonnull)download 
didReceiveData:(int64_t)length API_AVAILABLE(macos(11.3), ios(14.5))
{
  mIWebView->OnReceivedData(length, 0);
}

- (void)download:(WKDownload * _Nonnull)download 
didFailWithError:(NSError * _Nonnull)error API_AVAILABLE(macos(11.3), ios(14.5))
{
  mIWebView->OnFailedToDownloadFile([[downloadDestinationURL absoluteString] UTF8String]);
  downloadDestinationURL = nil;
}

- (void)downloadDidFinish:(WKDownload * _Nonnull)download API_AVAILABLE(macos(11.3), ios(14.5))
{
  mIWebView->OnDownloadedFile([[downloadDestinationURL path] UTF8String]);
  downloadDestinationURL = nil;
}

@end
