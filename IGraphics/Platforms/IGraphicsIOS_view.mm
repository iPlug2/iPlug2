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
//  [self setBoundsSize:CGSizeMake(pGraphics->Width(), pGraphics->Height())];
//
//  if (!self.wantsLayer) {
//    self.layer = (CALayer*) mGraphics->mLayer;
//    self.layer.opaque = NO;
//    self.wantsLayer = YES;
//  }

  double sec = 1.0 / (double) pGraphics->FPS();
  mTimer = [NSTimer timerWithTimeInterval:sec target:self selector:@selector(onTimer:) userInfo:nil repeats:YES];
  [[NSRunLoop currentRunLoop] addTimer: mTimer forMode: (NSString*) kCFRunLoopCommonModes];

  return self;
}

- (BOOL) isOpaque
{
  return mGraphics ? YES : NO;
}

- (BOOL) acceptsFirstResponder
{
  return YES;
}

- (void) onTimer: (NSTimer*) pTimer
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
//  [self setNeedsDisplay: YES];
}

- (void) killTimer
{
  [mTimer invalidate];
  mTimer = 0;
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

@end

#endif //NO_IGRAPHICS
