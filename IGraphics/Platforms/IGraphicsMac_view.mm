/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#ifndef NO_IGRAPHICS

#ifdef IGRAPHICS_NANOVG
#import <QuartzCore/QuartzCore.h>
#endif

#import "IGraphicsMac_view.h"
#include "IControl.h"
#include "IPlugParameter.h"
#include "IPlugLogger.h"

@implementation IGRAPHICS_MENU_RCVR

- (NSMenuItem*)menuItem
{
  return nsMenuItem;
}

- (void)onMenuSelection:(id)sender
{
  nsMenuItem = sender;
}

@end

@implementation IGRAPHICS_MENU

- (id)initWithIPopupMenuAndReciever:(IPopupMenu*)pMenu : (NSView*)pView
{
  [self initWithTitle: @""];

  NSMenuItem* nsMenuItem;
  NSMutableString* nsMenuItemTitle;

  [self setAutoenablesItems:NO];

  int numItems = pMenu->NItems();

  for (int i = 0; i < numItems; ++i)
  {
    IPopupMenu::Item* pMenuItem = pMenu->GetItem(i);

    nsMenuItemTitle = [[[NSMutableString alloc] initWithCString:pMenuItem->GetText() encoding:NSUTF8StringEncoding] autorelease];

    if (pMenu->GetPrefix())
    {
      NSString* prefixString = 0;

      switch (pMenu->GetPrefix())
      {
        case 0: prefixString = [NSString stringWithUTF8String:""]; break;
        case 1: prefixString = [NSString stringWithFormat:@"%1d: ", i+1]; break;
        case 2: prefixString = [NSString stringWithFormat:@"%02d: ", i+1]; break;
        case 3: prefixString = [NSString stringWithFormat:@"%03d: ", i+1]; break;
      }

      [nsMenuItemTitle insertString:prefixString atIndex:0];
    }

    if (pMenuItem->GetSubmenu())
    {
      nsMenuItem = [self addItemWithTitle:nsMenuItemTitle action:nil keyEquivalent:@""];
      NSMenu* subMenu = [[IGRAPHICS_MENU alloc] initWithIPopupMenuAndReciever:pMenuItem->GetSubmenu() :pView];
      [self setSubmenu: subMenu forItem:nsMenuItem];
      [subMenu release];
    }
    else if (pMenuItem->GetIsSeparator())
    {
      [self addItem:[NSMenuItem separatorItem]];
    }
    else
    {
      nsMenuItem = [self addItemWithTitle:nsMenuItemTitle action:@selector(onMenuSelection:) keyEquivalent:@""];
      
      [nsMenuItem setTarget:pView];
      
      if (pMenuItem->GetIsTitle ())
      {
        [nsMenuItem setIndentationLevel:1];
      }

      if (pMenuItem->GetChecked())
      {
        [nsMenuItem setState:NSOnState];
      }
      else
      {
        [nsMenuItem setState:NSOffState];
      }

      if (pMenuItem->GetEnabled())
      {
        [nsMenuItem setEnabled:YES];
      }
      else
      {
        [nsMenuItem setEnabled:NO];
      }

    }
  }

  mIPopupMenu = pMenu;

  return self;
}

- (IPopupMenu*)iPopupMenu
{
  return mIPopupMenu;
}

@end

NSString* ToNSString(const char* cStr)
{
  return [NSString stringWithCString:cStr encoding:NSUTF8StringEncoding];
}

inline int GetMouseOver(IGraphicsMac* pGraphics)
{
  return pGraphics->GetMouseOver();
}

// IGRAPHICS_TEXTFIELDCELL based on...

// https://red-sweater.com/blog/148/what-a-difference-a-cell-makes

// This source code is provided to you compliments of Red Sweater Software under the license as described below. NOTE: This is the MIT License.
//
// Copyright (c) 2006 Red Sweater Software
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

@implementation IGRAPHICS_TEXTFIELDCELL

- (NSRect)drawingRectForBounds:(NSRect)theRect
{
  // Get the parent's idea of where we should draw
  NSRect newRect = [super drawingRectForBounds:theRect];
  
  // When the text field is being
  // edited or selected, we have to turn off the magic because it screws up
  // the configuration of the field editor.  We sneak around this by
  // intercepting selectWithFrame and editWithFrame and sneaking a
  // reduced, centered rect in at the last minute.
  if (mIsEditingOrSelecting == NO)
  {
    // Get our ideal size for current text
    NSSize textSize = [self cellSizeForBounds:theRect];
    
    // Center that in the proposed rect
    float heightDelta = newRect.size.height - textSize.height;
    if (heightDelta > 0)
    {
      newRect.size.height -= heightDelta;
      newRect.origin.y += (heightDelta / 2);
    }
  }
  
  return newRect;
}

