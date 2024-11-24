 /*
 ==============================================================================
 
  MIT License

  iPlug2 WebView Library
  Copyright (c) 2024 Oliver Larkin

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
 
 ==============================================================================
*/

#import "IPlugWKWebViewDelegate.h"

#include "IPlugWebView.h"

#if !__has_feature(objc_arc)
#error This file must be compiled with Arc. Use -fobjc-arc flag
#endif

using namespace iplug;

@implementation IPLUG_WKWEBVIEW_DELEGATE

NS_ASSUME_NONNULL_BEGIN

- (id)initWithIWebView:(iplug::IWebView*)webView;
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

- (void)webView:(WKWebView*)webView navigationResponse:(WKNavigationResponse*)navigationResponse didBecomeDownload:(WKDownload*)download API_AVAILABLE(macos(11.3), ios(14.5))
{
  download.delegate = self;
}

- (void)webView:(WKWebView*)webView decidePolicyForNavigationAction:(WKNavigationAction*)navigationAction decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler
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

- (void)webView:(WKWebView*)webView decidePolicyForNavigationResponse:(WKNavigationResponse*)navigationResponse decisionHandler:(void (^)(WKNavigationResponsePolicy))decisionHandler
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

- (void) webView:(IPLUG_WKWEBVIEW*) webView didFinishNavigation:(WKNavigation*) navigation
{
  mIWebView->OnWebContentLoaded();
}

- (void)download:(WKDownload*)download decideDestinationUsingResponse:(NSURLResponse*)response suggestedFilename:(NSString*)filename completionHandler:(void (^_Nonnull)(NSURL* _Nullable))completionHandler API_AVAILABLE(macos(11.3), ios(14.5))
{
  WDL_String localPath;
  mIWebView->OnGetLocalDownloadPathForFile([filename UTF8String], localPath);
  downloadDestinationURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:localPath.Get()]];
  completionHandler(downloadDestinationURL);
}

- (void)download:(WKDownload*)download didReceiveData:(int64_t)length API_AVAILABLE(macos(11.3), ios(14.5))
{
  mIWebView->OnReceivedData(length, 0);
}

- (void)download:(WKDownload*)download didFailWithError:(NSError*)error API_AVAILABLE(macos(11.3), ios(14.5))
{
  mIWebView->OnFailedToDownloadFile([[downloadDestinationURL absoluteString] UTF8String]);
  downloadDestinationURL = nil;
}

- (void)downloadDidFinish:(WKDownload*)download API_AVAILABLE(macos(11.3), ios(14.5))
{
  mIWebView->OnDownloadedFile([[downloadDestinationURL path] UTF8String]);
  downloadDestinationURL = nil;
}

NS_ASSUME_NONNULL_END

@end
