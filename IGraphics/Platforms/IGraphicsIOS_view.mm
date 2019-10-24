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

#include "IGraphicsCoreText.h"
#include "IControl.h"
#include "IPlugParameter.h"

extern StaticStorage<CoreTextFontDescriptor> sFontDescriptorCache;

@implementation IGraphicsIOS_View

- (id) initWithIGraphics: (IGraphicsIOS*) pGraphics
{
  TRACE

  mGraphics = pGraphics;
  CGRect r = CGRectMake(0.f, 0.f, (float) pGraphics->WindowWidth(), (float) pGraphics->WindowHeight());
  self = [super initWithFrame:r];
  
  //scrollview
  [self setContentSize:r.size];
  self.delegate = self;
  self.scrollEnabled = NO;
  
  self.layer.opaque = YES;
  self.layer.contentsScale = [UIScreen mainScreen].scale;
  
//  self.multipleTouchEnabled = YES;
  
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillShow:) name:UIKeyboardWillShowNotification object:nil];
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillBeHidden:) name:UIKeyboardWillHideNotification object:nil];
  
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
  
  #ifdef IGRAPHICS_METAL
  CGSize drawableSize = self.bounds.size;
  
  // Since drawable size is in pixels, we need to multiply by the scale to move from points to pixels
  drawableSize.width *= scale;
  drawableSize.height *= scale;
  
  self.metalLayer.drawableSize = drawableSize;
  #endif
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
  if(mTextField)
    [self endUserInput];
  
  UITouch* pTouch = [pTouches anyObject];
  CGPoint pt = [pTouch locationInView: self];

  IMouseInfo info;
  info.ms.L = true;
  [self getTouchXY:pt x:&info.x y:&info.y];
  
  if(mGraphics)
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
  
  if(mGraphics)
    mGraphics->OnMouseDrag(info.x, info.y, dX, dY, info.ms);
}

- (void) touchesEnded: (NSSet*) pTouches withEvent: (UIEvent*) pEvent
{
  UITouch* pTouch = [pTouches anyObject];

  CGPoint pt = [pTouch locationInView: self];
  
  IMouseInfo info;
  [self getTouchXY:pt x:&info.x y:&info.y];
  
  if(mGraphics)
    mGraphics->OnMouseUp(info.x, info.y, info.ms);
}

- (void) touchesCancelled: (NSSet*) pTouches withEvent: (UIEvent*) pEvent
{
  //  [self pTouchesEnded: pTouches withEvent: event];
}

