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
  
#if DISPLAY_LINK
  mDisplayLink = [CADisplayLink displayLinkWithTarget:self
                                             selector:@selector(render)];
  [mDisplayLink addToRunLoop:[NSRunLoop mainRunLoop]
                     forMode:NSRunLoopCommonModes];
#else
  double sec = 1.0 / (double) pGraphics->FPS();
  mTimer = [NSTimer timerWithTimeInterval:sec target:self selector:@selector(onTimer:) userInfo:nil repeats:YES];
  [[NSRunLoop currentRunLoop] addTimer: mTimer forMode: (NSString*) kCFRunLoopCommonModes];
#endif
  
  return self;
}

#if DISPLAY_LINK
- (void)dealloc
{
  [mDisplayLink invalidate];
  [super dealloc];
}
#else
- (void) killTimer
{
  [mTimer invalidate];
  mTimer = nullptr;
}
#endif

- (void) onTimer: (NSTimer*) pTimer
{
#ifdef IGRAPHICS_NANOVG
  [self render];
#else
  IRECTList rects;

  if (mGraphics->IsDirty(rects))
  {
    for (auto i = 0; i < rects.Size(); i++)
      [self setNeedsDisplayInRect:ToNSRect(mGraphics, rects.Get(i))];
  }
#endif
}

- (void)render
{
  //TODO: this is redrawing every IControl!
  mGraphics->SetAllControlsDirty();
  
  IRECTList rects;
  
  if (mGraphics->IsDirty(rects))
  {
    mGraphics->Draw(rects);
    [self.layer setNeedsDisplay];
  }
}

- (BOOL) isOpaque
{
  return NO;
}

- (BOOL) acceptsFirstResponder
{
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
