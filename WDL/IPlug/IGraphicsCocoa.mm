#include "IGraphicsCocoa.h"

@implementation DUMMY_COCOA_VIEW

- (NSMenuItem*)MenuItem
{
    return nsMenuItem;
}

- (void)OnMenuSelection:(id)sender
{
    nsMenuItem = sender;
}

@end

NSString* ToNSString(const char* cStr)
{
	return [NSString stringWithCString:cStr encoding:NSUTF8StringEncoding];
}

inline IMouseMod GetMouseMod(NSEvent* pEvent)
{
  int mods = [pEvent modifierFlags];
  return IMouseMod(true, (mods & NSCommandKeyMask), (mods & NSShiftKeyMask), (mods & NSControlKeyMask), (mods & NSAlternateKeyMask));
}

inline IMouseMod GetRightMouseMod(NSEvent* pEvent)
{
  int mods = [pEvent modifierFlags];
  return IMouseMod(false, true, (mods & NSShiftKeyMask), (mods & NSControlKeyMask), (mods & NSAlternateKeyMask));
}

#ifdef RTAS_API
// super hack - used to find the ProTools Edit window and forward key events there
static WindowRef FindNamedCarbonWindow(WindowClass wcl, const char *s, bool exact)
{
  WindowRef tref = GetFrontWindowOfClass(wcl,FALSE);
	for(int i = 0; tref; ++i) {
		char buf[256];
		CFStringRef cfstr;
		CopyWindowTitleAsCFString(tref,&cfstr);
		if(cfstr && CFStringGetCString(cfstr,buf,sizeof buf-1,kCFStringEncodingASCII)) 
		{
      if(exact? 
         strcmp(buf,s) == 0: 
         strstr(buf,s) != NULL
         ) break;
		}
		tref = GetNextWindowOfClass(tref,wcl,FALSE);
	} 
	return tref;
}
#endif

@implementation IGRAPHICS_COCOA

- (id) init
{
  TRACE;
  
  mGraphics = 0;
  mTimer = 0;
  return self;
}

- (id) initWithIGraphics: (IGraphicsMac*) pGraphics
{
  TRACE;
   
  mGraphics = pGraphics;
  NSRect r;
  r.origin.x = r.origin.y = 0.0f;
  r.size.width = (float) pGraphics->Width();
  r.size.height = (float) pGraphics->Height();
  self = [super initWithFrame:r];

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
  if (pWindow) {
    [pWindow makeFirstResponder: self];
    [pWindow setAcceptsMouseMovedEvents: YES];
  }
}

- (void) drawRect: (NSRect) rect 
{
	if (mGraphics)
	{
		IRECT tmpRect = ToIRECT(mGraphics, &rect);
		if (mGraphics) mGraphics->Draw(&tmpRect);

	}
}

- (void) onTimer: (NSTimer*) pTimer
{
  IRECT r;
  if (pTimer == mTimer && mGraphics && mGraphics->IsDirty(&r)) {
    [self setNeedsDisplayInRect:ToNSRect(mGraphics, &r)];
  }
}

- (void) getMouseXY: (NSEvent*) pEvent x: (int*) pX y: (int*) pY
{
	if (mGraphics)
	{
		NSPoint pt = [self convertPoint:[pEvent locationInWindow] fromView:nil];
		*pX = (int) pt.x - 2;
		*pY = mGraphics->Height() - (int) pt.y - 3;
	}
}

- (void) mouseDown: (NSEvent*) pEvent
{
	if (mGraphics)
	{
		int x, y;
		[self getMouseXY:pEvent x:&x y:&y];
		IMouseMod ms = GetMouseMod(pEvent);
    
    #ifdef RTAS_API
    if (ms.L && ms.R && ms.C && ms.A && (mGraphics->GetParamIdxForPTAutomation(x, y) > -1)) 
    {
      WindowRef carbonParent = (WindowRef) [[[self window] parentWindow] windowRef];
      EventRef carbonEvent = (EventRef) [pEvent eventRef];
      SendEventToWindow(carbonEvent, carbonParent);
      return;
    }
    #endif

		if ([pEvent clickCount] > 1) {
			mGraphics->OnMouseDblClick(x, y, &ms);
		}
		else {
			mGraphics->OnMouseDown(x, y, &ms);
		}
	}
}

- (void) mouseUp: (NSEvent*) pEvent
{
	if (mGraphics)
	{
		int x, y;
		[self getMouseXY:pEvent x:&x y:&y];
		IMouseMod ms = GetMouseMod(pEvent);
		mGraphics->OnMouseUp(x, y, &ms);
	}
}