- (void)selectWithFrame:(NSRect)aRect inView:(NSView *)controlView editor:(NSText *)textObj delegate:(id)anObject start:(NSInteger)selStart length:(NSInteger)selLength
{
  aRect = [self drawingRectForBounds:aRect];
  mIsEditingOrSelecting = YES;
  [super selectWithFrame:aRect inView:controlView editor:textObj delegate:anObject start:selStart length:selLength];
  mIsEditingOrSelecting = NO;
}

- (void)editWithFrame:(NSRect)aRect inView:(NSView *)controlView editor:(NSText *)textObj delegate:(id)anObject event:(NSEvent *)theEvent
{
  aRect = [self drawingRectForBounds:aRect];
  mIsEditingOrSelecting = YES;
  [super editWithFrame:aRect inView:controlView editor:textObj delegate:anObject event:theEvent];
  mIsEditingOrSelecting = NO;
}
@end


@implementation IGRAPHICS_FORMATTER
- (void) dealloc
{
  [filterCharacterSet release];
  [super dealloc];
}

- (BOOL)isPartialStringValid:(NSString *)partialString newEditingString:(NSString **)newString errorDescription:(NSString **)error
{
  if (filterCharacterSet != nil)
  {
    int i = 0;
    int len = (int) [partialString length];

    for (i = 0; i < len; i++)
    {
      if (![filterCharacterSet characterIsMember:[partialString characterAtIndex:i]])
      {
        return NO;
      }
    }
  }

  if (maxLength)
  {
    if ([partialString length] > maxLength)
    {
      return NO;
    }
  }

  if (maxValue && [partialString intValue] > maxValue)
  {
    return NO;
  }

  return YES;
}

- (void) setAcceptableCharacterSet:(NSCharacterSet *) inCharacterSet
{
  [inCharacterSet retain];
  [filterCharacterSet release];
  filterCharacterSet = inCharacterSet;
}

- (void) setMaximumLength:(int) inLength
{
  maxLength = inLength;
}

- (void) setMaximumValue:(int) inValue
{
  maxValue = inValue;
}

- (NSString *)stringForObjectValue:(id)anObject
{
  if ([anObject isKindOfClass:[NSString class]])
  {
    return anObject;
  }

  return nil;
}

- (BOOL)getObjectValue:(id *)anObject forString:(NSString *)string errorDescription:(NSString **)error
{
  if (anObject && string)
  {
    *anObject = [NSString stringWithString:string];
  }

  return YES;
}
@end

#pragma mark -

@implementation IGRAPHICS_VIEW

- (id) initWithIGraphics: (IGraphicsMac*) pGraphics
{
  TRACE;

  mGraphics = pGraphics;
  NSRect r;
  r.origin.x = r.origin.y = 0.0f;
  r.size.width = (float) pGraphics->WindowWidth();
  r.size.height = (float) pGraphics->WindowHeight();
  self = [super initWithFrame:r];
  
#if defined IGRAPHICS_NANOVG
  #if defined IGRAPHICS_METAL
    if (!self.wantsLayer) {
      self.layer = [CAMetalLayer new];
      self.layer.opaque = YES;
      self.wantsLayer = YES;
    }
  #elif defined IGRAPHICS_GL
    const NSOpenGLPixelFormatAttribute kAttributes[] =  {
      NSOpenGLPFAAccelerated,
      NSOpenGLPFANoRecovery,
      NSOpenGLPFATripleBuffer,
      NSOpenGLPFAAlphaSize, 8,
      NSOpenGLPFAColorSize, 24,
      NSOpenGLPFADepthSize, 0,
      NSOpenGLPFAStencilSize, 8,
      (NSOpenGLPixelFormatAttribute)0};
    mPixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:kAttributes];
    mContext = [[NSOpenGLContext alloc] initWithFormat:mPixelFormat
                                          shareContext:nil];
  
    // Sets sync to VBL to eliminate tearing.
    GLint vblSync = 1;
    [mContext setValues:&vblSync forParameter:NSOpenGLCPSwapInterval];
    // Allows for transparent background.
