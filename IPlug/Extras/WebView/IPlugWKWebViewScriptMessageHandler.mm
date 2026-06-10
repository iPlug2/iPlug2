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

#import "IPlugWKWebViewScriptMessageHandler.h"
#include "IPlugWebView.h"

#if !__has_feature(objc_arc)
#error This file must be compiled with Arc. Use -fobjc-arc flag
#endif

using namespace iplug;

@implementation IPLUG_WKSCRIPTMESSAGEHANDLER

-(id) initWithIWebView:(IWebView*) webView
{
  self = [super init];
  
  if (self)
    mIWebView = webView;
  
  return self;
}

- (void)dealloc
{
  mIWebView = nullptr;
}

- (void) userContentController:(WKUserContentController*) userContentController didReceiveScriptMessage:(WKScriptMessage*) message
{
  if ([[message name] isEqualToString:@"callback"])
  {
    NSDictionary* dict = (NSDictionary*) message.body;
    NSData* data = [NSJSONSerialization dataWithJSONObject:dict options:NSJSONWritingPrettyPrinted error:nil];
    NSString* jsonString = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    mIWebView->OnMessageFromWebView([jsonString UTF8String]);
  }
}

- (NSURL*) changeURLScheme:(NSURL*) url toScheme:(NSString*) newScheme
{
  NSURLComponents* components = [NSURLComponents componentsWithURL:url resolvingAgainstBaseURL:YES];
  components.scheme = newScheme;
  return components.URL;
}

- (void) webView:(WKWebView *)webView startURLSchemeTask:(id<WKURLSchemeTask>)urlSchemeTask  API_AVAILABLE(macos(10.13)){

  NSString* customUrlScheme = [NSString stringWithUTF8String:mIWebView->GetCustomUrlScheme()];
  const BOOL useCustomUrlScheme = [customUrlScheme length];
  
  NSString* urlScheme = @"file:";
  
  if (useCustomUrlScheme)
  {
    urlScheme = [urlScheme stringByReplacingOccurrencesOfString:@"file" withString:customUrlScheme];
  }
  
  if (useCustomUrlScheme && [urlSchemeTask.request.URL.absoluteString containsString: urlScheme])
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
      else if ([extStr isEqualToString:@"json"]) contentTypeStr = @"text/json";

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

- (void) webView:(WKWebView *)webView stopURLSchemeTask:(id<WKURLSchemeTask>)urlSchemeTask  API_AVAILABLE(macos(10.13)){
  
}

@end
