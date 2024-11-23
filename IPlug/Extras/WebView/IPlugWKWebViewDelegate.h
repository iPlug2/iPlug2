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

- (void)webView:(WKWebView * _Nonnull)webView decidePolicyForNavigationAction:(WKNavigationAction * _Nonnull)navigationAction decisionHandler:(void (^ _Nonnull)(WKNavigationActionPolicy))decisionHandler;

- (void)webView:(WKWebView * _Nonnull)webView decidePolicyForNavigationResponse:(WKNavigationResponse * _Nonnull)navigationResponse decisionHandler:(void (^ _Nonnull)(WKNavigationResponsePolicy))decisionHandler;

- (void)webView:(IPLUG_WKWEBVIEW* _Nonnull)webView didFinishNavigation:(null_unspecified WKNavigation*)navigation;

#pragma mark WKDownloadDelegate

- (void)webView:(WKWebView * _Nonnull)webView navigationResponse:(WKNavigationResponse * _Nonnull)navigationResponse didBecomeDownload:(WKDownload * _Nonnull)download API_AVAILABLE(macos(11.3), ios(14.5));

- (void)download:(WKDownload * _Nonnull)download decideDestinationUsingResponse:(NSURLResponse * _Nonnull)response suggestedFilename:(NSString * _Nonnull)filename completionHandler:(void (^ _Nonnull)(NSURL * _Nullable))completionHandler API_AVAILABLE(macos(11.3), ios(14.5));

- (void)download:(WKDownload * _Nonnull)download didReceiveData:(int64_t)length API_AVAILABLE(macos(11.3), ios(14.5));

- (void)download:(WKDownload * _Nonnull)download didFailWithError:(NSError * _Nonnull)error API_AVAILABLE(macos(11.3), ios(14.5));

- (void)downloadDidFinish:(WKDownload * _Nonnull)download API_AVAILABLE(macos(11.3), ios(14.5));

@end
