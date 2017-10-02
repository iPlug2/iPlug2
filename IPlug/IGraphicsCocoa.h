#import <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>
#include <AudioUnit/AudioUnit.h>
#import <AudioUnit/AUCocoaUIView.h>
#include "IGraphicsMac.h"

inline NSRect ToNSRect(IGraphics* pGraphics, IRECT* pR)
{
  int B = pGraphics->Height() - pR->B;
  return NSMakeRect(pR->L, B-1, pR->W()+1, pR->H()+1);
}

inline IRECT ToIRECT(IGraphics* pGraphics, NSRect* pR)
{
  int x = pR->origin.x, y = pR->origin.y, w = pR->size.width, h = pR->size.height, gh = pGraphics->Height();
  return IRECT(x, gh - (y + h), x + w, gh - y);
}

inline NSColor* ToNSColor(IColor* pColor)
{
  double r = (double) pColor->R / 255.0;
  double g = (double) pColor->G / 255.0;
  double b = (double) pColor->B / 255.0;
  double a = (double) pColor->A / 255.0;
  return [NSColor colorWithCalibratedRed:r green:g blue:b alpha:a];
}

NSString* ToNSString(const char* cStr);

// based on code by Scott Gruby http://blog.gruby.com/2008/03/30/filtering-nstextfield-take-2/
@interface COCOA_FORMATTER : NSFormatter
{
  NSCharacterSet *filterCharacterSet;
  int maxLength;
  int maxValue;
}

- (void) setAcceptableCharacterSet:(NSCharacterSet *) inCharacterSet;
- (void) setMaximumLength:(int) inLength;
- (void) setMaximumValue:(int) inValue;

@end

@interface IGRAPHICS_NSMENU : NSMenu
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

@interface IGRAPHICS_COCOA : NSView
{
  NSTimer* mTimer;
  NSTextField* mTextFieldView;
  IControl* mEdControl; // the control linked to the open text edit
  IParam* mEdParam; // the param linked to the open text edit (optional)
  int mPrevX, mPrevY;
@public
  IGraphicsMac* mGraphics;
}
- (id) init;
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
//- (void) controlTextDidChange: (NSNotification *) aNotification;
- (void) controlTextDidEndEditing: (NSNotification*) aNotification;
- (IPopupMenu*) createIPopupMenu: (IPopupMenu*) pMenu : (NSRect) rect;
- (void) createTextEntry: (IControl*) pControl : (IParam*) pParam : (IText*) pText : (const char*) pString : (NSRect) areaRect;
- (void) endUserInput;
- (NSString*) view: (NSView*) pView stringForToolTip: (NSToolTipTag) tag point: (NSPoint) point userData: (void*) pData;
- (void) registerToolTip: (IRECT*) pRECT;
@end
