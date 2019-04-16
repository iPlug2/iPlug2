/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#import <QuartzCore/QuartzCore.h>
#ifdef IGRAPHICS_IMGUI
#import <Metal/Metal.h>
#include "imgui.h"
#import "imgui_impl_metal.h"
#endif

#import "IGraphicsIOS_view.h"
#include "IControl.h"
#include "IPlugParameter.h"

@implementation IGraphicsIOS_View

- (id) initWithIGraphics: (IGraphicsIOS*) pGraphics
{
  TRACE;

  mGraphics = pGraphics;
  CGRect r = CGRectMake(0.f, 0.f, (float) pGraphics->WindowWidth(), (float) pGraphics->WindowHeight());
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

#ifdef IGRAPHICS_IMGUI

@implementation IGRAPHICS_IMGUIVIEW
{
}

- (id) initWithIGraphicsView: (IGraphicsIOS_View*) pView;
{
  mView = pView;
  self = [super initWithFrame:[pView frame] device: MTLCreateSystemDefaultDevice()];
  if(self) {
    _commandQueue = [self.device newCommandQueue];
    self.layer.opaque = NO;
  }
  
  return self;
}

- (void)drawRect:(CGRect)rect
{
  id<MTLCommandBuffer> commandBuffer = [self.commandQueue commandBuffer];
  
  MTLRenderPassDescriptor *renderPassDescriptor = self.currentRenderPassDescriptor;
  if (renderPassDescriptor != nil)
  {
    renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0,0,0,0);
    
    id <MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    [renderEncoder pushDebugGroup:@"ImGui IGraphics"];
    
    ImGui_ImplMetal_NewFrame(renderPassDescriptor);
    
    mView->mGraphics->mImGuiRenderer->DoFrame();
    
    ImDrawData *drawData = ImGui::GetDrawData();
    ImGui_ImplMetal_RenderDrawData(drawData, commandBuffer, renderEncoder);
    
    [renderEncoder popDebugGroup];
    [renderEncoder endEncoding];
    
    [commandBuffer presentDrawable:self.currentDrawable];
  }
  [commandBuffer commit];
}

@end

#endif
