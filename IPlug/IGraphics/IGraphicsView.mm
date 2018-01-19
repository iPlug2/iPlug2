#import "IGraphicsView.h"

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

  int numItems = pMenu->GetNItems();

  for (int i = 0; i < numItems; ++i)
  {
    IPopupMenuItem* menuItem = pMenu->GetItem(i);

    nsMenuItemTitle = [[[NSMutableString alloc] initWithCString:menuItem->GetText() encoding:NSUTF8StringEncoding] autorelease];

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

    if (menuItem->GetSubmenu())
    {
      nsMenuItem = [self addItemWithTitle:nsMenuItemTitle action:nil keyEquivalent:@""];
      NSMenu* subMenu = [[IGRAPHICS_MENU alloc] initWithIPopupMenuAndReciever:menuItem->GetSubmenu() :pView];
      [self setSubmenu: subMenu forItem:nsMenuItem];
      [subMenu release];
    }
    else if (menuItem->GetIsSeparator())
    {
      [self addItem:[NSMenuItem separatorItem]];
    }
    else
    {
      nsMenuItem = [self addItemWithTitle:nsMenuItemTitle action:@selector(OnMenuSelection:) keyEquivalent:@""];

      if (menuItem->GetIsTitle ())
      {
        [nsMenuItem setIndentationLevel:1];
      }

      if (menuItem->GetChecked())
      {
        [nsMenuItem setState:NSOnState];
      }
      else
      {
        [nsMenuItem setState:NSOffState];
      }
      
      if (menuItem->GetEnabled())
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

//- (id) init
//{
//  TRACE;
//
//  mGraphics = nullptr;
//  mTimer = nullptr;
//  mPrevX = 0;
//  mPrevY = 0;
//  return self;
//}

- (id) initWithIGraphics: (IGraphicsMac*) pGraphics
{
  TRACE;

  mGraphics = pGraphics;
  NSRect r;
  r.origin.x = r.origin.y = 0.0f;
  r.size.width = (float) pGraphics->WindowWidth();
  r.size.height = (float) pGraphics->WindowHeight();
  self = [super initWithFrame:r];
  [self setBoundsSize:NSMakeSize(pGraphics->Width(), pGraphics->Height())];

#ifdef IGRAPHICS_NANOVG
  if (!self.wantsLayer) {
    self.layer = (CALayer*) mGraphics->mLayer;
    self.layer.opaque = NO;
    self.wantsLayer = YES;
  }
#endif
  
  [self registerForDraggedTypes:[NSArray arrayWithObjects: NSFilenamesPboardType, nil]];
  
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
  }
}

// not called for opengl/metal
- (void) drawRect: (NSRect) rect
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
      IRECT tmpRect = ToIRECT(mGraphics, &rect);
      mGraphics->Draw(tmpRect);
    }
    
  }
}

- (void) onTimer: (NSTimer*) pTimer
{
  IRECT r;
#ifdef IGRAPHICS_NANOVG
  mGraphics->BeginFrame();
  
  //TODO: this is redrawing every IControl!
  r.R = mGraphics->WindowWidth();
  r.B = mGraphics->WindowHeight();
//  if(mGraphics->IsDirty(r))
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
    *pY = mGraphics->Height() - pt.y - 3.f;
    mPrevX = *pX;
    mPrevY = *pY;
  }
}

- (MouseInfo) getMouseLeft: (NSEvent*) pEvent
{
  MouseInfo info;
  [self getMouseXY:pEvent x:&info.x y:&info.y];
  int mods = (int) [pEvent modifierFlags];
  info.ms = IMouseMod(true, (mods & NSCommandKeyMask), (mods & NSShiftKeyMask), (mods & NSControlKeyMask), (mods & NSAlternateKeyMask));
  return info;
}

- (MouseInfo) getMouseRight: (NSEvent*) pEvent
{
  MouseInfo info;
  [self getMouseXY:pEvent x:&info.x y:&info.y];
  int mods = (int) [pEvent modifierFlags];
  info.ms = IMouseMod(false, true, (mods & NSShiftKeyMask), (mods & NSControlKeyMask), (mods & NSAlternateKeyMask));

  return info;
}

- (void) mouseDown: (NSEvent*) pEvent
{
  MouseInfo info = [self getMouseLeft:pEvent];
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
  MouseInfo info = [self getMouseLeft:pEvent];
  if (mGraphics)
    mGraphics->OnMouseUp(info.x, info.y, info.ms);
}

- (void) mouseDragged: (NSEvent*) pEvent
{
  MouseInfo info = [self getMouseLeft:pEvent];
  float dX = [pEvent deltaX];
  float dY = [pEvent deltaY];
  if (mGraphics && !mTextFieldView)
    mGraphics->OnMouseDrag(info.x, info.y, dX, dY, info.ms);
}

