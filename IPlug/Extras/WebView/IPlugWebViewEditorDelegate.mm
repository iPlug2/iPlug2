#include "IPlugWebViewEditorDelegate.h"
#import <WebKit/WebKit.h>
#include "config.h"
#include "IPlugPluginBase.h"
#include "IPlugPaths.h"
#include "wdl_base64.h"

#if defined OS_MAC
  #define VIEW NSView
  #define MAKERECT NSMakeRect
#elif defined OS_IOS
  #define VIEW UIView
  #define MAKERECT CGRectMake
#endif

BEGIN_IPLUG_NAMESPACE
extern bool GetResourcePathFromBundle(const char* fileName, const char* searchExt, WDL_String& fullPath, const char* bundleID);
END_IPLUG_NAMESPACE

using namespace iplug;

@interface ScriptHandler : NSObject <WKScriptMessageHandler, WKNavigationDelegate>
{
  WebViewEditorDelegate* mWebViewEditorDelegate;
}
@end

@implementation ScriptHandler

-(id) initWithWebViewEditorDelegate:(WebViewEditorDelegate*) webViewEditorDelegate
{
  self = [super init];
  
  if(self)
    mWebViewEditorDelegate = webViewEditorDelegate;
  
  return self;
}

- (void)userContentController:(nonnull WKUserContentController *)userContentController didReceiveScriptMessage:(nonnull WKScriptMessage *)message
{
  if([[message name] isEqualToString:@"callback"])
  {
    NSDictionary* data = (NSDictionary*) message.body;
    
    if([data[@"msg"] isEqualToString:@"SPVFUI"])
    {
      long paramIdx = [data[@"paramIdx"] integerValue];
      double value = [data[@"value"] doubleValue];

      mWebViewEditorDelegate->SendParameterValueFromUI(static_cast<int>(paramIdx), value);
    }
    else if ([data[@"msg"] isEqualToString:@"BPCFUI"])
    {
      long paramIdx = [data[@"paramIdx"] integerValue];
      mWebViewEditorDelegate->BeginInformHostOfParamChangeFromUI(static_cast<int>(paramIdx));
    }
    else if ([data[@"msg"] isEqualToString:@"EPCFUI"])
    {
      long paramIdx = [data[@"paramIdx"] integerValue];
      mWebViewEditorDelegate->EndInformHostOfParamChangeFromUI(static_cast<int>(paramIdx));
    }
    else if ([data[@"msg"] isEqualToString:@"SAMFUI"])
    {
      long msgTag = [data[@"msgTag"] integerValue];
      long ctrlTag = [data[@"ctrlTag"] integerValue];
      long dataSize = [data[@"dataSize"] integerValue];

      mWebViewEditorDelegate->SendArbitraryMsgFromUI(static_cast<int>(msgTag), static_cast<int>(ctrlTag), static_cast<int>(dataSize), dataSize > 0 ? [data[@"data"] bytes] : nullptr);
    }
  }
}

- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation
{
  mWebViewEditorDelegate->OnWebContentLoaded();
}

@end

WebViewEditorDelegate::WebViewEditorDelegate(int nParams)
: IEditorDelegate(nParams)
{
}

WebViewEditorDelegate::~WebViewEditorDelegate()
{
}

void* WebViewEditorDelegate::OpenWindow(void* pParent)
{
  VIEW* parentView = (VIEW*) pParent;
  
  WKWebViewConfiguration* webConfig = [[WKWebViewConfiguration alloc] init];
  WKPreferences* preferences = [[[WKPreferences alloc] init] autorelease];
  
  WKUserContentController* controller = [[[WKUserContentController alloc] init] autorelease];
  webConfig.userContentController = controller;

  ScriptHandler* scriptHandler = [[ScriptHandler alloc] initWithWebViewEditorDelegate: this];
  [controller addScriptMessageHandler: scriptHandler name:@"callback"];
  [preferences setValue:@YES forKey:@"developerExtrasEnabled"];
  webConfig.preferences = preferences;
  
  WKWebView* webView = [[WKWebView alloc] initWithFrame: MAKERECT(0.f, 0.f, PLUG_WIDTH, PLUG_HEIGHT) configuration:webConfig];
  
#if defined OS_IOS
  [webView.scrollView setScrollEnabled:NO];
#endif

#ifdef OS_MAC
  [webView setAllowsMagnification:NO];
#endif
  
  [webView setNavigationDelegate:scriptHandler];
  
  [parentView addSubview:webView];
  
//#ifdef OS_MAC
//  [webView setAutoresizingMask: NSViewHeightSizable|NSViewWidthSizable|NSViewMinXMargin|NSViewMaxXMargin|NSViewMinYMargin|NSViewMaxYMargin ];
//#else
//  [webView setAutoresizingMask: UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleBottomMargin | UIViewAutoresizingFlexibleTopMargin];
//#endif
//  [parentView setAutoresizesSubviews:YES];
  
  mWebConfig = webConfig;
  mWKWebView = webView;
  mScriptHandler = scriptHandler;
  
  if(mEditorInitFunc)
    mEditorInitFunc();
  
  return webView;
}

