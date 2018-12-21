/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#ifndef NO_IGRAPHICS

#import <Cocoa/Cocoa.h>
//#import <WebKit/WebKit.h>

#include "IGraphicsMac.h"

#if defined IGRAPHICS_GL
#include <OpenGL/gl.h>
#endif

inline NSRect ToNSRect(IGraphics* pGraphics, const IRECT& bounds)
{
  float scale = pGraphics->GetDrawScale();
  float x = floor(bounds.L * scale);
  float y = floor(bounds.T * scale);
  float x2 = ceil(bounds.R * scale);
  float y2 = ceil(bounds.B * scale);
    
  return NSMakeRect(x, y, x2 - x, y2 - y);
}

inline IRECT ToIRECT(IGraphics* pGraphics, const NSRect* pR)
{
  float scale = 1.f/pGraphics->GetDrawScale();
  float x = pR->origin.x, y = pR->origin.y, w = pR->size.width, h = pR->size.height;
  return IRECT(x * scale, y * scale, (x + w) * scale, (y + h) * scale);
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

@interface IGRAPHICS_TEXTFIELDCELL : NSTextFieldCell
{
  BOOL mIsEditingOrSelecting;
}

@end

@interface IGRAPHICS_MENU : NSMenu
{
  IPopupMenu* mIPopupMenu;
}
- (id) initWithIPopupMenuAndReciever:(IPopupMenu*)pMenu : (NSView*)pView;
- (IPopupMenu*) iPopupMenu;
@end

// Dummy view class used to receive Menu Events inline
@interface IGRAPHICS_MENU_RCVR : NSView
{
  NSMenuItem* nsMenuItem;
}
- (void) onMenuSelection:(id)sender;
- (NSMenuItem*) menuItem;
@end

@interface IGRAPHICS_VIEW : NSView <NSTextFieldDelegate/*, WKScriptMessageHandler*/>
{
#ifdef IGRAPHICS_GL
  NSOpenGLContext* mContext;
  NSOpenGLPixelFormat* mPixelFormat;
#endif
  
  NSTimer* mTimer;
  NSTextField* mTextFieldView;
//  WKWebView* mWebView;
  IControl* mEdControl; // the control linked to the open text edit
  float mPrevX, mPrevY;
@public
  IGraphicsMac* mGraphics; // OBJC instance variables have to be pointers
}
//- (id) init;
- (id) initWithIGraphics: (IGraphicsMac*) pGraphics;
- (BOOL) isOpaque;
- (BOOL) acceptsFirstResponder;
- (BOOL) acceptsFirstMouse: (NSEvent*) pEvent;
- (void) viewDidMoveToWindow;
- (void) viewDidChangeBackingProperties:(NSNotification *) notification;
- (void) drawRect: (NSRect) bounds;
- (void) onTimer: (NSTimer*) pTimer;
- (void) render;
- (void) killTimer;
//mouse
- (void) getMouseXY: (NSEvent*) pEvent x: (float*) pX y: (float*) pY;
- (IMouseInfo) getMouseLeft: (NSEvent*) pEvent;
- (IMouseInfo) getMouseRight: (NSEvent*) pEvent;
- (void) mouseDown: (NSEvent*) pEvent;
- (void) mouseUp: (NSEvent*) pEvent;
- (void) mouseDragged: (NSEvent*) pEvent;
- (void) rightMouseDown: (NSEvent*) pEvent;
- (void) rightMouseUp: (NSEvent*) pEvent;
- (void) rightMouseDragged: (NSEvent*) pEvent;
- (void) mouseMoved: (NSEvent*) pEvent;
- (void) scrollWheel: (NSEvent*) pEvent;
- (void) keyDown: (NSEvent *)pEvent;
//text entry
- (void) removeFromSuperview;
- (void) controlTextDidEndEditing: (NSNotification*) aNotification;
- (void) createTextEntry: (IControl&) control : (const IText&) text : (const char*) str : (NSRect) areaRect;
- (void) endUserInput;
//web view
//- (void) createWebView: (NSRect) areaRect : (const char*) url;
//- (void) userContentController:didReceiveScriptMessage;
//pop-up menu
- (IPopupMenu*) createPopupMenu: (const IPopupMenu&) menu : (NSRect) bounds;
//tooltip
- (NSString*) view: (NSView*) pView stringForToolTip: (NSToolTipTag) tag point: (NSPoint) point userData: (void*) pData;
- (void) registerToolTip: (IRECT&) bounds;
//drag-and-drop
- (NSDragOperation) draggingEntered: (id <NSDraggingInfo>) sender;
- (BOOL) performDragOperation: (id<NSDraggingInfo>) sender;
//
- (void) setMouseCursor: (ECursor) cursor;
@end

#endif //NO_IGRAPHICS