- (void) rightMouseDown: (NSEvent*) pEvent
{
  MouseInfo info = [self getMouseRight:pEvent];
  if (mGraphics)
    mGraphics->OnMouseDown(info.x, info.y, info.ms);
}

- (void) rightMouseUp: (NSEvent*) pEvent
{
  MouseInfo info = [self getMouseRight:pEvent];
  if (mGraphics)
    mGraphics->OnMouseUp(info.x, info.y, info.ms);
}

- (void) rightMouseDragged: (NSEvent*) pEvent
{
  MouseInfo info = [self getMouseRight:pEvent];
  float dX = [pEvent deltaX];
  float dY = [pEvent deltaY];
  if (mGraphics && !mTextFieldView)
    mGraphics->OnMouseDrag(info.x, info.y, dX, dY, info.ms);
}

- (void) mouseMoved: (NSEvent*) pEvent
{
  MouseInfo info = [self getMouseLeft:pEvent];
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
    int key;

    if (k == 49) key = KEY_SPACE;
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
  if (mGraphics)
  {
    if(mTextFieldView) [self endUserInput ];

    MouseInfo info = [self getMouseLeft:pEvent];
    float d = [pEvent deltaY];

    mGraphics->OnMouseWheel(info.x, info.y, info.ms, d);
  }
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

  if (mEdParam)
  {
    mGraphics->SetFromStringAfterPrompt(mEdControl, mEdParam, txt);
  }
  else
  {
    mEdControl->TextFromTextEntry(txt);
  }
  
  mGraphics->SetAllControlsDirty();
  
  [self endUserInput ];
  [self setNeedsDisplay: YES];
}

- (IPopupMenu*) createIPopupMenu: (IPopupMenu&) menu : (NSRect) rect;
{
  IGRAPHICS_MENU_RCVR* dummyView = [[[IGRAPHICS_MENU_RCVR alloc] initWithFrame:rect] autorelease];
  NSMenu* nsMenu = [[[IGRAPHICS_MENU alloc] initWithIPopupMenuAndReciever:&menu :dummyView] autorelease];

  NSWindow* pWindow = [self window];

  NSPoint wp = {rect.origin.x, rect.origin.y - 4};
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
  else return 0;
}

- (void) createTextEntry: (IControl*) pControl : (IParam*) pParam : (const IText&) text : (const char*) str : (NSRect) areaRect;
{
  if (mTextFieldView) return;

  mTextFieldView = [[NSTextField alloc] initWithFrame: areaRect];
  NSString* font = [NSString stringWithUTF8String: text.mFont];
  [mTextFieldView setFont: [NSFont fontWithName:font size: (float) AdjustFontSize(text.mSize)]];

  switch ( text.mAlign )
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
    [[mTextFieldView formatter] setMaximumLength:pControl->GetTextEntryLength()];
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
  
  [mTextFieldView setDelegate: (id<NSTextFieldDelegate>) self];
  
  [self addSubview: mTextFieldView];
  NSWindow* pWindow = [self window];
  [pWindow makeKeyAndOrderFront:nil];
  [pWindow makeFirstResponder: mTextFieldView];

  mEdParam = pParam; // might be 0
  mEdControl = pControl;
}

- (void) endUserInput
{
  [mTextFieldView setDelegate: nil];
  [mTextFieldView removeFromSuperview];

  NSWindow* pWindow = [self window];
  [pWindow makeFirstResponder: self];

  mTextFieldView = 0;
  mEdControl = 0;
  mEdParam = 0;
}

- (NSString*) view: (NSView*) pView stringForToolTip: (NSToolTipTag) tag point: (NSPoint) point userData: (void*) pData
{
  int c = mGraphics ? GetMouseOver(mGraphics) : -1;
  if (c < 0) return @"";
  
  const char* tooltip = mGraphics->GetControl(c)->GetTooltip();
  return CSTR_NOT_EMPTY(tooltip) ? ToNSString((const char*) tooltip) : @"";
}

- (void) registerToolTip: (IRECT&) rect
{
  [self addToolTipRect: ToNSRect(mGraphics, rect) owner: self userData: nil];
}

- (void)viewDidChangeBackingProperties:(NSNotification *) notification
{
#if GRAPHICS_SCALING
  NSWindow* pWindow = [self window];
  CGFloat newBackingScaleFactor = [pWindow backingScaleFactor];
  CGFloat oldBackingScaleFactor = [[[notification userInfo] objectForKey:@"NSBackingPropertyOldScaleFactorKey"] doubleValue];
  
  if(!pWindow)
    return;
  
  if (newBackingScaleFactor != oldBackingScaleFactor)
  {
    mGraphics->SetDisplayScale(newBackingScaleFactor);
  }
#endif
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
    int x = (int) relativePoint.x - 2;
    int y = mGraphics->Height() - (int) relativePoint.y - 3;
    mGraphics->OnDrop([pFirstFile UTF8String], x, y);
  }
  
  return YES;
}


@end
