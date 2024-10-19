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
    mWebView = webView;
  
  return self;
}

- (void)dealloc
{
  mWebView = nullptr;
}

- (void) userContentController:(nonnull WKUserContentController*) userContentController didReceiveScriptMessage:(nonnull WKScriptMessage*) message
{
  if ([[message name] isEqualToString:@"callback"])
  {
    NSDictionary* dict = (NSDictionary*) message.body;
    NSData* data = [NSJSONSerialization dataWithJSONObject:dict options:NSJSONWritingPrettyPrinted error:nil];
    NSString* jsonString = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    mWebView->OnMessageFromWebView([jsonString UTF8String]);
  }
}

- (void) webView:(IPLUG_WKWEBVIEW*) webView didFinishNavigation:(WKNavigation*) navigation
{
  mWebView->OnWebContentLoaded();
}

- (NSURL*) changeURLScheme:(NSURL*) url toScheme:(NSString*) newScheme
{
  NSURLComponents* components = [NSURLComponents componentsWithURL:url resolvingAgainstBaseURL:YES];
  components.scheme = newScheme;
  return components.URL;
}

- (void) webView:(nonnull WKWebView *)webView startURLSchemeTask:(nonnull id<WKURLSchemeTask>)urlSchemeTask  API_AVAILABLE(macos(10.13)){

  if ([urlSchemeTask.request.URL.absoluteString containsString:@"iplug2:"])
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

- (void) webView:(nonnull WKWebView *)webView stopURLSchemeTask:(nonnull id<WKURLSchemeTask>)urlSchemeTask  API_AVAILABLE(macos(10.13)){
  
}

@end