//    GLint opaque = 0;
//    [mContext setValues:&opaque forParameter:NSOpenGLCPSurfaceOpacity];
//    [self setWantsBestResolutionOpenGLSurface:YES];
    [mContext makeCurrentContext];
  
  #endif
#endif

  [self registerForDraggedTypes:[NSArray arrayWithObjects: NSFilenamesPboardType, nil]];

  double sec = 1.0 / (double) pGraphics->FPS();
  mTimer = [NSTimer timerWithTimeInterval:sec target:self selector:@selector(onTimer:) userInfo:nil repeats:YES];
  [[NSRunLoop currentRunLoop] addTimer: mTimer forMode: (NSString*) kCFRunLoopCommonModes];

  return self;
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [super dealloc];
}

- (BOOL) isOpaque
{
  return mGraphics ? YES : NO;
}

- (BOOL) isFlipped
{
    return YES;
}

- (BOOL) acceptsFirstResponder
{
  return YES;
}

- (BOOL) acceptsFirstMouse: (NSEvent*) pEvent
{
  return YES;
}

- (void) viewDidMoveToWindow
{
  NSWindow* pWindow = [self window];
  
  if (pWindow)
  {
    [pWindow makeFirstResponder: self];
    [pWindow setAcceptsMouseMovedEvents: YES];
    
    if (mGraphics)
      mGraphics->SetScreenScale([pWindow backingScaleFactor]);
    
//    [[NSNotificationCenter defaultCenter] addObserver:self
//                                             selector:@selector(windowResized:) name:NSWindowDidEndLiveResizeNotification
//                                               object:pWindow];
//
//    [[NSNotificationCenter defaultCenter] addObserver:self
//                                             selector:@selector(windowFullscreened:) name:NSWindowDidEnterFullScreenNotification
//                                               object:pWindow];
//
//    [[NSNotificationCenter defaultCenter] addObserver:self
//                                             selector:@selector(windowFullscreened:) name:NSWindowDidExitFullScreenNotification
//                                               object:pWindow];
  }
}

- (void) viewDidChangeBackingProperties:(NSNotification *) notification
{
  NSWindow* pWindow = [self window];
  
  if (!pWindow)
    return;
  
  CGFloat newScale = [pWindow backingScaleFactor];
  
  if (newScale != mGraphics->GetScreenScale())
    mGraphics->SetScreenScale(newScale);
}

// not called for opengl/metal
- (void) drawRect: (NSRect) bounds
{
#ifndef IGRAPHICS_GL
  if (mGraphics)
  {
    //TODO: can we really only get this context on the first draw call?
    if (!mGraphics->GetPlatformContext())
    {
      CGContextRef pCGC = nullptr;
      pCGC = (CGContextRef) [[NSGraphicsContext currentContext] graphicsPort];
      NSGraphicsContext* gc = [NSGraphicsContext graphicsContextWithGraphicsPort: pCGC flipped: YES];
      pCGC = (CGContextRef) [gc graphicsPort];
      mGraphics->SetPlatformContext(pCGC);
    }
      
    if (mGraphics->GetPlatformContext())
    {
      const NSRect *rects;
      NSInteger numRects;
      [self getRectsBeingDrawn:&rects count:&numRects];
      IRECTList drawRects;

      for (int i = 0; i < numRects; i++)
        drawRects.Add(ToIRECT(mGraphics, &rects[i]));
      
      mGraphics->Draw(drawRects);
    }
  }
#endif
}

- (void) onTimer: (NSTimer*) pTimer
{
  [self render];
}

- (void) render
{
#ifdef IGRAPHICS_GL
//  CGLLockContext([mContext CGLContextObj]);
  [mContext setView:self];
  [mContext makeCurrentContext];
#endif
  
  IRECTList rects;
  if (mGraphics->IsDirty(rects))
  {
    mGraphics->SetAllControlsClean();
#if !defined IGRAPHICS_NANOVG
    for (int i = 0; i < rects.Size(); i++)
    [self setNeedsDisplayInRect:ToNSRect(mGraphics, rects.Get(i))];
#else
    mGraphics->Draw(rects); // for metal/opengl drawRect is not called
#endif
  }
  
#ifdef IGRAPHICS_GL
//  CGLLockContext([mContext CGLContextObj]);
  [mContext flushBuffer];
#endif
}

