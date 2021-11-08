/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#import <Cocoa/Cocoa.h>

#if defined IGRAPHICS_GL
#import <QuartzCore/QuartzCore.h>
#endif

#include "IGraphicsMac.h"
#include "IGraphicsStructs.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

inline NSRect ToNSRect(IGraphics* pGraphics, const IRECT& bounds)
{
  const float scale = pGraphics->GetDrawScale();
  const float x = floor(bounds.L * scale);
  const float y = floor(bounds.T * scale);
  const float x2 = ceil(bounds.R * scale);
  const float y2 = ceil(bounds.B * scale);
    
  return NSMakeRect(x, y, x2 - x, y2 - y);
}

inline IRECT ToIRECT(IGraphics* pGraphics, const NSRect* pNSRect)
{
  const float scale = 1.f/pGraphics->GetDrawScale();
  const float x = pNSRect->origin.x;
  const float y = pNSRect->origin.y;
  const float w = pNSRect->size.width;
  const float h = pNSRect->size.height;
  
  return IRECT(x * scale, y * scale, (x + w) * scale, (y + h) * scale);
}

inline NSColor* ToNSColor(const IColor& c)
{
  return [NSColor colorWithDeviceRed:(double) c.R / 255.0 green:(double) c.G / 255.0 blue:(double) c.B / 255.0 alpha:(double) c.A / 255.0];
}

inline IColor FromNSColor(const NSColor* c)
{
  return IColor(c.alphaComponent * 255., c.redComponent* 255., c.greenComponent * 255., c.blueComponent * 255.);
}

inline int GetMouseOver(IGraphicsMac* pGraphics)
{
  return pGraphics->GetMouseOver();
}

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

// based on code by Scott Gruby http://blog.gruby.com/2008/03/30/filtering-nstextfield-take-2/
@interface IGRAPHICS_FORMATTER : NSFormatter
{
  NSCharacterSet* filterCharacterSet;
  int maxLength;
  int maxValue;
}

- (void) setAcceptableCharacterSet: (NSCharacterSet*) pCharacterSet;
- (void) setMaximumLength:(int) inLength;
- (void) setMaximumValue:(int) inValue;

@end

@interface IGRAPHICS_TEXTFIELDCELL : NSTextFieldCell
{
  BOOL mIsEditingOrSelecting;
}

@end

using namespace iplug;
using namespace igraphics;

@interface IGRAPHICS_MENU : NSMenu
{
  IPopupMenu* mIPopupMenu;
}
- (id) initWithIPopupMenuAndReceiver: (IPopupMenu*) pMenu : (NSView*) pView;
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

@interface IGRAPHICS_TEXTFIELD : NSTextField
{
}
- (bool) becomeFirstResponder;
@end

#ifdef IGRAPHICS_GL
#define VIEW_BASE NSOpenGLView
#else
#define VIEW_BASE NSView
#endif

@interface IGRAPHICS_VIEW : VIEW_BASE <NSTextFieldDelegate/*, WKScriptMessageHandler*/>
{
  CVDisplayLinkRef mDisplayLink;
  dispatch_source_t mDisplaySource;
  NSTimer* mTimer;
  
  NSTrackingArea* mTrackingArea;
  IGRAPHICS_TEXTFIELD* mTextFieldView;
  NSCursor* mMoveCursor;
  float mPrevX, mPrevY;
  bool mMouseOutDuringDrag;
  IRECTList mDirtyRects;
  IColorPickerHandlerFunc mColorPickerFunc;
@public
  IGraphicsMac* mGraphics; // OBJC instance variables have to be pointers
}
- (id) initWithIGraphics: (IGraphicsMac*) pGraphics;
- (BOOL) isOpaque;
- (BOOL) acceptsFirstResponder;
- (BOOL) acceptsFirstMouse: (NSEvent*) pEvent;
- (void) viewDidMoveToWindow;
- (void) viewDidChangeBackingProperties: (NSNotification*) pNotification;
- (void) drawRect: (NSRect) bounds;
- (void) render;
- (void) killTimer;
- (void) onTimer: (NSTimer*) pTimer;
- (void) viewDidChangeEffectiveAppearance;
//mouse
- (void) getMouseXY: (NSEvent*) pEvent : (float&) x : (float&) y;
- (IMouseInfo) getMouseLeft: (NSEvent*) pEvent;
- (IMouseInfo) getMouseRight: (NSEvent*) pEvent;
- (void) updateTrackingAreas;
- (void) mouseEntered:(NSEvent*) pEvent;
- (void) mouseExited:(NSEvent*) pEvent;
- (void) mouseDown: (NSEvent*) pEvent;
- (void) mouseUp: (NSEvent*) pEvent;
- (void) mouseDragged: (NSEvent*) pEvent;
- (void) rightMouseDown: (NSEvent*) pEvent;
- (void) rightMouseUp: (NSEvent*) pEvent;
- (void) rightMouseDragged: (NSEvent*) pEvent;
- (void) mouseMoved: (NSEvent*) pEvent;
- (void) scrollWheel: (NSEvent*) pEvent;
- (void) keyDown: (NSEvent*) pEvent;
- (void) keyUp: (NSEvent*) pEvent;
//text entry
- (void) removeFromSuperview;
- (void) controlTextDidEndEditing: (NSNotification*) pNotification;
- (void) createTextEntry: (int) paramIdx : (const IText&) text : (const char*) str : (int) length : (NSRect) areaRect;
- (void) endUserInput;
//pop-up menu
- (IPopupMenu*) createPopupMenu: (IPopupMenu&) menu : (NSRect) bounds;
//color picker
- (BOOL) promptForColor: (IColor&) color : (IColorPickerHandlerFunc) func;
- (void) onColorPicked: (NSColorPanel*) pColorPanel;

//tooltip
- (NSString*) view: (NSView*) pView stringForToolTip: (NSToolTipTag) tag point: (NSPoint) point userData: (void*) pData;
- (void) registerToolTip: (IRECT&) bounds;
//drag-and-drop
- (NSDragOperation) draggingEntered: (id <NSDraggingInfo>) sender;
- (BOOL) performDragOperation: (id<NSDraggingInfo>) sender;
//
- (void) setMouseCursor: (ECursor) cursorType;
@end

#if defined IGRAPHICS_IMGUI
#import <MetalKit/MetalKit.h>

@interface IGRAPHICS_IMGUIVIEW : MTKView
{
  IGRAPHICS_VIEW* mView;
}
@property (nonatomic, strong) id <MTLCommandQueue> commandQueue;
- (id) initWithIGraphicsView: (IGRAPHICS_VIEW*) pView;
@end
#endif

