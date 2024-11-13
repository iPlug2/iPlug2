 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

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
#endif
}

@end