- (void) getMouseXY: (NSEvent*) pEvent x: (float*) pX y: (float*) pY
{
  if (mGraphics)
  {
    NSPoint pt = [self convertPoint:[pEvent locationInWindow] fromView:nil];
    // TODO - fix or remove these values!!
    *pX = pt.x / mGraphics->GetDrawScale();//- 2.f;
    *pY = pt.y / mGraphics->GetDrawScale();//- 3.f;
    mPrevX = *pX;
    mPrevY = *pY;

    // Detect tablet input correctly
    mGraphics->SetTabletInput(pEvent.subtype == NSTabletPointEventSubtype);
    mGraphics->SetMousePosition(*pX, *pY);
  }
}

- (IMouseInfo) getMouseLeft: (NSEvent*) pEvent
{
  IMouseInfo info;
  [self getMouseXY:pEvent x:&info.x y:&info.y];
  int mods = (int) [pEvent modifierFlags];
  info.ms = IMouseMod(true, (mods & NSCommandKeyMask), (mods & NSShiftKeyMask), (mods & NSControlKeyMask), (mods & NSAlternateKeyMask));

  return info;
}

- (IMouseInfo) getMouseRight: (NSEvent*) pEvent
{
  IMouseInfo info;
  [self getMouseXY:pEvent x:&info.x y:&info.y];
  int mods = (int) [pEvent modifierFlags];
  info.ms = IMouseMod(false, true, (mods & NSShiftKeyMask), (mods & NSControlKeyMask), (mods & NSAlternateKeyMask));

  return info;
}

- (void) mouseDown: (NSEvent*) pEvent
{
  IMouseInfo info = [self getMouseLeft:pEvent];
  if (mGraphics)
  {
    if ([pEvent clickCount] > 1)
    {
      mGraphics->OnMouseDblClick(info.x, info.y, info.ms);
    }
    else
    {
      mGraphics->OnMouseDown(info.x, info.y, info.ms);
    }
  }
}

- (void) mouseUp: (NSEvent*) pEvent
{
  IMouseInfo info = [self getMouseLeft:pEvent];
  if (mGraphics)
    mGraphics->OnMouseUp(info.x, info.y, info.ms);
}

- (void) mouseDragged: (NSEvent*) pEvent
{
  IMouseInfo info = [self getMouseLeft:pEvent];
  float dX = [pEvent deltaX];
  float dY = [pEvent deltaY];
  if (mGraphics && !mTextFieldView)
    mGraphics->OnMouseDrag(info.x, info.y, dX, dY, info.ms);
}

- (void) rightMouseDown: (NSEvent*) pEvent
{
  IMouseInfo info = [self getMouseRight:pEvent];
  if (mGraphics)
    mGraphics->OnMouseDown(info.x, info.y, info.ms);
}

- (void) rightMouseUp: (NSEvent*) pEvent
{
  IMouseInfo info = [self getMouseRight:pEvent];
  if (mGraphics)
    mGraphics->OnMouseUp(info.x, info.y, info.ms);
}

- (void) rightMouseDragged: (NSEvent*) pEvent
{
  IMouseInfo info = [self getMouseRight:pEvent];
  float dX = [pEvent deltaX];
  float dY = [pEvent deltaY];
  if (mGraphics && !mTextFieldView)
    mGraphics->OnMouseDrag(info.x, info.y, dX, dY, info.ms);
}

- (void) mouseMoved: (NSEvent*) pEvent
{
  IMouseInfo info = [self getMouseLeft:pEvent];
  if (mGraphics)
    mGraphics->OnMouseOver(info.x, info.y, info.ms);
}

