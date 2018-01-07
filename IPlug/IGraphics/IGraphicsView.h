#import <Cocoa/Cocoa.h>
#include "IGraphicsMac.h"

inline NSRect ToNSRect(IGraphics* pGraphics, const IRECT& rect)
{
  int B = pGraphics->Height() - rect.B;
  return NSMakeRect(rect.L, B, rect.W(), rect.H());
}

inline IRECT ToIRECT(IGraphics* pGraphics, NSRect* pR)
{
  int x = pR->origin.x, y = pR->origin.y, w = pR->size.width, h = pR->size.height, gh = pGraphics->Height();
  return IRECT(x, gh - (y + h), x + w, gh - y);
}

inline NSColor* ToNSColor(const IColor& c)
{
  return [NSColor colorWithCalibratedRed:(double) c.R / 255.0 green:(double) c.G / 255.0 blue:(double) c.B / 255.0 alpha:(double) c.A / 255.0];
}

NSString* ToNSString(const char* cStr);

// based on code by Scott Gruby http://blog.gruby.com/2008/03/30/filtering-nstextfield-take-2/
@interface IGRAPHICS_FORMATTER : NSFormatter
{
  NSCharacterSet *filterCharacterSet;
  int maxLength;
  int maxValue;
}

- (void) setAcceptableCharacterSet:(NSCharacterSet *) inCharacterSet;
- (void) setMaximumLength:(int) inLength;
- (void) setMaximumValue:(int) inValue;

@end

@interface IGRAPHICS_MENU : NSMenu
{
  IPopupMenu* mIPopupMenu;
}
- (id) initWithIPopupMenuAndReciever:(IPopupMenu*)pMenu : (NSView*)pView;
- (IPopupMenu*) AssociatedIPopupMenu;
@end

// Dummy view class used to receive Menu Events inline
@interface IGRAPHICS_MENU_RCVR : NSView
{
  NSMenuItem* nsMenuItem;
}
- (void) OnMenuSelection:(id)sender;
- (NSMenuItem*) MenuItem;
@end

@interface IGRAPHICS_VIEW : NSView
{
  NSTimer* mTimer;
  NSTextField* mTextFieldView;
  IControl* mEdControl; // the control linked to the open text edit
  IParam* mEdParam; // the param linked to the open text edit (optional)
  int mPrevX, mPrevY;
@public
  IGraphicsMac* mGraphics; // OBJC instance variables have to be pointers
}
//- (id) init;
- (id) initWithIGraphics: (IGraphicsMac*) pGraphics;
- (BOOL) isOpaque;
- (BOOL) acceptsFirstResponder;
- (BOOL) acceptsFirstMouse: (NSEvent*) pEvent;
- (void) viewDidMoveToWindow;
- (void) drawRect: (NSRect) rect;
- (void) onTimer: (NSTimer*) pTimer;
- (void) getMouseXY: (NSEvent*) pEvent x: (int*) pX y: (int*) pY;
- (void) mouseDown: (NSEvent*) pEvent;
- (void) mouseUp: (NSEvent*) pEvent;
- (void) mouseDragged: (NSEvent*) pEvent;
- (void) rightMouseDown: (NSEvent*) pEvent;
- (void) rightMouseUp: (NSEvent*) pEvent;
- (void) rightMouseDragged: (NSEvent*) pEvent;
- (void) mouseMoved: (NSEvent*) pEvent;
- (void) scrollWheel: (NSEvent*) pEvent;
- (void) keyDown: (NSEvent *)pEvent;
- (void) killTimer;
- (void) removeFromSuperview;
- (void) controlTextDidEndEditing: (NSNotification*) aNotification;
- (IPopupMenu*) createIPopupMenu: (IPopupMenu&) menu : (NSRect) rect;
- (void) createTextEntry: (IControl*) pControl : (IParam*) pParam : (const IText&) text : (const char*) str : (NSRect) areaRect;
- (void) endUserInput;
- (NSString*) view: (NSView*) pView stringForToolTip: (NSToolTipTag) tag point: (NSPoint) point userData: (void*) pData;
- (void) registerToolTip: (IRECT&) rect;
- (void) viewDidChangeBackingProperties:(NSNotification *) notification;
- (NSDragOperation) draggingEntered: (id <NSDraggingInfo>) sender;
- (BOOL) performDragOperation: (id<NSDraggingInfo>) sender;
@end
