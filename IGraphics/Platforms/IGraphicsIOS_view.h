#ifndef NO_IGRAPHICS

#import <UIKit/UIKit.h>
#include "IGraphicsIOS.h"

@interface IGraphicsIOS_View : UIView
{
#if DISPLAY_LINK
  CADisplayLink* mDisplayLink;
#else
  NSTimer* mTimer;
#endif
@public
  IGraphicsIOS* mGraphics; // OBJC instance variables have to be pointers
}
- (id) initWithIGraphics: (IGraphicsIOS*) pGraphics;
- (BOOL) isOpaque;
- (BOOL) acceptsFirstResponder;
- (void) removeFromSuperview;
- (void) controlTextDidEndEditing: (NSNotification*) aNotification;
- (IPopupMenu*) createPopupMenu: (const IPopupMenu&) menu : (CGRect) bounds;
- (void) createTextEntry: (IControl&) control : (const IText&) text : (const char*) str : (CGRect) areaRect;
- (void) endUserInput;
- (void) onTimer: (NSTimer*) pTimer;
@end

#endif //NO_IGRAPHICS