- (void)keyDown: (NSEvent *)pEvent
{
#ifdef IGRAPHICS_SWELL
  int flag, code = SWELL_MacKeyToWindowsKey(pEvent, &flag);

  bool handle = mGraphics->OnKeyDown(mPrevX, mPrevY, code);
  
  if (!handle)
  {
    [[self nextResponder] keyDown:pEvent];
  }
#else
  NSString *s = [pEvent charactersIgnoringModifiers];

  if ([s length] == 1)
  {
    unsigned short k = [pEvent keyCode];
    unichar c = [s characterAtIndex:0];

    bool handle = true;
    int key = KEY_NONE;

    if (k == 48) key = KEY_TAB;
    else if (k == 49) key = KEY_SPACE;
    else if (k == 126) key = KEY_UPARROW;
    else if (k == 125) key = KEY_DOWNARROW;
    else if (k == 123) key = KEY_LEFTARROW;
    else if (k == 124) key = KEY_RIGHTARROW;
    else if (c >= '0' && c <= '9') key = KEY_DIGIT_0+c-'0';
    else if (c >= 'A' && c <= 'Z') key = KEY_ALPHA_A+c-'A';
    else if (c >= 'a' && c <= 'z') key = KEY_ALPHA_A+c-'a';
    else handle = false;

    if (handle)
    {
      // can't use getMouseXY because its a key event
      handle = mGraphics->OnKeyDown(mPrevX, mPrevY, key);
    }
    
    if (!handle)
    {
      [[self nextResponder] keyDown:pEvent];
    }
  }
#endif
}

- (void) scrollWheel: (NSEvent*) pEvent
{
  if (mTextFieldView) [self endUserInput ];
  IMouseInfo info = [self getMouseLeft:pEvent];
  float d = [pEvent deltaY];
  if (mGraphics)
    mGraphics->OnMouseWheel(info.x, info.y, info.ms, d);
}

- (void) setMouseCursor: (ECursor) cursor
{
  NSCursor* pCursor = nullptr;
  
  switch (cursor)
  {
    case ECursor::ARROW: pCursor = [NSCursor arrowCursor]; break;
    case ECursor::IBEAM: pCursor = [NSCursor IBeamCursor]; break;
    case ECursor::WAIT:
      if ([NSCursor respondsToSelector:@selector(_waitCursor)])
        pCursor = [NSCursor performSelector:@selector(_waitCursor)];
      break;
    case ECursor::CROSS: pCursor = [NSCursor crosshairCursor]; break;
    case ECursor::UPARROW: pCursor = [NSCursor resizeUpCursor]; break;
    case ECursor::SIZENWSE:
      if ([NSCursor respondsToSelector:@selector(_windowResizeNorthWestSouthEastCursor)])
        pCursor = [NSCursor performSelector:@selector(_windowResizeNorthWestSouthEastCursor)];
      break;
    case ECursor::SIZENESW:
      if ([NSCursor respondsToSelector:@selector(_windowResizeNorthEastSouthWestCursor)])
        pCursor = [NSCursor performSelector:@selector(_windowResizeNorthEastSouthWestCursor)];
      break;
    case ECursor::SIZEWE: pCursor = [NSCursor resizeLeftRightCursor]; break;
    case ECursor::SIZENS:
      if ([NSCursor respondsToSelector:@selector(_windowResizeNorthSouthCursor)])
        pCursor = [NSCursor performSelector:@selector(_windowResizeNorthSouthCursor)];
      break;
    case ECursor::SIZEALL:
      if ([NSCursor respondsToSelector:@selector(_moveCursor)])
        pCursor = [NSCursor performSelector:@selector(_moveCursor)];
      break;
    case ECursor::INO: pCursor = [NSCursor performSelector:@selector(operationNotAllowedCursor)]; break;
    case ECursor::HAND: pCursor = [NSCursor pointingHandCursor]; break;
    case ECursor::HELP:
      if ([NSCursor respondsToSelector:@selector(_helpCursor)])
        pCursor = [NSCursor performSelector:@selector(_helpCursor)];
      break;
    default: pCursor = [NSCursor arrowCursor]; break;
  }
  
  if(!pCursor)
    pCursor = [NSCursor arrowCursor];

  [pCursor set];
}

- (void) killTimer
{
  [mTimer invalidate];
  mTimer = 0;
}

- (void) removeFromSuperview
{
  if (mTextFieldView)
    [self endUserInput ];
  
//  if (mWebView) {
//    [mWebView removeFromSuperview ];
//    mWebView = nullptr;
//  }
  
  if (mGraphics)
  {
    IGraphics* pGraphics = mGraphics;
    mGraphics = nullptr;
    pGraphics->SetPlatformContext(nullptr);
    pGraphics->CloseWindow();
  }
  [super removeFromSuperview];
}

- (void) controlTextDidEndEditing: (NSNotification*) aNotification
{
  char* txt = (char*)[[mTextFieldView stringValue] UTF8String];

  if (mEdControl->GetParam())
    mGraphics->SetControlValueFromStringAfterPrompt(*mEdControl, txt);

  mEdControl->OnTextEntryCompletion(txt);
  mGraphics->SetAllControlsDirty();

  [self endUserInput ];
  [self setNeedsDisplay: YES];
}

