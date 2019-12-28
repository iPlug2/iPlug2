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

#include "wdlutf8.h"

#import "IGraphicsMac_view.h"
#include "IControl.h"
#include "IPlugParameter.h"
#include "IPlugLogger.h"

using namespace iplug;
using namespace igraphics;

static int MacKeyCodeToVK(int code)
{
  switch (code)
  {
    case 51: return kVK_BACK;
    case 65: return kVK_DECIMAL;
    case 67: return kVK_MULTIPLY;
    case 69: return kVK_ADD;
    case 71: return kVK_NUMLOCK;
    case 75: return kVK_DIVIDE;
    case 76: return kVK_RETURN | 0x8000;
    case 78: return kVK_SUBTRACT;
    case 81: return kVK_SEPARATOR;
    case 82: return kVK_NUMPAD0;
    case 83: return kVK_NUMPAD1;
    case 84: return kVK_NUMPAD2;
    case 85: return kVK_NUMPAD3;
    case 86: return kVK_NUMPAD4;
    case 87: return kVK_NUMPAD5;
    case 88: return kVK_NUMPAD6;
    case 89: return kVK_NUMPAD7;
    case 91: return kVK_NUMPAD8;
    case 92: return kVK_NUMPAD9;
    case 96: return kVK_F5;
    case 97: return kVK_F6;
    case 98: return kVK_F7;
    case 99: return kVK_F3;
    case 100: return kVK_F8;
    case 101: return kVK_F9;
    case 109: return kVK_F10;
    case 103: return kVK_F11;
    case 111: return kVK_F12;
    case 114: return kVK_INSERT;
    case 115: return kVK_HOME;
    case 117: return kVK_DELETE;
    case 116: return kVK_PRIOR;
    case 118: return kVK_F4;
    case 119: return kVK_END;
    case 120: return kVK_F2;
    case 121: return kVK_NEXT;
    case 122: return kVK_F1;
    case 123: return kVK_LEFT;
    case 124: return kVK_RIGHT;
    case 125: return kVK_DOWN;
    case 126: return kVK_UP;
    case 0x69: return kVK_F13;
    case 0x6B: return kVK_F14;
    case 0x71: return kVK_F15;
    case 0x6A: return kVK_F16;
  }
  return kVK_NONE;
}

