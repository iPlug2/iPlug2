/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#ifndef NO_IGRAPHICS

#import <UIKit/UIKit.h>
#include "IGraphicsIOS.h"

inline CGRect ToCGRect(IGraphics* pGraphics, const IRECT& bounds)
{
  float B = (pGraphics->Height() - bounds.B);
  return CGRectMake(bounds.L, B, bounds.W(), bounds.H());
}

@interface IGraphicsIOS_View : UIView
{  
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
- (void) getTouchXY: (CGPoint) pt x: (float*) pX y: (float*) pY;
@property (readonly) CAMetalLayer* metalLayer;
@property (nonatomic, strong) CADisplayLink *displayLink;

@end

#endif //NO_IGRAPHICS
