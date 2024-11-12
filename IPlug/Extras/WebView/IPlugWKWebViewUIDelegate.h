#pragma once

#ifdef __OBJC__
#import <WebKit/WebKit.h>
#if defined __APPLE__
  #include "TargetConditionals.h"
  #if TARGET_OS_IOS
    #import <UIKit/UIKit.h>
  #endif
#endif

namespace iplug {
class IWebView;
}

@interface IPLUG_WKWEBVIEW_UI_DELEGATE : NSObject <WKUIDelegate>
{
  iplug::IWebView* _Nonnull mIWebView;
}

- (id _Nonnull)initWithIWebView:(iplug::IWebView* _Nonnull)webView;

- (void)webView:(WKWebView *_Nonnull)webView runOpenPanelWithParameters:(WKOpenPanelParameters *_Nullable)parameters 
  initiatedByFrame:(WKFrameInfo *_Nonnull)frame completionHandler:(void (^_Nullable)(NSArray<NSURL *> * _Nullable URLs))completionHandler;

@end

#if TARGET_OS_IOS
@interface IPLUG_WKWEBVIEW_UI_DELEGATE () <UIDocumentPickerDelegate>
@property (nonatomic, copy) void (^ _Nullable filePickerCompletionHandler)(NSArray<NSURL *> * _Nullable URLs);
@end
#endif

#endif // __OBJC__
