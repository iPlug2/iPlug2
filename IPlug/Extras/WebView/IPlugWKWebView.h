 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#import <WebKit/WebKit.h>

@interface IPLUG_WKWEBVIEW : WKWebView
{
  bool mEnableInteraction;
}

- (instancetype)initWithFrame:(CGRect)frame configuration:(WKWebViewConfiguration *)configuration;
- (void)setEnableInteraction:(bool)enable;

#ifdef OS_MAC
- (NSView *)hitTest:(NSPoint)point;
- (void)willOpenMenu:(NSMenu *)menu withEvent:(NSEvent *)event;
#endif

@end