- (IPopupMenu*) createPopupMenu: (const IPopupMenu&) menu : (NSRect) bounds;
{
  IGRAPHICS_MENU_RCVR* pDummyView = [[[IGRAPHICS_MENU_RCVR alloc] initWithFrame:bounds] autorelease];
  NSMenu* pNSMenu = [[[IGRAPHICS_MENU alloc] initWithIPopupMenuAndReciever:&const_cast<IPopupMenu&>(menu) :pDummyView] autorelease];
  NSPoint wp = {bounds.origin.x, bounds.origin.y - 4};

  [pNSMenu popUpMenuPositioningItem:nil atLocation:wp inView:self];
  
  NSMenuItem* pChosenItem = [pDummyView menuItem];
  NSMenu* pChosenMenu = [pChosenItem menu];
  IPopupMenu* pIPopupMenu = [(IGRAPHICS_MENU*) pChosenMenu iPopupMenu];

  long chosenItemIdx = [pChosenMenu indexOfItem: pChosenItem];

  if (chosenItemIdx > -1 && pIPopupMenu)
  {
    pIPopupMenu->SetChosenItemIdx((int) chosenItemIdx);
    return pIPopupMenu;
  }
  else return nullptr;
}

- (void) createTextEntry: (IControl&) control : (const IText&) text : (const char*) str : (NSRect) areaRect;
{
  if (mTextFieldView)
    return;

  mTextFieldView = [[NSTextField alloc] initWithFrame: areaRect];
  
  if (text.mVAlign == IText::kVAlignMiddle)
  {
    IGRAPHICS_TEXTFIELDCELL* pCell = [[IGRAPHICS_TEXTFIELDCELL alloc] initTextCell:@"textfield"];
    [mTextFieldView setCell: pCell];
    [mTextFieldView setEditable: TRUE];
    [mTextFieldView setDrawsBackground: TRUE];
  }

  //TODO: this is wrong
#ifdef IGRAPHICS_NANOVG
  NSString* font = [NSString stringWithUTF8String: "Arial"];
#else
  NSString* font = [NSString stringWithUTF8String: text.mFont];
#endif

#ifdef IGRAPHICS_LICE
  [mTextFieldView setFont: [NSFont fontWithName:font size: text.mSize * 0.75f]];
#else
  [mTextFieldView setFont: [NSFont fontWithName:font size: text.mSize]];
#endif
  
  switch (text.mAlign)
  {
    case IText::kAlignNear:
      [mTextFieldView setAlignment: NSLeftTextAlignment];
      break;
    case IText::kAlignCenter:
      [mTextFieldView setAlignment: NSCenterTextAlignment];
      break;
    case IText::kAlignFar:
      [mTextFieldView setAlignment: NSRightTextAlignment];
      break;
    default:
      break;
  }

  const IParam* pParam = control.GetParam();

  // set up formatter
  if (pParam)
  {
    NSMutableCharacterSet *characterSet = [[NSMutableCharacterSet alloc] init];

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

    [mTextFieldView setFormatter:[[[IGRAPHICS_FORMATTER alloc] init] autorelease]];
    [[mTextFieldView formatter] setAcceptableCharacterSet:characterSet];
    [[mTextFieldView formatter] setMaximumLength:control.GetTextEntryLength()];
    [characterSet release];
  }

  [[mTextFieldView cell] setLineBreakMode: NSLineBreakByTruncatingTail];
  [mTextFieldView setAllowsEditingTextAttributes:NO];
  [mTextFieldView setTextColor:ToNSColor(text.mTextEntryFGColor)];
  [mTextFieldView setBackgroundColor:ToNSColor(text.mTextEntryBGColor)];

  [mTextFieldView setStringValue: ToNSString(str)];

#ifndef COCOA_TEXTENTRY_BORDERED
  [mTextFieldView setBordered: NO];
  [mTextFieldView setFocusRingType:NSFocusRingTypeNone];
#endif

  [mTextFieldView setDelegate: self];

  [self addSubview: mTextFieldView];
  NSWindow* pWindow = [self window];
  [pWindow makeKeyAndOrderFront:nil];
  [pWindow makeFirstResponder: mTextFieldView];

  mEdControl = &control;
}

