#import "IPlugWKWebViewUIDelegate.h"
#include "IPlugWebView.h"

#if !__has_feature(objc_arc)
#error This file must be compiled with Arc. Use -fobjc-arc flag
#endif

using namespace iplug;

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

- (void)webView:(WKWebView *)webView runOpenPanelWithParameters:(WKOpenPanelParameters *)parameters 
  initiatedByFrame:(WKFrameInfo *)frame completionHandler:(void (^)(NSArray<NSURL *> *URLs))completionHandler
{
#ifdef OS_MAC
  NSOpenPanel* openPanel = [NSOpenPanel openPanel];
  openPanel.allowsMultipleSelection = parameters.allowsMultipleSelection;
  
  [openPanel setCanChooseFiles:YES];
  [openPanel setCanChooseDirectories:NO];
  [openPanel setAllowsMultipleSelection:parameters.allowsMultipleSelection];
  
  [openPanel beginWithCompletionHandler:^(NSInteger result) {
    if (result == NSModalResponseOK) {
      completionHandler(openPanel.URLs);
    } else {
      completionHandler(nil);
    }
  }];
#else
  UIAlertController* alert = [UIAlertController alertControllerWithTitle:@"Not Implemented" 
                                                                message:@"File picking is not implemented on iOS" 
                                                         preferredStyle:UIAlertControllerStyleAlert];
  
  UIAlertAction* okAction = [UIAlertAction actionWithTitle:@"OK" 
                                                    style:UIAlertActionStyleDefault
                                                  handler:^(UIAlertAction * action) {
    completionHandler(nil);
  }];
  
  [alert addAction:okAction];
  [webView.window.rootViewController presentViewController:alert animated:YES completion:nil];


#endif
}

#ifdef OS_IOS
- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls
{
  if (self.filePickerCompletionHandler) {
    self.filePickerCompletionHandler(urls);
    self.filePickerCompletionHandler = nil;
  }
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller
{
  if (self.filePickerCompletionHandler) {
    self.filePickerCompletionHandler(nil);
    self.filePickerCompletionHandler = nil;
  }
}
#endif

@end