static int MacKeyEventToVK(NSEvent* pEvent, int& flag)
{
  int code = kVK_NONE;
  
  const NSInteger mod = [pEvent modifierFlags];
  
  if (mod & NSShiftKeyMask) flag |= kFSHIFT;
  if (mod & NSCommandKeyMask) flag |= kFCONTROL; // todo: this should be command once we figure it out
  if (mod & NSAlternateKeyMask) flag |= kFALT;
  
  int rawcode = [pEvent keyCode];
  
  code = MacKeyCodeToVK(rawcode);
  if (code == kVK_NONE)
  {
    NSString *str = NULL;
    
    if (!str || ![str length]) str = [pEvent charactersIgnoringModifiers];
    
    if (!str || ![str length])
    {
      if (!code)
      {
        code = 1024 + rawcode; // raw code
        flag |= kFVIRTKEY;
      }
    }
    else
    {
      code = [str characterAtIndex:0];
      if (code >= NSF1FunctionKey && code <= NSF24FunctionKey)
      {
        flag |= kFVIRTKEY;
        code += kVK_F1 - NSF1FunctionKey;
      }
      else
      {
        if (code >= 'a' && code <= 'z') code += 'A'-'a';
        if (code == 25 && (flag & FSHIFT)) code = kVK_TAB;
        if (isalnum(code) || code==' ' || code == '\r' || code == '\n' || code ==27 || code == kVK_TAB) flag |= kFVIRTKEY;
      }
    }
  }
  else
  {
    flag |= kFVIRTKEY;
    if (code == 8) code = '\b';
  }
  
  if (!(flag & kFVIRTKEY)) flag &= ~kFSHIFT;
  
  return code;
}

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
      [self addItem:[NSMenuItem separatorItem]];
    else
    {
      nsMenuItem = [self addItemWithTitle:nsMenuItemTitle action:@selector(onMenuSelection:) keyEquivalent:@""];
      
      [nsMenuItem setTarget:pView];
      
      if (pMenuItem->GetIsTitle ())
        [nsMenuItem setIndentationLevel:1];

      if (pMenuItem->GetChecked())
        [nsMenuItem setState:NSOnState];
      else
        [nsMenuItem setState:NSOffState];

      if (pMenuItem->GetEnabled())
        [nsMenuItem setEnabled:YES];
      else
        [nsMenuItem setEnabled:NO];

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

@implementation IGRAPHICS_TEXTFIELD

- (bool) becomeFirstResponder;
{
  bool success = [super becomeFirstResponder];
  if (success)
  {
    NSTextView *textField = (NSTextView*) [self currentEditor];
    if( [textField respondsToSelector: @selector(setInsertionPointColor:)] )
      [textField setInsertionPointColor: [self textColor]];
  }
  return success;
}

@end

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

@implementation IGRAPHICS_GLLAYER

- (id) initWithIGraphicsView: (IGRAPHICS_VIEW*) pView;
{
  mView = pView;
  
  self = [super init];
  if ( self != nil )
  {
    self.needsDisplayOnBoundsChange = YES;
    self.asynchronous = NO;
  }
  
  return self;
}

- (NSOpenGLContext *)openGLContextForPixelFormat:(NSOpenGLPixelFormat *)pixelFormat
{
  NSOpenGLContext* context = [super openGLContextForPixelFormat: pixelFormat];
  
  [context makeCurrentContext];
  
  if(!mView->mGraphics->GetDrawContext())
    mView->mGraphics->ContextReady(self);
  
  return context;
}

- (NSOpenGLPixelFormat *)openGLPixelFormatForDisplayMask:(uint32_t)mask
{
  NSOpenGLPixelFormatAttribute profile = NSOpenGLProfileVersionLegacy;
  #if defined IGRAPHICS_GL3
    profile = (NSOpenGLPixelFormatAttribute)NSOpenGLProfileVersion3_2Core;
  #endif
  
  const NSOpenGLPixelFormatAttribute kAttributes[] =  {
    NSOpenGLPFAAccelerated,
    NSOpenGLPFANoRecovery,
    NSOpenGLPFADoubleBuffer,
    NSOpenGLPFAAlphaSize, 8,
    NSOpenGLPFAColorSize, 24,
    NSOpenGLPFADepthSize, 0,
    NSOpenGLPFAStencilSize, 8,
    NSOpenGLPFAOpenGLProfile, profile,
    (NSOpenGLPixelFormatAttribute) 0
  };

  return [[NSOpenGLPixelFormat alloc] initWithAttributes:kAttributes];
}

//- (BOOL)canDrawInOpenGLContext:(NSOpenGLContext *)context pixelFormat:(NSOpenGLPixelFormat *)pixelFormat forLayerTime:(CFTimeInterval)timeInterval displayTime:(const CVTimeStamp *)timeStamp
//{
//}

- (void)drawInOpenGLContext:(NSOpenGLContext *)context pixelFormat:(NSOpenGLPixelFormat *)pixelFormat forLayerTime:(CFTimeInterval)timeInterval displayTime:(const CVTimeStamp *)timeStamp
{
  [mView render];
}

@end

#pragma mark -

extern StaticStorage<CoreTextFontDescriptor> sFontDescriptorCache;

@implementation IGRAPHICS_VIEW

- (id) initWithIGraphics: (IGraphicsMac*) pGraphics
{
  TRACE

  mGraphics = pGraphics;
  NSRect r = NSMakeRect(0.f, 0.f, (float) pGraphics->WindowWidth(), (float) pGraphics->WindowHeight());
  self = [super initWithFrame:r];
  
#if defined IGRAPHICS_NANOVG || defined IGRAPHICS_SKIA
  if (!self.wantsLayer) {
    #if defined IGRAPHICS_METAL
    self.layer = [CAMetalLayer new];
    [(CAMetalLayer*)[self layer] setPixelFormat:MTLPixelFormatBGRA8Unorm];
    #elif defined IGRAPHICS_GL
    self.layer = [[IGRAPHICS_GLLAYER alloc] initWithIGraphicsView:self];
    self.wantsBestResolutionOpenGLSurface = YES;
    #endif
    self.layer.opaque = YES;
    self.wantsLayer = YES;
  }
#endif
  
  [self registerForDraggedTypes:[NSArray arrayWithObjects: NSFilenamesPboardType, nil]];

  double sec = 1.0 / (double) pGraphics->FPS();
  mTimer = [NSTimer timerWithTimeInterval:sec target:self selector:@selector(onTimer:) userInfo:nil repeats:YES];
  [[NSRunLoop currentRunLoop] addTimer: mTimer forMode: (NSString*) kCFRunLoopCommonModes];

  return self;
}

- (void)dealloc
{
  if([NSColorPanel sharedColorPanelExists])
    [[NSColorPanel sharedColorPanel] close];
  
  mColorPickerFunc = nullptr;
  [mMoveCursor release];
  [mTrackingArea release];
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
    
    CGFloat newScale = [pWindow backingScaleFactor];

    if (mGraphics)
      mGraphics->SetScreenScale(newScale);
    
    #ifdef IGRAPHICS_METAL
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(frameDidChange:)
                                                 name:NSViewFrameDidChangeNotification
                                               object:self];
    #endif
    
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
  
  mGraphics->SetPlatformContext(nullptr);
  
  if (newScale != mGraphics->GetScreenScale())
    mGraphics->SetScreenScale(newScale);

#if defined IGRAPHICS_GL
  self.layer.contentsScale = 1./newScale;
#elif defined IGRAPHICS_METAL
  [(CAMetalLayer*)[self layer] setDrawableSize:CGSizeMake(self.frame.size.width * newScale,
                                                          self.frame.size.height * newScale)];
#endif
}

