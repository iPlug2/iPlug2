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
  {
    scale = self.window.screen.scale;
  }
  
  if(mGraphics)
    mGraphics->SetDisplayScale(scale);

  CGSize drawableSize = self.bounds.size;
  
  // Since drawable size is in pixels, we need to multiply by the scale to move from points to pixels
  drawableSize.width *= scale;
  drawableSize.height *= scale;
  
  self.metalLayer.drawableSize = drawableSize;
}

- (void) touchesBegan: (NSSet*) pTouches withEvent: (UIEvent*) pEvent
{
  UITouch* pTouch = [pTouches anyObject];
  
  CGPoint p = [pTouch locationInView: self];
  
  IMouseMod mod { true };
  
  mGraphics->OnMouseDown(p.x, p.y, mod);
}

- (void) touchesMoved: (NSSet*) pTouches withEvent: (UIEvent*) pEvent
{
  UITouch* pTouch = [pTouches anyObject];

  CGPoint p = [pTouch locationInView: self];
  CGPoint pPrev = [pTouch previousLocationInView: self];

  IMouseMod mod;
  float dX = p.x - pPrev.x;
  float dY = p.y - pPrev.y;
  
  mGraphics->OnMouseDrag(p.x, p.y, dX, dY, mod);
}

- (void) touchesEnded: (NSSet*) pTouches withEvent: (UIEvent*) pEvent
{
  UITouch* pTouch = [pTouches anyObject];

  CGPoint p = [pTouch locationInView: self];
  
  IMouseMod mod;
  
  mGraphics->OnMouseUp(p.x, p.y, mod);
}

- (void) touchesCancelled: (NSSet*) pTouches withEvent: (UIEvent*) pEvent
{
  //  [self pTouchesEnded: pTouches withEvent: event];
}

- (CAMetalLayer *)metalLayer {
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
    self.displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(displayLinkDidFire:)];
    [self.displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
  }
  else
  {
    [self.displayLink invalidate];
    self.displayLink = nil;
  }
}

- (void)displayLinkDidFire:(CADisplayLink *)displayLink
{
  [self redraw];
}

- (void)redraw
{  
  IRECT r;
  
  //TODO: this is redrawing every IControl!
  r.R = mGraphics->Width();
  r.B = mGraphics->Height();
  mGraphics->IsDirty(r);
  //
  
  mGraphics->Draw(r);
  
  mGraphics->EndFrame();
}

- (BOOL) isOpaque
{
  return YES;
}

- (BOOL) acceptsFirstResponder
{
  return YES;
}

- (BOOL)canBecomeFirstResponder {
  return YES;
}

- (void) removeFromSuperview
{
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
