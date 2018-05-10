#ifndef NO_IGRAPHICS

#import <UIKit/UIKit.h>
#include "IGraphicsIOS.h"

@interface IGraphicsIOS_View : UIView
{
  NSTimer* mTimer;
@public
  IGraphicsIOS* mGraphics; // OBJC instance variables have to be pointers
}
- (id) initWithIGraphics: (IGraphicsIOS*) pGraphics;
- (BOOL) isOpaque;
- (BOOL) acceptsFirstResponder;
- (void) onTimer: (NSTimer*) pTimer;
- (void) killTimer;
- (void) removeFromSuperview;
- (void) controlTextDidEndEditing: (NSNotification*) aNotification;
- (IPopupMenu*) createPopupMenu: (const IPopupMenu&) menu : (CGRect) bounds;
- (void) createTextEntry: (IControl&) control : (const IText&) text : (const char*) str : (CGRect) areaRect;
- (void) endUserInput;
@end

#endif //NO_IGRAPHICS
