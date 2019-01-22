/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#ifndef NO_IGRAPHICS

#import "IGraphicsIOS_view.h"
#include "IControl.h"
#include "IPlugParameter.h"

@implementation IGraphicsIOS_View

- (id) initWithIGraphics: (IGraphicsIOS*) pGraphics
{
  TRACE;

  mGraphics = pGraphics;
  CGRect r;
  r.origin.x = r.origin.y = 0.0f;
  r.size.width = (float) pGraphics->WindowWidth();
  r.size.height = (float) pGraphics->WindowHeight();
  self = [super initWithFrame:r];

  self.layer.opaque = YES;
  self.layer.contentsScale = [UIScreen mainScreen].scale;
  
//  self.multipleTouchEnabled = YES;
  
  return self;
}

- (void)setFrame:(CGRect)frame
{
  [super setFrame:frame];
  
  // During the first layout pass, we will not be in a view hierarchy, so we guess our scale
  CGFloat scale = [UIScreen mainScreen].scale;
  
  // If we've moved to a window by the time our frame is being set, we can take its scale as our own
  if (self.window)
    scale = self.window.screen.scale;
  
  CGSize drawableSize = self.bounds.size;
  
  // Since drawable size is in pixels, we need to multiply by the scale to move from points to pixels
  drawableSize.width *= scale;
  drawableSize.height *= scale;
  
  self.metalLayer.drawableSize = drawableSize;
}

- (void) getTouchXY: (CGPoint) pt x: (float*) pX y: (float*) pY
{
  if (mGraphics)
  {
    *pX = pt.x / mGraphics->GetDrawScale();
    *pY = pt.y / mGraphics->GetDrawScale();
  }
}

- (void) touchesBegan: (NSSet*) pTouches withEvent: (UIEvent*) pEvent
{
  UITouch* pTouch = [pTouches anyObject];
  CGPoint pt = [pTouch locationInView: self];

  IMouseInfo info;
  info.ms.L = true;
  [self getTouchXY:pt x:&info.x y:&info.y];
  mGraphics->OnMouseDown(info.x, info.y, info.ms);
}

- (void) touchesMoved: (NSSet*) pTouches withEvent: (UIEvent*) pEvent
{
  UITouch* pTouch = [pTouches anyObject];

  CGPoint pt = [pTouch locationInView: self];
  CGPoint ptPrev = [pTouch previousLocationInView: self];

  IMouseInfo info;
  [self getTouchXY:pt x:&info.x y:&info.y];
  float prevX, prevY;
  [self getTouchXY:ptPrev x:&prevX y:&prevY];

  float dX = info.x - prevX;
  float dY = info.y - prevY;
  
  mGraphics->OnMouseDrag(info.x, info.y, dX, dY, info.ms);
}

- (void) touchesEnded: (NSSet*) pTouches withEvent: (UIEvent*) pEvent
{
  UITouch* pTouch = [pTouches anyObject];

  CGPoint pt = [pTouch locationInView: self];
  
  IMouseInfo info;
  [self getTouchXY:pt x:&info.x y:&info.y];
  mGraphics->OnMouseUp(info.x, info.y, info.ms);
}

- (void) touchesCancelled: (NSSet*) pTouches withEvent: (UIEvent*) pEvent
{
  //  [self pTouchesEnded: pTouches withEvent: event];
}

- (CAMetalLayer*) metalLayer
{
  return (CAMetalLayer *)self.layer;
}

- (void)dealloc
{
  [_displayLink invalidate];
  
  [super dealloc];
}

- (void)didMoveToSuperview
{
  [super didMoveToSuperview];
  if (self.superview)
  {
    self.displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(redraw:)];
    [self.displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
  }
  else
  {
    [self.displayLink invalidate];
    self.displayLink = nil;
  }
}

- (void)redraw:(CADisplayLink*) displayLink
{
  IRECTList rects;
  
  if (mGraphics->IsDirty(rects))
  {
    mGraphics->SetAllControlsClean();
    mGraphics->Draw(rects);
  }
}

- (BOOL) isOpaque
{
  return YES;
}

- (BOOL) acceptsFirstResponder
{
  return YES;
}

- (BOOL)canBecomeFirstResponder
{
  return YES;
}

- (void) removeFromSuperview
{
  [self.displayLink invalidate];
  self.displayLink = nil;
}

- (void) controlTextDidEndEditing: (NSNotification*) aNotification
{
}

- (IPopupMenu*) createPopupMenu: (const IPopupMenu&) menu : (CGRect) bounds;
{
  return nullptr;
}

- (void) createTextEntry: (IControl&) control : (const IText&) text : (const char*) str : (CGRect) areaRect;
{
 
}

- (void) endUserInput
{
}

+ (Class)layerClass
{
  return [CAMetalLayer class];
}

@end

#endif //NO_IGRAPHICS