- (void) mouseDragged: (NSEvent*) pEvent
{
	if (mGraphics)
	{
	  int x, y;
	  [self getMouseXY:pEvent x:&x y:&y];
	  IMouseMod ms = GetMouseMod(pEvent);
		
	if(!mTextFieldView) 
		mGraphics->OnMouseDrag(x, y, &ms);

	}
}

- (void) rightMouseDown: (NSEvent*) pEvent
{
	if (mGraphics)
	{  
		int x, y;
		[self getMouseXY:pEvent x:&x y:&y];
		IMouseMod ms = GetRightMouseMod(pEvent);
		mGraphics->OnMouseDown(x, y, &ms);
	}
}

- (void) rightMouseUp: (NSEvent*) pEvent
{
	if (mGraphics)
	{  
		int x, y;
		[self getMouseXY:pEvent x:&x y:&y];
		IMouseMod ms = GetRightMouseMod(pEvent);
		mGraphics->OnMouseUp(x, y, &ms);
	}
}

- (void) rightMouseDragged: (NSEvent*) pEvent
{
	if (mGraphics)
	{
		int x, y;
		[self getMouseXY:pEvent x:&x y:&y];
		IMouseMod ms = GetRightMouseMod(pEvent);
		
		if(!mTextFieldView) 
			mGraphics->OnMouseDrag(x, y, &ms);
	}
}

- (void) mouseMoved: (NSEvent*) pEvent
{
	if (mGraphics)
	{
	  int x, y;
	  [self getMouseXY:pEvent x:&x y:&y];
	
	  IMouseMod ms = GetMouseMod(pEvent);
	  mGraphics->OnMouseOver(x, y, &ms);
	}
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
    
    //if (k == 49) key = KEY_SPACE;
    //else if (k == 125) key = KEY_UPARROW;
    if (k == 125) key = KEY_UPARROW;
    else if (k == 126) key = KEY_DOWNARROW;
    else if (k == 123) key = KEY_LEFTARROW;
    else if (k == 124) key = KEY_RIGHTARROW;
    else if (c >= '0' && c <= '9') key = KEY_DIGIT_0+c-'0';
    else if (c >= 'A' && c <= 'Z') key = KEY_ALPHA_A+c-'A';
    else if (c >= 'a' && c <= 'z') key = KEY_ALPHA_A+c-'a';
    else handle = false;
    
    if (handle) {
      int x, y;
      [self getMouseXY:pEvent x:&x y:&y];
      mGraphics->OnKeyDown(x, y, key);
    }
    else {
#ifdef RTAS_API
      // TODO: fix this super hack - and why does it work when there is no edit window... is it hidden?
      WindowRef root = FindNamedCarbonWindow(kAllWindowClasses, "Edit:" , false);
      ActivateWindow(root, true);
      EventRef carbonEvent = (EventRef) [pEvent eventRef];
      SendEventToWindow(carbonEvent, root);
#else
      [[self nextResponder] keyDown:pEvent];
#endif
      //mGraphics->ForwardKeyEventToHost();
    }
  }
} 

- (void) scrollWheel: (NSEvent*) pEvent
{	
	if (mGraphics)
	{
		if(mTextFieldView) [self endUserInput ];

	  int x, y;
	  [self getMouseXY:pEvent x:&x y:&y];
	  int d = [pEvent deltaY];
		
	  IMouseMod ms = GetMouseMod(pEvent);
	  mGraphics->OnMouseWheel(x, y, &ms, d);
	}
}

- (void) killTimer
{
  [mTimer invalidate];
  mTimer = 0;
}

- (void) removeFromSuperview
{
  if (mTextFieldView) [self endUserInput ];;
  if (mGraphics)
  {
    IGraphics* graphics = mGraphics;
    mGraphics = 0;
    graphics->CloseWindow();
  }
}

- (void) controlTextDidEndEditing: (NSNotification*) aNotification
{
	char* txt = (char*)[[mTextFieldView stringValue] UTF8String];
	
  if (mEdParam) 
    mGraphics->SetFromStringAfterPrompt(mEdControl, mEdParam, txt);
  else
    mEdControl->TextFromTextEntry(txt);

  [self endUserInput ];
  [self setNeedsDisplay: YES];		
}

