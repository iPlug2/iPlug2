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

#import "IPlugWKWebViewUIDelegate.h"
#include "IPlugWebView.h"

#if !__has_feature(objc_arc)
#error This file must be compiled with Arc. Use -fobjc-arc flag
#endif

using namespace iplug;

#ifdef OS_IOS
@interface IPLUG_WKWEBVIEW_UI_DELEGATE () <UIDocumentPickerDelegate>
{
  void (^mOpenPanelCompletionHandler)(NSArray<NSURL*>* _Nullable URLs);
}
@end
#endif

NS_ASSUME_NONNULL_BEGIN

@implementation IPLUG_WKWEBVIEW_UI_DELEGATE

- (id)initWithIWebView:(IWebView*)webView
{
  self = [super init];
  if (self)
  {
    mIWebView = webView;
  }
  return self;
}

- (void)webView:(WKWebView *)webView runOpenPanelWithParameters:(WKOpenPanelParameters *)parameters initiatedByFrame:(WKFrameInfo *)frame completionHandler:(void (^)(NSArray<NSURL *> *URLs))completionHandler
{
#if defined(OS_MAC)
  NSOpenPanel* openPanel = [NSOpenPanel openPanel];
  openPanel.allowsMultipleSelection = parameters.allowsMultipleSelection;

  [openPanel setCanChooseFiles:YES];
  [openPanel setCanChooseDirectories:NO];
  [openPanel setAllowsMultipleSelection:parameters.allowsMultipleSelection];

  [openPanel beginSheetModalForWindow:webView.window completionHandler:^(NSInteger result) {
    if (result == NSModalResponseOK) {
      completionHandler(openPanel.URLs);
    } else {
      completionHandler(nil);
    }
  }];
#elif defined(OS_IOS)
  mOpenPanelCompletionHandler = [completionHandler copy];

  NSArray<UTType*>* contentTypes = @[UTTypeItem];
  UIDocumentPickerViewController* picker = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:contentTypes];
  picker.allowsMultipleSelection = parameters.allowsMultipleSelection;
  picker.delegate = self;

  // Find the view controller to present from
  UIResponder* responder = webView;
  while (responder && ![responder isKindOfClass:[UIViewController class]]) {
    responder = [responder nextResponder];
  }

  UIViewController* viewController = (UIViewController*)responder;
  if (viewController) {
    [viewController presentViewController:picker animated:YES completion:nil];
  } else {
    mOpenPanelCompletionHandler(nil);
    mOpenPanelCompletionHandler = nil;
  }
#endif
}

#ifdef OS_IOS
- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls
{
  if (mOpenPanelCompletionHandler) {
    mOpenPanelCompletionHandler(urls);
    mOpenPanelCompletionHandler = nil;
  }
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller
{
  if (mOpenPanelCompletionHandler) {
    mOpenPanelCompletionHandler(nil);
    mOpenPanelCompletionHandler = nil;
  }
}
#endif

- (nullable WKWebView *)webView:(WKWebView *)webView createWebViewWithConfiguration:(WKWebViewConfiguration *)configuration forNavigationAction:(WKNavigationAction *)navigationAction windowFeatures:(WKWindowFeatures *)windowFeatures
{
  // Open URLs in the browser
#if defined OS_MAC
  [[NSWorkspace sharedWorkspace] openURL:[navigationAction.request URL]];
#elif defined OS_IOS
  UIResponder* pResponder = (UIResponder*) webView;
  while(pResponder) {
    if ([pResponder respondsToSelector: @selector(openURL:)])
    {
      [pResponder performSelector: @selector(openURL:) withObject: [navigationAction.request URL]];
    }
    
    pResponder = [pResponder nextResponder];
  }
#endif
  return nil;
}

@end

NS_ASSUME_NONNULL_END
