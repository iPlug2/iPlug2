#ifndef NO_IGRAPHICS

#import "IGraphicsMac_view.h"
#include "IControl.h"
#include "IPlugParameter.h"
#include "IPlugLogger.h"

@implementation IGRAPHICS_MENU_RCVR

- (NSMenuItem*)MenuItem
{
  return nsMenuItem;
}

- (void)OnMenuSelection:(id)sender
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
      nsMenuItem = [self addItemWithTitle:nsMenuItemTitle action:@selector(OnMenuSelection:) keyEquivalent:@""];

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

- (IPopupMenu*)AssociatedIPopupMenu
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
  [self setBoundsSize:NSMakeSize(pGraphics->WindowWidth(), pGraphics->WindowHeight())];
  
#ifdef IGRAPHICS_NANOVG
  if (!self.wantsLayer) {
    self.layer = (CALayer*) mGraphics->mLayer;
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
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [super dealloc];
}

- (BOOL) isOpaque
{
  return mGraphics ? YES : NO;
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
    {
      mGraphics->SetAllControlsDirty();
    }
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(windowResized:) name:NSWindowDidEndLiveResizeNotification
                                               object:pWindow];
  }
}

// not called for opengl/metal
- (void) drawRect: (NSRect) bounds
{
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
      IRECT tmpBounds = ToIRECT(mGraphics, &bounds);
      mGraphics->Draw(tmpBounds);
    }

  }
}

- (void) onTimer: (NSTimer*) pTimer
{
  IRECT r;
#ifdef IGRAPHICS_NANOVG
  mGraphics->BeginFrame();

  //TODO: this is redrawing every IControl!
  r.R = mGraphics->Width();
  r.B = mGraphics->Height();
//  if(mGraphics->IsDirty(r))
  mGraphics->IsDirty(r); // TEMP TO FORCE UPDATES ON ANIMATION
  mGraphics->Draw(r);

  mGraphics->EndFrame();
  [self setNeedsDisplay: YES];
#else
  if (/*pTimer == mTimer && mGraphics && */mGraphics->IsDirty(r))
  {
    [self setNeedsDisplayInRect:ToNSRect(mGraphics, r)];
  }
#endif
}

- (void) getMouseXY: (NSEvent*) pEvent x: (float*) pX y: (float*) pY
{
  if (mGraphics)
  {
    NSPoint pt = [self convertPoint:[pEvent locationInWindow] fromView:nil];
    *pX = pt.x - 2.f;
    *pY = mGraphics->WindowHeight() - pt.y - 3.f;
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
  if (mTextFieldView) [self endUserInput ];

  if (mGraphics)
  {
    IGraphics* graphics = mGraphics;
    mGraphics = nullptr;
    graphics->SetPlatformContext(nullptr);
    graphics->CloseWindow();
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
  IGRAPHICS_MENU_RCVR* dummyView = [[[IGRAPHICS_MENU_RCVR alloc] initWithFrame:bounds] autorelease];
  NSMenu* nsMenu = [[[IGRAPHICS_MENU alloc] initWithIPopupMenuAndReciever:&const_cast<IPopupMenu&>(menu) :dummyView] autorelease];

  NSWindow* pWindow = [self window];

  NSPoint wp = {bounds.origin.x, bounds.origin.y - 4};
  wp = [self convertPointToBase:wp];

  //fix position for retina display
  float displayScale = 1.0f;
  NSScreen* screen = [pWindow screen];
  if ([screen respondsToSelector: @selector (backingScaleFactor)])
    displayScale = screen.backingScaleFactor;
  wp.x /= displayScale;
  wp.y /= displayScale;

  NSEvent* event = [NSEvent otherEventWithType:NSApplicationDefined
                  location:wp
                  modifierFlags:NSApplicationDefined
                  timestamp: (NSTimeInterval) 0
                  windowNumber: [pWindow windowNumber]
                  context: [NSGraphicsContext currentContext]
                    subtype:0
                    data1: 0
                    data2: 0];

  [NSMenu popUpContextMenu:nsMenu withEvent:event forView:dummyView];

  NSMenuItem* chosenItem = [dummyView MenuItem];
  NSMenu* chosenMenu = [chosenItem menu];
  IPopupMenu* associatedIPopupMenu = [(IGRAPHICS_MENU*) chosenMenu AssociatedIPopupMenu];

  long chosenItemIdx = [chosenMenu indexOfItem: chosenItem];

  if (chosenItemIdx > -1 && associatedIPopupMenu)
  {
    associatedIPopupMenu->SetChosenItemIdx((int) chosenItemIdx);
    return associatedIPopupMenu;
  }
  else return nullptr;
}

- (void) createTextEntry: (IControl&) control : (const IText&) text : (const char*) str : (NSRect) areaRect;
{
  if (mTextFieldView)
    return;

  mTextFieldView = [[NSTextField alloc] initWithFrame: areaRect];
  
  //TODO: this is wrong
#ifdef IGRAPHICS_NANOVG
  NSString* font = [NSString stringWithUTF8String: "Arial"];
#else
  NSString* font = [NSString stringWithUTF8String: text.mFont];
#endif
  
  [mTextFieldView setFont: [NSFont fontWithName:font size: (float) AdjustFontSize(text.mSize)]];

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

  mTextFieldView = 0;
  mEdControl = 0;
}

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

- (void) viewDidChangeBackingProperties:(NSNotification *) notification
{
  NSWindow* pWindow = [self window];

  if (!pWindow)
    return;

  CGFloat newScale = [pWindow backingScaleFactor];

  if (newScale != mGraphics->GetDisplayScale())
  {
    mGraphics->SetDisplayScale(newScale);
  }
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
    float x = relativePoint.x - 2.f;
    float y = mGraphics->Height() - relativePoint.y - 3.f;
    mGraphics->OnDrop([pFirstFile UTF8String], x, y);
  }

  return YES;
}

- (void)windowResized:(NSNotification *)notification;
{
  NSSize windowSize = [[self window] frame].size;
  NSRect viewFrameInWindowCoords = [self convertRect: [self bounds] toView: nil];

  float width = windowSize.width - viewFrameInWindowCoords.origin.x;
  float height = viewFrameInWindowCoords.origin.y + viewFrameInWindowCoords.size.height;
  
  mGraphics->OnResizeGesture(0., height);
}

@end

#endif //NO_IGRAPHICS
