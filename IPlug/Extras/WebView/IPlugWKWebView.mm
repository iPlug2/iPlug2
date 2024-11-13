#import "IPlugWKWebView.h"

#include "IPlugPlatform.h"

@implementation IPLUG_WKWEBVIEW

- (instancetype)initWithFrame:(CGRect)frame configuration:(WKWebViewConfiguration *)configuration
{
  self = [super initWithFrame:frame configuration:configuration];
  
  if (self)
  {
    mEnableInteraction = true;
  }
  return self;
}

#ifdef OS_MAC
- (NSView *)hitTest:(NSPoint)point
{
  if (!mEnableInteraction)
  {
    return nil;
  }
  else
    return [super hitTest:point];
}

- (void)willOpenMenu:(NSMenu *)menu withEvent:(NSEvent *)event
{
  [super willOpenMenu:menu withEvent:event];
  
  NSArray<NSString *> *WKStrings = @[
   @"WKMenuItemIdentifierCopy",
   @"WKMenuItemIdentifierCopyImage",
   @"WKMenuItemIdentifierCopyLink",
   @"WKMenuItemIdentifierDownloadImage",
   @"WKMenuItemIdentifierDownloadLinkedFile",
   @"WKMenuItemIdentifierGoBack",
   @"WKMenuItemIdentifierGoForward",
//   @"WKMenuItemIdentifierInspectElement",
   @"WKMenuItemIdentifierLookUp",
   @"WKMenuItemIdentifierOpenFrameInNewWindow",
   @"WKMenuItemIdentifierOpenImageInNewWindow",
   @"WKMenuItemIdentifierOpenLink",
   @"WKMenuItemIdentifierOpenLinkInNewWindow",
   @"WKMenuItemIdentifierPaste",
//   @"WKMenuItemIdentifierReload",
   @"WKMenuItemIdentifierSearchWeb",
   @"WKMenuItemIdentifierShowHideMediaControls",
   @"WKMenuItemIdentifierToggleFullScreen",
   @"WKMenuItemIdentifierTranslate",
   @"WKMenuItemIdentifierShareMenu",
   @"WKMenuItemIdentifierSpeechMenu"
  ];
  
  for (NSInteger itemIndex = 0; itemIndex < menu.itemArray.count; itemIndex++)
  {
    if ([WKStrings containsObject:menu.itemArray[itemIndex].identifier])
    {
      [menu removeItemAtIndex:itemIndex];
    }
  }
}

#endif

- (void)setEnableInteraction:(bool)enable
{
  mEnableInteraction = enable;
  
#ifdef OS_MAC
  if (!mEnableInteraction)
  {
    for (NSTrackingArea* trackingArea in self.trackingAreas)
    {
      [self removeTrackingArea:trackingArea];
    }
  }
#else
  self.userInteractionEnabled = mEnableInteraction;
#endif
}

@end