- (IPopupMenu*) createIPopupMenu: (IPopupMenu*) pMenu: (NSRect) rect;
{	
	//Sanity Check... cocoa menu variables start with ns, IPlug menu variables don't
	
	NSMenu* nsMenu = [[[NSMenu alloc] initWithTitle:@"IPlug Popup Menu"] autorelease]; //TODO: title?
	DUMMY_COCOA_VIEW* nsView = [[[DUMMY_COCOA_VIEW alloc] initWithFrame:rect] autorelease];
	NSMenuItem* nsMenuItem;
	NSMutableString* nsMenuItemTitle;
  
  [nsMenu setAutoenablesItems:NO];
	
	int numItems = pMenu->GetNItems();

	for (int i = 0; i < numItems; ++i)
	{	
		IPopupMenuItem* menuItem = pMenu->GetItem(i);
		
		nsMenuItemTitle = [[[NSMutableString alloc] initWithCString:menuItem->GetText() encoding:NSUTF8StringEncoding] autorelease];
		
		if ( pMenu->GetPrefix() )
		{
			NSString* prefixString = 0;
			
			switch ( pMenu->GetPrefix() )
			{
				case 0: prefixString = [NSString stringWithFormat:@"", i+1]; break;
				case 1:	prefixString = [NSString stringWithFormat:@"%1d: ", i+1]; break;
				case 2: prefixString = [NSString stringWithFormat:@"%02d: ", i+1]; break;
				case 3: prefixString = [NSString stringWithFormat:@"%03d: ", i+1]; break;
			}
			
			[nsMenuItemTitle insertString:prefixString atIndex:0];
		}
		
		if ( menuItem->GetSubmenu() )
		{
//			item = [nsMenu addItemWithTitle:nsMenuItemTitle action:nil keyEquivalent:@""];
//			NSMenu* subMenu = [[menuClass alloc] performSelector:@selector(initWithOptionMenu:) withObject:(id)item->getSubmenu ()];
//			[nsMenu setSubmenu: subMenu forItem:nsItem];
		}
		else if (menuItem->GetIsSeparator())
		{
			[nsMenu addItem:[NSMenuItem separatorItem]];
		}
		else
		{
			nsMenuItem = [nsMenu addItemWithTitle:nsMenuItemTitle action:@selector(OnMenuSelection:) keyEquivalent:@""];
			
			if (menuItem->GetIsTitle ())
				[nsMenuItem setIndentationLevel:1];
			
			//[item setTarget:nsMenu];
			
			if (menuItem->GetChecked())
				[nsMenuItem setState:NSOnState];
			else
				[nsMenuItem setState:NSOffState];
      
      if (menuItem->GetEnabled())
        [nsMenuItem setEnabled:YES];
      else {
        [nsMenuItem setEnabled:NO];
      }

		}
	}
	
	NSWindow* pWindow = [self window];
//NSRect frame = [pWindow frame];
  
	NSPoint wp = {rect.origin.x, rect.origin.y - 5}; // TODO: fix -5 bodge
	wp = [self convertPointToBase:wp];
	
	NSEvent* event = [NSEvent otherEventWithType:NSApplicationDefined
										location:wp
								   modifierFlags:NSApplicationDefined 
									   timestamp: (NSTimeInterval) 0
									windowNumber: [pWindow windowNumber]
										 context: [NSGraphicsContext currentContext]
										 subtype:0
										   data1: 0
										   data2: 0]; 
	
	[NSMenu popUpContextMenu:nsMenu withEvent:event forView:nsView];      
	NSMenuItem* chosenItem = [nsView MenuItem];
	
	int chosenItemIdx = [nsMenu indexOfItem: chosenItem];
	
	if (chosenItemIdx > -1)
	{
		pMenu->SetChosenItemIdx(chosenItemIdx);
		return pMenu;
	}
	else return 0;

}

- (void) createTextEntry: (IControl*) pControl: (IParam*) pParam: (IText*) pText: (const char*) pString: (NSRect) areaRect;
{
  if (!pControl || mTextFieldView) return;

  mTextFieldView = [[NSTextField alloc] initWithFrame: areaRect];
//  [mTextFieldView setFont: [NSFont fontWithName: @"Monaco" size: 9.]];
  NSString* font = [NSString stringWithUTF8String: pText->mFont];
  [mTextFieldView setFont: [NSFont fontWithName:font size: (float) AdjustFontSize(pText->mSize)]];

  switch ( pText->mAlign ) 
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
  
  [[mTextFieldView cell] setLineBreakMode: NSLineBreakByTruncatingTail];
  //[[mTextFieldView cell] setDrawsBackground:NO];
  [mTextFieldView setAllowsEditingTextAttributes:NO];
  [mTextFieldView setFocusRingType:NSFocusRingTypeNone];
  [mTextFieldView setTextColor:ToNSColor(&pText->mColor)];
  
  //if ([mTextFieldView frame].size.height > areaRect.size.height)
  
  [mTextFieldView setStringValue: ToNSString(pString)];
  [mTextFieldView setBordered: NO];
  [mTextFieldView setDelegate: self];
  //[mTextFieldView setAutoresizingMask:NSViewMinYMargin];
  //[mTextFieldView sizeToFit];

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

@end
