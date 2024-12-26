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

- (BOOL)allowsLinkPreview
{
  return false;
}

@end