void WebViewEditorDelegate::CloseWindow()
{
  [(WKWebViewConfiguration*) mWebConfig release];
  [(WKWebView*) mWKWebView release];
  [(ScriptHandler*) mScriptHandler release];
}

void WebViewEditorDelegate::SendControlValueFromDelegate(int ctrlTag, double normalizedValue)
{
  WDL_String str;
  str.SetFormatted(50, "SCVFD(%i, %f)", ctrlTag, normalizedValue);
  EvaluateJavaScript(str.Get());
}

void WebViewEditorDelegate::SendControlMsgFromDelegate(int ctrlTag, int msgTag, int dataSize, const void* pData)
{
  WDL_String str;
  WDL_TypedBuf<char> base64;
  int sizeOfBase64 = 4 * std::ceil(((double) dataSize/3.));
  base64.Resize(sizeOfBase64);
  wdl_base64encode(reinterpret_cast<const unsigned char*>(pData), base64.GetFast(), dataSize);
  str.SetFormatted(50, "SCMFD(%i, %i, %i, %s)", ctrlTag, msgTag, dataSize, base64.GetFast());
  EvaluateJavaScript(str.Get());
}

void WebViewEditorDelegate::SendParameterValueFromDelegate(int paramIdx, double value, bool normalized)
{
  WDL_String str;
  str.SetFormatted(50, "SPVFD(%i, %f)", paramIdx, value);
  EvaluateJavaScript(str.Get());
}

void WebViewEditorDelegate::SendArbitraryMsgFromDelegate(int msgTag, int dataSize, const void* pData)
{
  WDL_String str;
  WDL_TypedBuf<char> base64;
  int sizeOfBase64 = 4 * std::ceil(((double) dataSize/3.));
  base64.Resize(sizeOfBase64);
  wdl_base64encode(reinterpret_cast<const unsigned char*>(pData), base64.GetFast(), dataSize);
  str.SetFormatted(50, "SAMFD(%i, %i, %s)", msgTag, dataSize, base64.GetFast());
  EvaluateJavaScript(str.Get());
}

void WebViewEditorDelegate::LoadHTML(const WDL_String& html)
{
  WKWebView* webView = (WKWebView*) mWKWebView;
  [webView loadHTMLString:[NSString stringWithUTF8String:html.Get()] baseURL:nil];
}

void WebViewEditorDelegate::LoadURL(const char* url)
{
  WKWebView* webView = (WKWebView*) mWKWebView;
  
  NSURL* nsurl = [NSURL URLWithString:[NSString stringWithUTF8String:url] relativeToURL:nil];
  NSURLRequest* req = [[[NSURLRequest alloc] initWithURL:nsurl] autorelease];
  [webView loadRequest:req];
}

void WebViewEditorDelegate::LoadFileFromBundle(const char* fileName)
{
  IPluginBase* pPlug = dynamic_cast<IPluginBase*>(this);
  WKWebView* webView = (WKWebView*) mWKWebView;

  WDL_String fullPath;
  WDL_String fileNameWeb("web/");
  fileNameWeb.Append(fileName);
  
  GetResourcePathFromBundle(fileNameWeb.Get(), "html", fullPath, pPlug->GetBundleID());
  
  NSString* pPath = [NSString stringWithUTF8String:fullPath.Get()];
  
  NSString* str = @"file:";
  NSString* webroot = [str stringByAppendingString:[pPath stringByReplacingOccurrencesOfString:[NSString stringWithUTF8String:fileName] withString:@""]];
  NSURL* pageUrl = [NSURL URLWithString:[webroot stringByAppendingString:[NSString stringWithUTF8String:fileName]] relativeToURL:nil];
  NSURL* rootUrl = [NSURL URLWithString:webroot relativeToURL:nil];

  [webView loadFileURL:pageUrl allowingReadAccessToURL:rootUrl];
}

void WebViewEditorDelegate::EvaluateJavaScript(const char* scriptStr)
{
  WKWebView* webView = (WKWebView*) mWKWebView;
  
  if (![webView isLoading]) {
    [webView evaluateJavaScript:[NSString stringWithUTF8String:scriptStr] completionHandler:^(NSString *result, NSError *error)
     {
       if(error != nil)
         NSLog(@"Error %@",error);
     }];
  }
}

void WebViewEditorDelegate::EnableScroll(bool enable)
{
#ifdef OS_IOS
  WKWebView* webView = (WKWebView*) mWKWebView;
  [webView.scrollView setScrollEnabled:enable];
#endif
}

void WebViewEditorDelegate::Resize(int width, int height)
{
//  [NSAnimationContext beginGrouping]; // Prevent animated resizing
//  [[NSAnimationContext currentContext] setDuration:0.0];
  [(WKWebView*) mWKWebView setFrame: MAKERECT(0.f, 0.f, (float) width, (float) height) ];
//  [NSAnimationContext endGrouping];
  EditorResizeFromUI(width, height);
}