- (CAMetalLayer*) metalLayer
{
  return (CAMetalLayer*) self.layer;
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
    self.displayLink.preferredFramesPerSecond = mGraphics->FPS();
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
  
  if(mGraphics)
  {
    if (mGraphics->IsDirty(rects))
    {
      mGraphics->SetAllControlsClean();
      mGraphics->Draw(rects);
    }
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

- (void)textFieldDidEndEditing:(UITextField *)textField reason:(UITextFieldDidEndEditingReason)reason
{
  [self endUserInput];
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
  if(textField == mTextField)
  {
    mGraphics->SetControlValueAfterTextEdit([[mTextField text] UTF8String]);
    mGraphics->SetAllControlsDirty();
    
    [self endUserInput];
  }
  return YES;
}

- (void) textFieldDidEndEditing:(UITextField *) textField
{
  [self endUserInput];
}

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
  if (!string.length)
    return YES;
  
  // verify max length has not been exceeded
  NSString* proposedText = [mTextField.text stringByReplacingCharactersInRange:range withString:string];
  
  if (proposedText.length > mTextFieldLength)
    return NO;
  
  IControl* pInTextEntry = mGraphics->GetControlInTextEntry();
  
  if(pInTextEntry)
  {
    const IParam* pParam = pInTextEntry->GetParam();
    
    if (pParam)
    {
      NSMutableCharacterSet *characterSet = [[[NSMutableCharacterSet alloc] init] autorelease];
      
      switch ( pParam->Type() )
      {
        case IParam::kTypeEnum:
        case IParam::kTypeInt:
        case IParam::kTypeBool:
          [characterSet addCharactersInString:@"0123456789-+"];
          break;
        case IParam::kTypeDouble:
          [characterSet addCharactersInString:@"0123456789.-+"];
          break;
        default:
          break;
      }
      
      if ([string rangeOfCharacterFromSet:characterSet.invertedSet].location != NSNotFound)
        return NO;
    }
  }
  
  return YES;
}

- (IPopupMenu*) createPopupMenu: (const IPopupMenu&) menu : (CGRect) bounds;
{
  return nullptr;
}

- (void) createTextEntry: (int) paramIdx : (const IText&) text : (const char*) str : (int) length : (CGRect) areaRect
{
  if (mTextField)
    return;

  mTextField = [[UITextField alloc] initWithFrame:areaRect];
  mTextFieldLength = length;
  
  CoreTextFontDescriptor* CTFontDescriptor = CoreTextHelpers::GetCTFontDescriptor(text, sFontDescriptorCache);
  UIFontDescriptor* fontDescriptor = (UIFontDescriptor*) CTFontDescriptor->GetDescriptor();
  UIFont* font = [UIFont fontWithDescriptor: fontDescriptor size: text.mSize * 0.75];
  [mTextField setFont: font];
  
  [mTextField setText:[NSString stringWithUTF8String:str]];
  [mTextField setTextColor:ToUIColor(text.mTextEntryFGColor)];
  [mTextField setBackgroundColor:ToUIColor(text.mTextEntryBGColor)];
  [mTextField setAutocorrectionType:UITextAutocorrectionTypeNo];
  [mTextField setDelegate:self];
  
  switch (text.mVAlign)
  {
    case EVAlign::Top:
      [mTextField setContentVerticalAlignment:UIControlContentVerticalAlignmentTop];
      break;
    case EVAlign::Middle:
      [mTextField setContentVerticalAlignment:UIControlContentVerticalAlignmentCenter];
      break;
    case EVAlign::Bottom:
      [mTextField setContentVerticalAlignment:UIControlContentVerticalAlignmentBottom];
      break;
    default:
      break;
  }
  
  switch (text.mAlign)
  {
    case EAlign::Near:
      [mTextField setTextAlignment: NSTextAlignmentLeft];
      break;
    case EAlign::Center:
      [mTextField setTextAlignment: NSTextAlignmentCenter];
      break;
    case EAlign::Far:
      [mTextField setTextAlignment: NSTextAlignmentRight];
      break;
    default:
      break;
  }
  
  [self addSubview: mTextField];
  [mTextField becomeFirstResponder];
}

- (void) endUserInput
{
  [self becomeFirstResponder];
  [mTextField setDelegate: nil];
  [mTextField removeFromSuperview]; //releases
  mTextField = nullptr;
}

- (void) showMessageBox: (const char*) str : (const char*) caption : (EMsgBoxType) type : (IMsgBoxCompletionHanderFunc) completionHandler
{
  NSString* titleNString = [NSString stringWithUTF8String:str];
  NSString* captionNString = [NSString stringWithUTF8String:caption];
  
  UIAlertController* alertController = [UIAlertController alertControllerWithTitle:titleNString message:captionNString preferredStyle:UIAlertControllerStyleAlert];
  
  void (^handlerBlock)(UIAlertAction*) =
  ^(UIAlertAction* action) {
    
    if(completionHandler != nullptr)
    {
      EMsgBoxResult result = EMsgBoxResult::kCANCEL;
      
      if([action.title isEqualToString:@"OK"])
        result = EMsgBoxResult::kOK;
      if([action.title isEqualToString:@"Cancel"])
        result = EMsgBoxResult::kCANCEL;
      if([action.title isEqualToString:@"Yes"])
        result = EMsgBoxResult::kYES;
      if([action.title isEqualToString:@"No"])
        result = EMsgBoxResult::kNO;
      if([action.title isEqualToString:@"Retry"])
        result = EMsgBoxResult::kRETRY;
      
      completionHandler(result);
    }
    
  };
  
  if(type == kMB_OK || type == kMB_OKCANCEL)
  {
    UIAlertAction* okAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:handlerBlock];
    [alertController addAction:okAction];
  }
  
  if(type == kMB_YESNO || type == kMB_YESNOCANCEL)
  {
    UIAlertAction* yesAction = [UIAlertAction actionWithTitle:@"Yes" style:UIAlertActionStyleDefault handler:handlerBlock];
    [alertController addAction:yesAction];
    
    UIAlertAction* noAction = [UIAlertAction actionWithTitle:@"No" style:UIAlertActionStyleDefault handler:handlerBlock];
    [alertController addAction:noAction];
  }
  
  if(type == kMB_RETRYCANCEL)
  {
    UIAlertAction* retryAction = [UIAlertAction actionWithTitle:@"Retry" style:UIAlertActionStyleDefault handler:handlerBlock];
    [alertController addAction:retryAction];
  }
  
  if(type == kMB_OKCANCEL || type == kMB_YESNOCANCEL || type == kMB_RETRYCANCEL)
  {
    UIAlertAction* cancelAction = [UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:handlerBlock];
    [alertController addAction:cancelAction];
  }
  
  [self.window.rootViewController presentViewController:alertController animated:YES completion:nil];
}

+ (Class) layerClass
{
#ifdef IGRAPHICS_METAL
  return [CAMetalLayer class];
#else
  return [CALayer class];
#endif
}

- (void)keyboardWillShow:(NSNotification*) notification
{
  NSDictionary* info = [notification userInfo];
  CGSize kbSize = [[info objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue].size;
  
  UIEdgeInsets contentInsets = UIEdgeInsetsMake(0.0, 0.0, kbSize.height, 0.0);
  self.contentInset = contentInsets;
  self.scrollIndicatorInsets = contentInsets;
  
  CGRect r = self.frame;
  r.size.height -= kbSize.height;
  
  if (!CGRectContainsPoint(r, CGPointMake(mTextField.frame.origin.x + mTextField.frame.size.width, mTextField.frame.origin.y + mTextField.frame.size.height)) ) {
    [self scrollRectToVisible:mTextField.frame animated:YES];
  }
}

- (void)keyboardWillBeHidden:(NSNotification*) notification
{
  UIEdgeInsets contentInsets = UIEdgeInsetsZero;
  self.contentInset = contentInsets;
  self.scrollIndicatorInsets = contentInsets;
}

- (BOOL) delaysContentTouches
{
  return NO;
}

- (void)scrollViewDidScroll:(UIScrollView*) scrollView
{
  mGraphics->SetTranslation(0, -self.contentOffset.y);
  mGraphics->SetAllControlsDirty();
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