- (CGContextRef) getCGContextRef
{
  CGContextRef pCGC = [NSGraphicsContext currentContext].CGContext;
  return [NSGraphicsContext graphicsContextWithCGContext: pCGC flipped: YES].CGContext;
}

// not called for layer backed views
- (void) drawRect: (NSRect) bounds
{
  if (mGraphics)
  {
    if (!mGraphics->GetPlatformContext())
      mGraphics->SetPlatformContext([self getCGContextRef]);
      
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
}

- (void) render
{
#if !defined IGRAPHICS_GL && !defined IGRAPHICS_METAL // for layer-backed views setNeedsDisplayInRect/drawRect is not called
  for (int i = 0; i < mDirtyRects.Size(); i++)
    [self setNeedsDisplayInRect:ToNSRect(mGraphics, mDirtyRects.Get(i))];
#else
  // so just draw on each frame, if something is dirty
  mGraphics->Draw(mDirtyRects);
#endif
}

- (void) onTimer: (NSTimer*) pTimer
{
  mDirtyRects.Clear();
  
  if (mGraphics->IsDirty(mDirtyRects))
  {
    mGraphics->SetAllControlsClean();
      
#ifdef IGRAPHICS_GL
    [self.layer setNeedsDisplay];
#else
    [self render];
#endif
  }
}

- (void) getMouseXY: (NSEvent*) pEvent x: (float&) pX y: (float&) pY
{
  if (mGraphics)
  {
    NSPoint pt = [self convertPoint:[pEvent locationInWindow] fromView:nil];
    pX = pt.x / mGraphics->GetDrawScale();
    pY = pt.y / mGraphics->GetDrawScale();
   
    mGraphics->DoCursorLock(pX, pY, mPrevX, mPrevY);
    mGraphics->SetTabletInput(pEvent.subtype == NSTabletPointEventSubtype);
  }
}

- (IMouseInfo) getMouseLeft: (NSEvent*) pEvent
{
  IMouseInfo info;
  [self getMouseXY:pEvent x:info.x y:info.y];
  int mods = (int) [pEvent modifierFlags];
  info.ms = IMouseMod(true, (mods & NSCommandKeyMask), (mods & NSShiftKeyMask), (mods & NSControlKeyMask), (mods & NSAlternateKeyMask));

  return info;
}

- (IMouseInfo) getMouseRight: (NSEvent*) pEvent
{
  IMouseInfo info;
  [self getMouseXY:pEvent x:info.x y:info.y];
  int mods = (int) [pEvent modifierFlags];
  info.ms = IMouseMod(false, true, (mods & NSShiftKeyMask), (mods & NSControlKeyMask), (mods & NSAlternateKeyMask));

  return info;
}

- (void) updateTrackingAreas
{
  [super updateTrackingAreas]; // This is needed to get mouseEntered and mouseExited
    
  if (mTrackingArea != nil) {
      [self removeTrackingArea:mTrackingArea];
    [mTrackingArea release];
  }
    
  int opts = (NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways);
  mTrackingArea = [ [NSTrackingArea alloc] initWithRect:[self bounds] options:opts owner:self userInfo:nil];
  [self addTrackingArea:mTrackingArea];
}

- (void) mouseEntered: (NSEvent *)event
{
  mGraphics->OnSetCursor();
}

- (void) mouseExited: (NSEvent *)event
{
  mGraphics->OnMouseOut();
}

- (void) mouseDown: (NSEvent*) pEvent
{
  IMouseInfo info = [self getMouseLeft:pEvent];
  if (mGraphics)
  {
    if (([pEvent clickCount] - 1) % 2)
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
  // Cache previous values before retrieving the new mouse position (which will update them)
  float prevX = mPrevX;
  float prevY = mPrevY;
  IMouseInfo info = [self getMouseLeft:pEvent];
  if (mGraphics && !mGraphics->IsInTextEntry())
    mGraphics->OnMouseDrag(info.x, info.y, info.x - prevX, info.y - prevY, info.ms);
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
  // Cache previous values before retrieving the new mouse position (which will update them)
  float prevX = mPrevX;
  float prevY = mPrevY;
  IMouseInfo info = [self getMouseRight:pEvent];

  if (mGraphics && !mTextFieldView)
    mGraphics->OnMouseDrag(info.x, info.y, info.x - prevX, info.y - prevY, info.ms);
}

- (void) mouseMoved: (NSEvent*) pEvent
{
  IMouseInfo info = [self getMouseLeft:pEvent];
  if (mGraphics)
    mGraphics->OnMouseOver(info.x, info.y, info.ms);
}

- (void)keyDown: (NSEvent *)pEvent
{
  int flag = 0;
  int code = MacKeyEventToVK(pEvent, flag);
  NSString *s = [pEvent charactersIgnoringModifiers];

  unichar c = 0;
  
  if ([s length] == 1)
    c = [s characterAtIndex:0];
  
  if(!static_cast<bool>(flag & kFVIRTKEY))
  {
    code = kVK_NONE;
  }
  
  char utf8[5];
  WDL_MakeUTFChar(utf8, c, 4);
  
  IKeyPress keyPress {utf8, code, static_cast<bool>(flag & kFSHIFT),
                                  static_cast<bool>(flag & kFCONTROL),
                                  static_cast<bool>(flag & kFALT)};
  
  bool handle = mGraphics->OnKeyDown(mPrevX, mPrevY, keyPress);
  
  if (!handle)
  {
    [[self nextResponder] keyDown:pEvent];
  }
}

- (void)keyUp: (NSEvent *)pEvent
{
  int flag = 0;
  int code = MacKeyEventToVK(pEvent, flag);
  NSString *s = [pEvent charactersIgnoringModifiers];
  
  unichar c = 0;
  
  if ([s length] == 1)
    c = [s characterAtIndex:0];
  
  if(!static_cast<bool>(flag & kFVIRTKEY))
  {
    code = kVK_NONE;
  }
  
  char utf8[5];
  WDL_MakeUTFChar(utf8, c, 4);
  
  IKeyPress keyPress {utf8, code, static_cast<bool>(flag & kFSHIFT),
                                                  static_cast<bool>(flag & kFCONTROL),
                                                  static_cast<bool>(flag & kFALT)};
  
  bool handle = mGraphics->OnKeyUp(mPrevX, mPrevY, keyPress);
  
  if (!handle)
  {
    [[self nextResponder] keyUp:pEvent];
  }
}

- (void) scrollWheel: (NSEvent*) pEvent
{
  if (mTextFieldView) [self endUserInput ];
  IMouseInfo info = [self getMouseLeft:pEvent];
  float d = [pEvent deltaY];
  if (mGraphics)
    mGraphics->OnMouseWheel(info.x, info.y, info.ms, d);
}

static void MakeCursorFromName(NSCursor*& cursor, const char *name)
{
  // get paths and intialise images etc.
  const char* basePath = "/System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks/HIServices.framework/Versions/A/Resources/cursors/";
  
  NSString* imagePath = [NSString stringWithFormat:@"%s%s/cursor.pdf", basePath, name];
  NSString* infoPath = [NSString stringWithFormat:@"file:%s%s/info.plist", basePath, name];
  NSImage* fileImage = [[NSImage alloc] initByReferencingFile: imagePath];
  NSImage *cursorImage = [[NSImage alloc] initWithSize:[fileImage size]];
  NSDictionary* info = [NSDictionary dictionaryWithContentsOfURL:[NSURL URLWithString:infoPath]];
  
  // get info from dictionary
  double hotX = [info[@"hotx-scaled"] doubleValue];
  double hotY = [info[@"hoty-scaled"] doubleValue];
  double blur = [info[@"blur"] doubleValue];
  double offsetX = [info[@"shadowoffsetx"] doubleValue];
  double offsetY = [info[@"shadowoffsety"] doubleValue];
  double red = [info[@"shadowcolor"][0] doubleValue];
  double green = [info[@"shadowcolor"][1] doubleValue];
  double blue = [info[@"shadowcolor"][2] doubleValue];
  double alpha = [info[@"shadowcolor"][3] doubleValue];
  CGColorRef shadowColor = CGColorCreateGenericRGB(red, green, blue, alpha);
  
  for (int scale = 1; scale <= 4; scale++)
  {
    // scale
    NSAffineTransform* xform = [NSAffineTransform transform];
    [xform scaleBy:scale];
    id hints = @{ NSImageHintCTM: xform };
    CGImageRef rasterCGImage = [fileImage CGImageForProposedRect:NULL context:nil hints:hints];
    
    // apply shadow
    size_t width = CGImageGetWidth(rasterCGImage);
    size_t height = CGImageGetHeight(rasterCGImage);
    CGSize offset = CGSize { static_cast<CGFloat>(offsetX * scale), static_cast<CGFloat>(offsetY * scale) };
    CGContextRef shadowContext = CGBitmapContextCreate(NULL, width, height, CGImageGetBitsPerComponent(rasterCGImage), 0, CGImageGetColorSpace(rasterCGImage), CGImageGetBitmapInfo(rasterCGImage));
    CGContextSetShadowWithColor(shadowContext, offset, blur * scale, shadowColor);
    CGContextDrawImage(shadowContext, CGRectMake(0, 0, width, height), rasterCGImage);
    CGImageRef shadowCGImage = CGBitmapContextCreateImage(shadowContext);
    
    // add to cursor inmge
    NSBitmapImageRep *rep = [[NSBitmapImageRep alloc] initWithCGImage:shadowCGImage];
    [rep setSize:[fileImage size]];
    [cursorImage addRepresentation:rep];
    
    // release
    [rep release];
    CGContextRelease(shadowContext);
    CGImageRelease(shadowCGImage);
  }
  
  // create cursor
  cursor = [[NSCursor alloc] initWithImage:cursorImage hotSpot:NSMakePoint(hotX, hotY)];
  
  // release
  [cursorImage release];
  [fileImage release];
  CGColorRelease(shadowColor);
}

- (void) setMouseCursor: (ECursor) cursorType
{
  NSCursor* pCursor = nullptr;
  
  bool helpCurrent = false;
  bool helpRequested = false;
    
  switch (cursorType)
  {
    case ECursor::ARROW: pCursor = [NSCursor arrowCursor]; break;
    case ECursor::IBEAM: pCursor = [NSCursor IBeamCursor]; break;
    case ECursor::WAIT:
      if ([NSCursor respondsToSelector:@selector(busyButClickableCursor)])
        pCursor = [NSCursor performSelector:@selector(busyButClickableCursor)];
      break;
    case ECursor::CROSS: pCursor = [NSCursor crosshairCursor]; break;
    case ECursor::UPARROW:
      if ([NSCursor respondsToSelector:@selector(_windowResizeNorthCursor)])
          pCursor = [NSCursor performSelector:@selector(_windowResizeNorthCursor)];
      else
          pCursor = [NSCursor resizeUpCursor];
      break;
    case ECursor::SIZENWSE:
      if ([NSCursor respondsToSelector:@selector(_windowResizeNorthWestSouthEastCursor)])
        pCursor = [NSCursor performSelector:@selector(_windowResizeNorthWestSouthEastCursor)];
      break;
    case ECursor::SIZENESW:
      if ([NSCursor respondsToSelector:@selector(_windowResizeNorthEastSouthWestCursor)])
        pCursor = [NSCursor performSelector:@selector(_windowResizeNorthEastSouthWestCursor)];
      break;
    case ECursor::SIZEWE:
      if ([NSCursor respondsToSelector:@selector(_windowResizeEastWestCursor)])
        pCursor = [NSCursor performSelector:@selector(_windowResizeEastWestCursor)];
      else
        pCursor = [NSCursor resizeLeftRightCursor];
      break;
    case ECursor::SIZENS:
      if ([NSCursor respondsToSelector:@selector(_windowResizeNorthSouthCursor)])
        pCursor = [NSCursor performSelector:@selector(_windowResizeNorthSouthCursor)];
      else
        pCursor = [NSCursor resizeUpDownCursor];
      break;
    case ECursor::SIZEALL:
    {
      if (!mMoveCursor)
        MakeCursorFromName(mMoveCursor, "move");
      pCursor = mMoveCursor;
      break;
    }
    case ECursor::INO: pCursor = [NSCursor operationNotAllowedCursor]; break;
    case ECursor::HAND: pCursor = [NSCursor pointingHandCursor]; break;
    case ECursor::APPSTARTING:
      if ([NSCursor respondsToSelector:@selector(busyButClickableCursor)])
        pCursor = [NSCursor performSelector:@selector(busyButClickableCursor)];
       break;
    case ECursor::HELP:
      if ([NSCursor respondsToSelector:@selector(_helpCursor)])
        pCursor = [NSCursor performSelector:@selector(_helpCursor)];
      helpRequested = true;
      break;
    default: pCursor = [NSCursor arrowCursor]; break;
  }

  if ([NSCursor respondsToSelector:@selector(helpCursorShown)])
    helpCurrent = [NSCursor performSelector:@selector(helpCursorShown)];
    
  if (helpCurrent && !helpRequested)
  {
    // N.B. - suppress warnings for this call only
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wobjc-method-access"
    [NSCursor _setHelpCursor : false];
#pragma clang diagnostic pop
  }
    
  if (!pCursor)
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
  
  mGraphics->SetPlatformContext(nullptr);
    
  //For some APIs (AUv2) this is where we know about the window being closed, close via delegate
  mGraphics->GetDelegate()->CloseWindow();
  [super removeFromSuperview];
}

- (void) controlTextDidEndEditing: (NSNotification*) aNotification
{
  char* txt = (char*)[[mTextFieldView stringValue] UTF8String];

  mGraphics->SetControlValueAfterTextEdit(txt);
  mGraphics->SetAllControlsDirty();

  [self endUserInput ];
  [self setNeedsDisplay: YES];
}

- (IPopupMenu*) createPopupMenu: (IPopupMenu&) menu : (NSRect) bounds;
{
  IGRAPHICS_MENU_RCVR* pDummyView = [[[IGRAPHICS_MENU_RCVR alloc] initWithFrame:bounds] autorelease];
  NSMenu* pNSMenu = [[[IGRAPHICS_MENU alloc] initWithIPopupMenuAndReciever:&menu : pDummyView] autorelease];
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

- (void) createTextEntry: (int) paramIdx : (const IText&) text : (const char*) str : (int) length : (NSRect) areaRect;
{
  if (mTextFieldView)
    return;

  mTextFieldView = [[IGRAPHICS_TEXTFIELD alloc] initWithFrame: areaRect];
  
  if (text.mVAlign == EVAlign::Middle)
  {
    IGRAPHICS_TEXTFIELDCELL* pCell = [[IGRAPHICS_TEXTFIELDCELL alloc] initTextCell:@"textfield"];
    [mTextFieldView setCell: pCell];
    [mTextFieldView setEditable: TRUE];
    [mTextFieldView setDrawsBackground: TRUE];
  }

  CoreTextFontDescriptor* CTFontDescriptor = CoreTextHelpers::GetCTFontDescriptor(text, sFontDescriptorCache);
  NSFontDescriptor* fontDescriptor = (NSFontDescriptor*) CTFontDescriptor->GetDescriptor();
  NSFont* font = [NSFont fontWithDescriptor: fontDescriptor size: text.mSize * 0.75];
  [mTextFieldView setFont: font];
  
  switch (text.mAlign)
  {
    case EAlign::Near:
      [mTextFieldView setAlignment: NSLeftTextAlignment];
      break;
    case EAlign::Center:
      [mTextFieldView setAlignment: NSCenterTextAlignment];
      break;
    case EAlign::Far:
      [mTextFieldView setAlignment: NSRightTextAlignment];
      break;
    default:
      break;
  }

  const IParam* pParam = paramIdx > kNoParameter ? mGraphics->GetDelegate()->GetParam(paramIdx) : nullptr;

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
    [[mTextFieldView formatter] setMaximumLength:length];
    [characterSet release];
  }

  [[mTextFieldView cell] setLineBreakMode: NSLineBreakByTruncatingTail];
  [mTextFieldView setAllowsEditingTextAttributes:NO];
  [mTextFieldView setTextColor:ToNSColor(text.mTextEntryFGColor)];
  [mTextFieldView setBackgroundColor:ToNSColor(text.mTextEntryBGColor)];

  [mTextFieldView setStringValue: [NSString stringWithCString:str encoding:NSUTF8StringEncoding]];

#ifndef COCOA_TEXTENTRY_BORDERED
  [mTextFieldView setBordered: NO];
  [mTextFieldView setFocusRingType:NSFocusRingTypeNone];
#endif

  [mTextFieldView setDelegate: self];

  [self addSubview: mTextFieldView];
  NSWindow* pWindow = [self window];
  [pWindow makeKeyAndOrderFront:nil];
  [pWindow makeFirstResponder: mTextFieldView];
}

- (void) endUserInput
{
  [mTextFieldView setDelegate: nil];
  [mTextFieldView removeFromSuperview];

  NSWindow* pWindow = [self window];
  [pWindow makeFirstResponder: self];

  mTextFieldView = nullptr;
}

- (BOOL) promptForColor: (IColor&) color : (IColorPickerHandlerFunc) func;
{
  NSColorPanel* colorPanel = [NSColorPanel sharedColorPanel];
  mColorPickerFunc = func;

  [colorPanel setTarget:self];
  [colorPanel setShowsAlpha: TRUE];
  [colorPanel setAction:@selector(onColorPicked:)];
  [colorPanel setColor:ToNSColor(color)];
  [colorPanel orderFront:nil];

  return colorPanel != nil;
}

- (void) onColorPicked: (NSColorPanel*) colorPanel
{
  mColorPickerFunc(FromNSColor(colorPanel.color));
}

- (NSString*) view: (NSView*) pView stringForToolTip: (NSToolTipTag) tag point: (NSPoint) point userData: (void*) pData
{
  int c = mGraphics ? GetMouseOver(mGraphics) : -1;
  if (c < 0) return @"";

  const char* tooltip = mGraphics->GetControl(c)->GetTooltip();
  return CStringHasContents(tooltip) ? [NSString stringWithCString:tooltip encoding:NSUTF8StringEncoding] : @"";
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

#ifdef IGRAPHICS_METAL
- (void)frameDidChange:(NSNotification*)notification
{
  CGFloat scale = [[self window] backingScaleFactor];

  [(CAMetalLayer*)[self layer] setDrawableSize:CGSizeMake(self.frame.size.width * scale,
                                                          self.frame.size.height * scale)];
}
#endif

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
//  if(mGraphics->GetUIResizerMode() == EUIResizerMode::Scale)
//    mGraphics->Resize(width, height, mGraphics->GetDrawScale());
//  else // EUIResizerMode::Size
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
//  if(mGraphics->GetUIResizerMode() == EUIResizerMode::Scale)
//    mGraphics->Resize(width, height, mGraphics->GetDrawScale());
//  else // EUIResizerMode::Size
//    mGraphics->Resize(mGraphics->Width(), mGraphics->Height(), Clip(std::min(scaleX, scaleY), 0.1f, 10.f));
//}

@end

#ifdef IGRAPHICS_IMGUI

@implementation IGRAPHICS_IMGUIVIEW
{
}

- (id) initWithIGraphicsView: (IGRAPHICS_VIEW*) pView;
{
  mView = pView;
  self = [super initWithFrame:[pView frame] device: MTLCreateSystemDefaultDevice()];
  if(self) {
    _commandQueue = [self.device newCommandQueue];
    self.layer.opaque = NO;
  }
  
  return self;
}

- (void)drawRect:(NSRect)dirtyRect
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
