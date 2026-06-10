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

@class IPLUG_WKWEBVIEW;

@interface IPLUG_WKSCRIPTMESSAGEHANDLER : NSObject <WKScriptMessageHandler, WKURLSchemeHandler>
NS_ASSUME_NONNULL_BEGIN
{
  iplug::IWebView* mIWebView;
  NSURL* _Nullable downloadDestinationURL;
}

- (void)dealloc;

- (id)initWithIWebView:(iplug::IWebView*)webView;

- (void)userContentController:(WKUserContentController*)userContentController 
      didReceiveScriptMessage:(WKScriptMessage*)message;

#pragma mark - WKURLSchemeHandler

- (NSURL* _Nullable)changeURLScheme:(NSURL*)url toScheme:(NSString*)newScheme;

- (void)webView:(WKWebView*)webView 
startURLSchemeTask:(id<WKURLSchemeTask>)urlSchemeTask API_AVAILABLE(macos(10.13));

- (void)webView:(WKWebView*)webView 
stopURLSchemeTask:(id<WKURLSchemeTask>)urlSchemeTask API_AVAILABLE(macos(10.13));

NS_ASSUME_NONNULL_END
@end
