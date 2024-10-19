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

