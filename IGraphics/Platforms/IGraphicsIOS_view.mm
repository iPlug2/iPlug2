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
  mTimer = 0;
}
#endif

- (void) onTimer: (NSTimer*) pTimer
{
#ifdef IGRAPHICS_NANOVG
  [self render];
#else
  IRECT r;

  if (mGraphics->IsDirty(r))
  {
    [self setNeedsDisplayInRect:ToNSRect(mGraphics, r)];
  }
#endif
}

- (void)render
{
  IRECT r;
  mGraphics->BeginFrame();
  
  //TODO: this is redrawing every IControl!
  r.R = mGraphics->WindowWidth();
  r.B = mGraphics->WindowHeight();
  mGraphics->IsDirty(r);
  //
  
  mGraphics->Draw(r);
  
  mGraphics->EndFrame();
  [self.layer setNeedsDisplay]; // TODO: if nothing is dirty shouldn't set this
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