- (void) endUserInput
{
  [mTextFieldView setDelegate: nil];
  [mTextFieldView removeFromSuperview];

  NSWindow* pWindow = [self window];
  [pWindow makeFirstResponder: self];

  mTextFieldView = nullptr;
  mEdControl = nullptr;
}

//- (void) createWebView: (NSRect) areaRect : (const char*) url
//{
//  mWebView = [[WKWebView alloc] initWithFrame: areaRect ];
//  [self addSubview: mWebView];
//  [mWebView loadRequest: [NSURLRequest requestWithURL: [NSURL URLWithString:[NSString stringWithUTF8String:url]]]];
//}
//
//-(void)userContentController:(WKUserContentController *)userContentController didReceiveScriptMessage:(WKScriptMessage *)message
//{
//  NSLog(@"%@",message.body);
//}

- (NSString*) view: (NSView*) pView stringForToolTip: (NSToolTipTag) tag point: (NSPoint) point userData: (void*) pData
{
  int c = mGraphics ? GetMouseOver(mGraphics) : -1;
  if (c < 0) return @"";

  const char* tooltip = mGraphics->GetControl(c)->GetTooltip();
  return CStringHasContents(tooltip) ? ToNSString((const char*) tooltip) : @"";
}

- (void) registerToolTip: (IRECT&) bounds
{
  [self addToolTipRect: ToNSRect(mGraphics, bounds) owner: self userData: nil];
}

- (NSDragOperation)draggingEntered: (id <NSDraggingInfo>) sender
{
  NSPasteboard *pPasteBoard = [sender draggingPasteboard];

  if ([[pPasteBoard types] containsObject:NSFilenamesPboardType])
    return NSDragOperationGeneric;
  else
    return NSDragOperationNone;
}

- (BOOL)performDragOperation: (id<NSDraggingInfo>) sender
{
  NSPasteboard *pPasteBoard = [sender draggingPasteboard];

  if ([[pPasteBoard types] containsObject:NSFilenamesPboardType])
  {
    NSArray *pFiles = [pPasteBoard propertyListForType:NSFilenamesPboardType];
    NSString *pFirstFile = [pFiles firstObject];
    NSPoint point = [sender draggingLocation];
    NSPoint relativePoint = [self convertPoint: point fromView:nil];
    // TODO - fix or remove these values
    float x = relativePoint.x;// - 2.f;
    float y = relativePoint.y;// - 3.f;
    mGraphics->OnDrop([pFirstFile UTF8String], x, y);
  }

  return YES;
}

//- (void)windowResized:(NSNotification *)notification;
//{
//  if(!mGraphics)
//    return;
//
//  NSSize windowSize = [[self window] frame].size;
//  NSRect viewFrameInWindowCoords = [self convertRect: [self bounds] toView: nil];
//
//  float width = windowSize.width - viewFrameInWindowCoords.origin.x;
//  float height = windowSize.height - viewFrameInWindowCoords.origin.y;
//
//  float scaleX = width / mGraphics->Width();
//  float scaleY = height / mGraphics->Height();
//
//  if(mGraphics->GetUIResizerMode() == EUIResizerMode::kUIResizerScale)
//    mGraphics->Resize(width, height, mGraphics->GetDrawScale());
//  else // EUIResizerMode::kUIResizerSize
//    mGraphics->Resize(mGraphics->Width(), mGraphics->Height(), Clip(std::min(scaleX, scaleY), 0.1f, 10.f));
//}
//
//- (void)windowFullscreened:(NSNotification *)notification;
//{
//  NSSize windowSize = [[self window] frame].size;
//  NSRect viewFrameInWindowCoords = [self convertRect: [self bounds] toView: nil];
//
//  float width = windowSize.width - viewFrameInWindowCoords.origin.x;
//  float height = windowSize.height - viewFrameInWindowCoords.origin.y;
//
//  float scaleX = width / mGraphics->Width();
//  float scaleY = height / mGraphics->Height();
//
//  if(mGraphics->GetUIResizerMode() == EUIResizerMode::kUIResizerScale)
//    mGraphics->Resize(width, height, mGraphics->GetDrawScale());
//  else // EUIResizerMode::kUIResizerSize
//    mGraphics->Resize(mGraphics->Width(), mGraphics->Height(), Clip(std::min(scaleX, scaleY), 0.1f, 10.f));
//}

@end

#endif //NO_IGRAPHICS
