 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#import <WebKit/WebKit.h>

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