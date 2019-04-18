/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

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
  IGraphicsIOS* mGraphics;
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

#ifdef IGRAPHICS_IMGUI
#import <MetalKit/MetalKit.h>

@interface IGRAPHICS_IMGUIVIEW : MTKView
{
  IGraphicsIOS_View* mView;
}
@property (nonatomic, strong) id <MTLCommandQueue> commandQueue;
- (id) initWithIGraphicsView: (IGraphicsIOS_View*) pView;
@end
#endif
