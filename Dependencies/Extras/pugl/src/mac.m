// Copyright 2012-2023 David Robillard <d@drobilla.net>
// Copyright 2017 Hanspeter Portner <dev@open-music-kontrollers.ch>
// SPDX-License-Identifier: ISC

#define GL_SILENCE_DEPRECATION 1

#include "mac.h"

#include "internal.h"
#include "platform.h"

#include "pugl/pugl.h"

#import <Cocoa/Cocoa.h>

#include <mach/mach_time.h>

#include <stdlib.h>

#ifndef __MAC_10_10
typedef NSUInteger NSEventModifierFlags;
typedef NSUInteger NSEventSubtype;
#endif

#ifndef __MAC_10_12
typedef NSUInteger NSWindowStyleMask;
#endif

typedef struct {
  const char* uti;
  const char* mimeType;
} Datatype;

#define NUM_DATATYPES 16

static const Datatype datatypes[NUM_DATATYPES + 1] = {
  {"com.apple.pasteboard.promised-file-url", "text/uri-list"},
  {"org.7-zip.7-zip-archive", "application/x-7z-compressed"},
  {"org.gnu.gnu-zip-tar-archive", "application/tar+gzip"},
  {"public.7z-archive", "application/x-7z-compressed"},
  {"public.cpio-archive", "application/x-cpio"},
  {"public.deb-archive", "application/vnd.debian.binary-package"},
  {"public.file-url", "text/uri-list"},
  {"public.html", "text/html"},
  {"public.png", "image/png"},
  {"public.rar-archive", "application/x-rar-compressed"},
  {"public.rpm-archive", "application/x-rpm"},
  {"public.rtf", "text/rtf"},
  {"public.url", "text/uri-list"},
  {"public.utf8-plain-text", "text/plain"},
  {"public.utf8-tab-separated-values-text", "text/tab-separated-values"},
  {"public.xz-archive", "application/x-xz"},
  {NULL, NULL},
};

static NSString*
mimeTypeForUti(const NSString* const uti)
{
  const char* const utiString = [uti UTF8String];

  // First try internal map to override types the system won't convert sensibly
  for (const Datatype* datatype = datatypes; datatype->uti; ++datatype) {
    if (!strcmp(utiString, datatype->uti)) {
      return [NSString stringWithUTF8String:datatype->mimeType];
    }
  }

  // Try to get the MIME type from the system
  return (NSString*)CFBridgingRelease(UTTypeCopyPreferredTagWithClass(
    (__bridge CFStringRef)uti, kUTTagClassMIMEType));
}

static NSString*
utiForMimeType(const NSString* const mimeType)
{
  const char* const mimeTypeString = [mimeType UTF8String];

  // First try internal map to override types the system won't convert sensibly
  for (const Datatype* datatype = datatypes; datatype->mimeType; ++datatype) {
    if (!strcmp(mimeTypeString, datatype->mimeType)) {
      return [NSString stringWithUTF8String:datatype->uti];
    }
  }

  // Try to get the UTI from the system
  CFStringRef uti = UTTypeCreatePreferredIdentifierForTag(
    kUTTagClassMIMEType, (__bridge CFStringRef)mimeType, NULL);

  return (uti && UTTypeIsDynamic(uti)) ? (NSString*)CFBridgingRelease(uti)
                                       : NULL;
}

static NSRect
rectToScreen(const NSScreen* screen, NSRect rect)
{
  const double screenHeight = [screen frame].size.height;

  rect.origin.y = screenHeight - rect.origin.y - rect.size.height;
  return rect;
}

static NSRect
rectFromScreen(const NSScreen* screen, NSRect rect)
{
  /* The math happens to work out to the same expression because this is just
     an inversion of the Y axis based on the screen height.  This function is
     preserved to make the code clearer and more future-proof since the use
     cases are distinctly either a "from" or "to" conversion. */

  return rectToScreen(screen, rect);
}

static NSScreen*
viewScreen(const PuglView* view)
{
  return view->impl->window ? [view->impl->window screen]
         : [view->impl->wrapperView window]
           ? [[view->impl->wrapperView window] screen]
           : [NSScreen mainScreen];
}

static NSRect
nsRectToPoints(PuglView* view, const NSRect rect)
{
  const double scaleFactor = [viewScreen(view) backingScaleFactor];

  return NSMakeRect(rect.origin.x / scaleFactor,
                    rect.origin.y / scaleFactor,
                    rect.size.width / scaleFactor,
                    rect.size.height / scaleFactor);
}

static NSRect
nsRectFromPoints(PuglView* view, const NSRect rect)
{
  const double scaleFactor = [viewScreen(view) backingScaleFactor];

  return NSMakeRect(rect.origin.x * scaleFactor,
                    rect.origin.y * scaleFactor,
                    rect.size.width * scaleFactor,
                    rect.size.height * scaleFactor);
}

static NSPoint
nsPointFromPoints(PuglView* view, const NSPoint point)
{
  const double scaleFactor = [viewScreen(view) backingScaleFactor];

  return NSMakePoint(point.x * scaleFactor, point.y * scaleFactor);
}

static NSRect
rectToNsRect(const PuglRect rect)
{
  return NSMakeRect(rect.x, rect.y, rect.width, rect.height);
}

static NSSize
sizePoints(PuglView* view, const double width, const double height)
{
  const double scaleFactor = [viewScreen(view) backingScaleFactor];

  return NSMakeSize(width / scaleFactor, height / scaleFactor);
}

static PuglViewStyleFlags
getCurrentViewStyleFlags(PuglView* const view)
{
  const bool isResizing = view->resizing;

  if (!view->impl->window) {
    return (isResizing ? PUGL_VIEW_STYLE_RESIZING : 0U);
  }

  const NSWindowStyleMask styleMask = [view->impl->window styleMask];

  const bool isFullScreen   = styleMask & NSWindowStyleMaskFullScreen;
  const bool isMiniaturized = [view->impl->window isMiniaturized];
  const bool isZoomed       = [view->impl->window isZoomed];
  const bool isVisible      = [view->impl->window isVisible];

  return (isVisible ? PUGL_VIEW_STYLE_MAPPED : 0U) |
         (isFullScreen ? PUGL_VIEW_STYLE_FULLSCREEN : 0U) |
         (isMiniaturized ? PUGL_VIEW_STYLE_HIDDEN : 0U) |
         (isZoomed ? (PUGL_VIEW_STYLE_TALL | PUGL_VIEW_STYLE_WIDE) : 0U) |
         (isResizing ? PUGL_VIEW_STYLE_RESIZING : 0U);
}

static PuglStatus
dispatchCurrentChildViewConfiguration(PuglView* const view)
{
  const NSRect framePt = [view->impl->wrapperView frame];
  const NSRect framePx = nsRectFromPoints(view, framePt);

  if (view->stage < PUGL_VIEW_STAGE_REALIZED) {
    return PUGL_SUCCESS;
  }

  const PuglConfigureEvent ev = {
    PUGL_CONFIGURE,
    0,
    (PuglCoord)framePx.origin.x,
    (PuglCoord)framePx.origin.y,
    (PuglSpan)framePx.size.width,
    (PuglSpan)framePx.size.height,
    getCurrentViewStyleFlags(view),
  };

  PuglEvent configureEvent;
  configureEvent.configure = ev;

  return puglDispatchEvent(view, &configureEvent);
}

@implementation PuglWindow {
@public
  PuglView* puglview;
}

- (id)initWithContentRect:(NSRect)contentRect
                styleMask:(NSWindowStyleMask)aStyle
                  backing:(NSBackingStoreType)bufferingType
                    defer:(BOOL)flag
{
  (void)flag;

  NSWindow* result = [super initWithContentRect:contentRect
                                      styleMask:aStyle
                                        backing:bufferingType
                                          defer:NO];

  [result setAcceptsMouseMovedEvents:YES];
  return (PuglWindow*)result;
}

- (PuglStatus)dispatchCurrentConfiguration
{
  if (puglview->stage < PUGL_VIEW_STAGE_REALIZED) {
    return PUGL_SUCCESS;
  }

  const NSRect screenFramePt = [[NSScreen mainScreen] frame];
  const NSRect screenFramePx = nsRectFromPoints(puglview, screenFramePt);
  const NSRect framePt       = [self frame];
  const NSRect contentPt     = [self contentRectForFrameRect:framePt];
  const NSRect contentPx     = nsRectFromPoints(puglview, contentPt);
  const double screenHeight  = screenFramePx.size.height;

  const PuglConfigureEvent ev = {
    PUGL_CONFIGURE,
    0,
    (PuglCoord)contentPx.origin.x,
    (PuglCoord)(screenHeight - contentPx.origin.y - contentPx.size.height),
    (PuglSpan)contentPx.size.width,
    (PuglSpan)contentPx.size.height,
    getCurrentViewStyleFlags(puglview),
  };

  PuglEvent configureEvent;
  configureEvent.configure = ev;

  return puglDispatchEvent(puglview, &configureEvent);
}

- (BOOL)canBecomeKeyWindow
{
  return YES;
}

- (BOOL)canBecomeMainWindow
{
  return YES;
}

- (void)setIsVisible:(BOOL)flag
{
  [super setIsVisible:flag];

  if (flag != (puglview->lastConfigure.style & PUGL_VIEW_STYLE_MAPPED)) {
    [self dispatchCurrentConfiguration];
  }
}

- (void)setIsZoomed:(BOOL)flag
{
  [super setIsZoomed:flag];

  const bool wasZoomed = (puglview->lastConfigure.style &
                          (PUGL_VIEW_STYLE_TALL | PUGL_VIEW_STYLE_WIDE));

  if (flag != wasZoomed) {
    [self dispatchCurrentConfiguration];
  }
}

@end

@implementation PuglWrapperView {
@public
  PuglView*                  puglview;
  NSTrackingArea*            trackingArea;
  NSMutableAttributedString* markedText;
  NSMutableDictionary*       userTimers;
  id<NSDraggingInfo>         dragSource;
  NSDragOperation            dragOperation;
  size_t                     dragTypeIndex;
  NSString*                  droppedUriList;
  bool                       reshaped;
}

- (void)dispatchExpose:(NSRect)rect
{
  const double scaleFactor = [[NSScreen mainScreen] backingScaleFactor];

  if (reshaped) {
    if (puglview->impl->window) {
      [puglview->impl->window dispatchCurrentConfiguration];
    } else {
      dispatchCurrentChildViewConfiguration(puglview);
    }
    reshaped = false;
  }

  if (![[puglview->impl->drawView window] isVisible]) {
    return;
  }

  const PuglExposeEvent ev = {
    PUGL_EXPOSE,
    0,
    (PuglCoord)(rect.origin.x * scaleFactor),
    (PuglCoord)(rect.origin.y * scaleFactor),
    (PuglSpan)(rect.size.width * scaleFactor),
    (PuglSpan)(rect.size.height * scaleFactor),
  };

  PuglEvent exposeEvent;
  exposeEvent.expose = ev;
  puglDispatchEvent(puglview, &exposeEvent);
}

- (NSSize)intrinsicContentSize
{
  const PuglViewSize defaultSize = puglview->sizeHints[PUGL_DEFAULT_SIZE];

  return puglIsValidSize(defaultSize)
           ? sizePoints(puglview, defaultSize.width, defaultSize.height)
           : NSMakeSize(NSViewNoInstrinsicMetric, NSViewNoInstrinsicMetric);
}

- (BOOL)isFlipped
{
  return YES;
}

- (BOOL)acceptsFirstResponder
{
  return YES;
}

- (void)setReshaped
{
  reshaped = true;
}

static uint32_t
getModifiers(const NSEvent* const ev)
{
  const NSEventModifierFlags modifierFlags = [ev modifierFlags];

  return (((modifierFlags & NSShiftKeyMask) ? PUGL_MOD_SHIFT : 0) |
          ((modifierFlags & NSControlKeyMask) ? PUGL_MOD_CTRL : 0) |
          ((modifierFlags & NSAlternateKeyMask) ? PUGL_MOD_ALT : 0) |
          ((modifierFlags & NSCommandKeyMask) ? PUGL_MOD_SUPER : 0));
}

static PuglKey
keySymToSpecial(const NSEvent* const ev)
{
  NSString* chars = [ev charactersIgnoringModifiers];
  if ([chars length] == 1) {
    const unichar sym = [chars characterAtIndex:0];
    if (sym >= NSF1FunctionKey && sym <= NSF12FunctionKey) {
      return (PuglKey)(PUGL_KEY_F1 + (sym - NSF1FunctionKey));
    }

    switch (sym) {
    case NSDeleteCharacter:
      return PUGL_KEY_BACKSPACE;
    case NSDeleteFunctionKey:
      return PUGL_KEY_DELETE;
    case NSLeftArrowFunctionKey:
      return PUGL_KEY_LEFT;
    case NSUpArrowFunctionKey:
      return PUGL_KEY_UP;
    case NSRightArrowFunctionKey:
      return PUGL_KEY_RIGHT;
    case NSDownArrowFunctionKey:
      return PUGL_KEY_DOWN;
    case NSPageUpFunctionKey:
      return PUGL_KEY_PAGE_UP;
    case NSPageDownFunctionKey:
      return PUGL_KEY_PAGE_DOWN;
    case NSHomeFunctionKey:
      return PUGL_KEY_HOME;
    case NSEndFunctionKey:
      return PUGL_KEY_END;
    case NSInsertFunctionKey:
      return PUGL_KEY_INSERT;
    case NSMenuFunctionKey:
      return PUGL_KEY_MENU;
    case NSScrollLockFunctionKey:
      return PUGL_KEY_SCROLL_LOCK;
    case NSClearLineFunctionKey:
      return PUGL_KEY_NUM_LOCK;
    case NSPrintScreenFunctionKey:
      return PUGL_KEY_PRINT_SCREEN;
    case NSPauseFunctionKey:
      return PUGL_KEY_PAUSE;
    }
    // SHIFT, CTRL, ALT, and SUPER are handled in [flagsChanged]
  }

  switch ([ev keyCode]) {
  case 0x41:
    return PUGL_KEY_PAD_DECIMAL;
  case 0x43:
    return PUGL_KEY_PAD_MULTIPLY;
  case 0x45:
    return PUGL_KEY_PAD_ADD;
  case 0x4B:
    return PUGL_KEY_PAD_DIVIDE;
  case 0x4C:
    return PUGL_KEY_PAD_ENTER;
  case 0x4E:
    return PUGL_KEY_PAD_SUBTRACT;
  case 0x51:
    return PUGL_KEY_PAD_EQUAL;
  case 0x52:
    return PUGL_KEY_PAD_0;
  case 0x53:
    return PUGL_KEY_PAD_1;
  case 0x54:
    return PUGL_KEY_PAD_2;
  case 0x55:
    return PUGL_KEY_PAD_3;
  case 0x56:
    return PUGL_KEY_PAD_4;
  case 0x57:
    return PUGL_KEY_PAD_5;
  case 0x58:
    return PUGL_KEY_PAD_6;
  case 0x59:
    return PUGL_KEY_PAD_7;
  case 0x5B:
    return PUGL_KEY_PAD_8;
  case 0x5C:
    return PUGL_KEY_PAD_9;
  }

  return (PuglKey)0;
}

- (void)updateTrackingAreas
{
  if (trackingArea != nil) {
    [self removeTrackingArea:trackingArea];
    [trackingArea release];
  }

  const int opts = (NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved |
                    NSTrackingActiveAlways);
  trackingArea   = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                              options:opts
                                                owner:self
                                             userInfo:nil];
  [self addTrackingArea:trackingArea];
  [super updateTrackingAreas];
}

- (NSPoint)eventLocation:(NSEvent*)event
{
  return nsPointFromPoints(
    puglview, [self convertPoint:[event locationInWindow] fromView:nil]);
}

static void
handleCrossing(PuglWrapperView* view, NSEvent* event, const PuglEventType type)
{
  const NSPoint           wloc = [view eventLocation:event];
  const NSPoint           rloc = [NSEvent mouseLocation];
  const PuglCrossingEvent ev   = {
    type,
    0,
    [event timestamp],
    wloc.x,
    wloc.y,
    rloc.x,
    [[NSScreen mainScreen] frame].size.height - rloc.y,
    getModifiers(event),
    PUGL_CROSSING_NORMAL,
  };

  PuglEvent crossingEvent;
  crossingEvent.crossing = ev;
  puglDispatchEvent(view->puglview, &crossingEvent);
}

- (void)mouseEntered:(NSEvent*)event
{
  handleCrossing(self, event, PUGL_POINTER_IN);
  [puglview->impl->cursor set];
  puglview->impl->mouseTracked = true;
}

- (void)mouseExited:(NSEvent*)event
{
  [[NSCursor arrowCursor] set];
  handleCrossing(self, event, PUGL_POINTER_OUT);
  puglview->impl->mouseTracked = false;
}

- (void)cursorUpdate:(NSEvent*)event
{
  (void)event;
  [puglview->impl->cursor set];
}

- (void)mouseMoved:(NSEvent*)event
{
  const NSPoint         wloc = [self eventLocation:event];
  const NSPoint         rloc = [NSEvent mouseLocation];
  const PuglMotionEvent ev   = {
    PUGL_MOTION,
    0,
    [event timestamp],
    wloc.x,
    wloc.y,
    rloc.x,
    [[NSScreen mainScreen] frame].size.height - rloc.y,
    getModifiers(event),
  };

  PuglEvent motionEvent;
  motionEvent.motion = ev;
  puglDispatchEvent(puglview, &motionEvent);
}

- (void)mouseDragged:(NSEvent*)event
{
  [self mouseMoved:event];
}

- (void)rightMouseDragged:(NSEvent*)event
{
  [self mouseMoved:event];
}

- (void)otherMouseDragged:(NSEvent*)event
{
  [self mouseMoved:event];
}

- (void)mouseDown:(NSEvent*)event
{
  const NSPoint         wloc = [self eventLocation:event];
  const NSPoint         rloc = [NSEvent mouseLocation];
  const PuglButtonEvent ev   = {
    PUGL_BUTTON_PRESS,
    0,
    [event timestamp],
    wloc.x,
    wloc.y,
    rloc.x,
    [[NSScreen mainScreen] frame].size.height - rloc.y,
    getModifiers(event),
    (uint32_t)[event buttonNumber],
  };

  PuglEvent pressEvent;
  pressEvent.button = ev;
  puglDispatchEvent(puglview, &pressEvent);
}

- (void)mouseUp:(NSEvent*)event
{
  const NSPoint         wloc = [self eventLocation:event];
  const NSPoint         rloc = [NSEvent mouseLocation];
  const PuglButtonEvent ev   = {
    PUGL_BUTTON_RELEASE,
    0,
    [event timestamp],
    wloc.x,
    wloc.y,
    rloc.x,
    [[NSScreen mainScreen] frame].size.height - rloc.y,
    getModifiers(event),
    (uint32_t)[event buttonNumber],
  };

  PuglEvent releaseEvent;
  releaseEvent.button = ev;
  puglDispatchEvent(puglview, &releaseEvent);
}

- (void)rightMouseDown:(NSEvent*)event
{
  [self mouseDown:event];
}

- (void)rightMouseUp:(NSEvent*)event
{
  [self mouseUp:event];
}

- (void)otherMouseDown:(NSEvent*)event
{
  [self mouseDown:event];
}

- (void)otherMouseUp:(NSEvent*)event
{
  [self mouseUp:event];
}

- (void)scrollWheel:(NSEvent*)event
{
  const NSPoint             wloc = [self eventLocation:event];
  const NSPoint             rloc = [NSEvent mouseLocation];
  const double              dx   = [event scrollingDeltaX];
  const double              dy   = [event scrollingDeltaY];
  const PuglScrollDirection dir =
    ((dx == 0.0 && dy > 0.0)
       ? PUGL_SCROLL_UP
       : ((dx == 0.0 && dy < 0.0)
            ? PUGL_SCROLL_DOWN
            : ((dy == 0.0 && dx > 0.0)
                 ? PUGL_SCROLL_RIGHT
                 : ((dy == 0.0 && dx < 0.0) ? PUGL_SCROLL_LEFT
                                            : PUGL_SCROLL_SMOOTH))));

  const PuglScrollEvent ev = {
    PUGL_SCROLL,
    0,
    [event timestamp],
    wloc.x,
    wloc.y,
    rloc.x,
    [[NSScreen mainScreen] frame].size.height - rloc.y,
    getModifiers(event),
    [event hasPreciseScrollingDeltas] ? PUGL_SCROLL_SMOOTH : dir,
    dx,
    dy,
  };

  PuglEvent scrollEvent;
  scrollEvent.scroll = ev;
  puglDispatchEvent(puglview, &scrollEvent);
}

- (void)keyDown:(NSEvent*)event
{
  if (puglview->hints[PUGL_IGNORE_KEY_REPEAT] && [event isARepeat]) {
    return;
  }

  const NSPoint   wloc  = [self eventLocation:event];
  const NSPoint   rloc  = [NSEvent mouseLocation];
  const PuglKey   spec  = keySymToSpecial(event);
  const NSString* chars = [event charactersIgnoringModifiers];
  const char*     str   = [[chars lowercaseString] UTF8String];
  const uint32_t  code  = (spec ? spec : puglDecodeUTF8((const uint8_t*)str));

  const PuglKeyEvent ev = {
    PUGL_KEY_PRESS,
    0,
    [event timestamp],
    wloc.x,
    wloc.y,
    rloc.x,
    [[NSScreen mainScreen] frame].size.height - rloc.y,
    getModifiers(event),
    [event keyCode],
    (code != 0xFFFD) ? code : 0,
  };

  PuglEvent pressEvent;
  pressEvent.key = ev;
  puglDispatchEvent(puglview, &pressEvent);

  if (!spec) {
    [self interpretKeyEvents:@[event]];
  }
}

- (void)keyUp:(NSEvent*)event
{
  const NSPoint   wloc  = [self eventLocation:event];
  const NSPoint   rloc  = [NSEvent mouseLocation];
  const PuglKey   spec  = keySymToSpecial(event);
  const NSString* chars = [event charactersIgnoringModifiers];
  const char*     str   = [[chars lowercaseString] UTF8String];
  const uint32_t  code  = (spec ? spec : puglDecodeUTF8((const uint8_t*)str));

  const PuglKeyEvent ev = {
    PUGL_KEY_RELEASE,
    0,
    [event timestamp],
    wloc.x,
    wloc.y,
    rloc.x,
    [[NSScreen mainScreen] frame].size.height - rloc.y,
    getModifiers(event),
    [event keyCode],
    (code != 0xFFFD) ? code : 0,
  };

  PuglEvent releaseEvent;
  releaseEvent.key = ev;
  puglDispatchEvent(puglview, &releaseEvent);
}

- (BOOL)hasMarkedText
{
  return [markedText length] > 0;
}

- (NSRange)markedRange
{
  return (([markedText length] > 0) ? NSMakeRange(0, [markedText length] - 1)
                                    : NSMakeRange(NSNotFound, 0));
}

- (NSRange)selectedRange
{
  return NSMakeRange(NSNotFound, 0);
}

- (void)setMarkedText:(id)string
        selectedRange:(NSRange)selected
     replacementRange:(NSRange)replacement
{
  (void)selected;
  (void)replacement;
  [markedText release];
  markedText =
    ([(NSObject*)string isKindOfClass:[NSAttributedString class]]
       ? [[NSMutableAttributedString alloc] initWithAttributedString:string]
       : [[NSMutableAttributedString alloc] initWithString:string]);
}

- (void)unmarkText
{
  [[markedText mutableString] setString:@""];
}

- (NSArray*)validAttributesForMarkedText
{
  return @[];
}

- (NSAttributedString*)attributedSubstringForProposedRange:(NSRange)range
                                               actualRange:
                                                 (NSRangePointer)actual
{
  (void)range;
  (void)actual;
  return nil;
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point
{
  (void)point;
  return 0;
}

- (NSRect)firstRectForCharacterRange:(NSRange)range
                         actualRange:(NSRangePointer)actual
{
  (void)range;
  (void)actual;

  const NSRect frame = [self bounds];
  return NSMakeRect(frame.origin.x, frame.origin.y, 0.0, 0.0);
}

- (void)doCommandBySelector:(SEL)selector
{
  (void)selector;
}

- (void)insertText:(id)string replacementRange:(NSRange)replacement
{
  (void)replacement;

  NSEvent* const  event = [NSApp currentEvent];
  NSString* const characters =
    ([(NSObject*)string isKindOfClass:[NSAttributedString class]]
       ? [(NSAttributedString*)string string]
       : (NSString*)string);

  const NSPoint wloc = [self eventLocation:event];
  const NSPoint rloc = [NSEvent mouseLocation];
  for (size_t i = 0; i < [characters length]; ++i) {
    const uint32_t code    = [characters characterAtIndex:i];
    char           utf8[8] = {0};
    NSUInteger     len     = 0;

    [characters getBytes:utf8
               maxLength:sizeof(utf8)
              usedLength:&len
                encoding:NSUTF8StringEncoding
                 options:0
                   range:NSMakeRange(i, i + 1)
          remainingRange:nil];

    PuglTextEvent ev = {
      PUGL_TEXT,
      0,
      [event timestamp],
      wloc.x,
      wloc.y,
      rloc.x,
      [[NSScreen mainScreen] frame].size.height - rloc.y,
      getModifiers(event),
      [event keyCode],
      code,
      { 0, 0, 0, 0, 0, 0, 0, 0 },
    };

    memcpy(ev.string, utf8, len);

    PuglEvent textEvent;
    textEvent.text = ev;
    puglDispatchEvent(puglview, &textEvent);
  }
}

- (void)flagsChanged:(NSEvent*)event
{
  const uint32_t mods    = getModifiers(event);
  PuglEventType  type    = PUGL_NOTHING;
  PuglKey        special = (PuglKey)0;

  if ((mods & PUGL_MOD_SHIFT) != (puglview->impl->mods & PUGL_MOD_SHIFT)) {
    type    = mods & PUGL_MOD_SHIFT ? PUGL_KEY_PRESS : PUGL_KEY_RELEASE;
    special = PUGL_KEY_SHIFT_L;
  } else if ((mods & PUGL_MOD_CTRL) != (puglview->impl->mods & PUGL_MOD_CTRL)) {
    type    = mods & PUGL_MOD_CTRL ? PUGL_KEY_PRESS : PUGL_KEY_RELEASE;
    special = PUGL_KEY_CTRL_L;
  } else if ((mods & PUGL_MOD_ALT) != (puglview->impl->mods & PUGL_MOD_ALT)) {
    type    = mods & PUGL_MOD_ALT ? PUGL_KEY_PRESS : PUGL_KEY_RELEASE;
    special = PUGL_KEY_ALT_L;
  } else if ((mods & PUGL_MOD_SUPER) !=
             (puglview->impl->mods & PUGL_MOD_SUPER)) {
    type    = mods & PUGL_MOD_SUPER ? PUGL_KEY_PRESS : PUGL_KEY_RELEASE;
    special = PUGL_KEY_SUPER_L;
  }

  if (special != 0) {
    const NSPoint wloc = [self eventLocation:event];
    const NSPoint rloc = [NSEvent mouseLocation];

    const PuglKeyEvent ev = {type,
                             0,
                             [event timestamp],
                             wloc.x,
                             wloc.y,
                             rloc.x,
                             [[NSScreen mainScreen] frame].size.height - rloc.y,
                             mods,
                             [event keyCode],
                             special};

    PuglEvent keyEvent;
    keyEvent.key = ev;
    puglDispatchEvent(puglview, &keyEvent);
  }

  puglview->impl->mods = mods;
}

- (BOOL)preservesContentInLiveResize
{
  return NO;
}

- (void)viewWillStartLiveResize
{
  puglDispatchSimpleEvent(puglview, PUGL_LOOP_ENTER);
}

- (void)viewDidEndLiveResize
{
  puglDispatchSimpleEvent(puglview, PUGL_LOOP_LEAVE);
}

- (void)viewWillDraw
{
  puglDispatchSimpleEvent(puglview, PUGL_UPDATE);
  [super viewWillDraw];
}

- (void)timerTick:(NSTimer*)userTimer
{
  const NSNumber*      userInfo = userTimer.userInfo;
  const PuglTimerEvent ev       = {PUGL_TIMER, 0, userInfo.unsignedLongValue};

  PuglEvent timerEvent;
  timerEvent.timer = ev;
  puglDispatchEvent(puglview, &timerEvent);
}

@end

@interface PuglWindowDelegate : NSObject<NSWindowDelegate>

- (instancetype)initWithPuglWindow:(PuglWindow*)window;
- (void)beginLiveResize:(NSNotification*)notification;
- (void)endLiveResize:(NSNotification*)notification;

@end

@implementation PuglWindowDelegate {
  PuglWindow* window;
}

- (instancetype)initWithPuglWindow:(PuglWindow*)puglWindow
{
  if ((self = [super init])) {
    window = puglWindow;
  }

  return self;
}

- (void)beginLiveResize:(NSNotification*)notification
{
  (void)notification;

  window->puglview->resizing = true;
  [window dispatchCurrentConfiguration];
}

- (void)endLiveResize:(NSNotification*)notification
{
  (void)notification;

  window->puglview->resizing = false;
  [window dispatchCurrentConfiguration];
}

- (BOOL)windowShouldClose:(id)sender
{
  (void)sender;

  puglDispatchSimpleEvent(window->puglview, PUGL_CLOSE);
  return YES;
}

- (void)windowDidMove:(NSNotification*)notification
{
  (void)notification;

  [window dispatchCurrentConfiguration];
}

- (void)windowDidBecomeKey:(NSNotification*)notification
{
  (void)notification;

  PuglEvent ev  = {{PUGL_FOCUS_IN, 0}};
  ev.focus.mode = PUGL_CROSSING_NORMAL;
  puglDispatchEvent(window->puglview, &ev);
}

- (void)windowDidResignKey:(NSNotification*)notification
{
  (void)notification;

  PuglEvent ev  = {{PUGL_FOCUS_OUT, 0}};
  ev.focus.mode = PUGL_CROSSING_NORMAL;
  puglDispatchEvent(window->puglview, &ev);
}

- (void)windowWillStartLiveResize:(NSNotification*)notification
{
  [self beginLiveResize:notification];
}

- (void)windowDidEndLiveResize:(NSNotification*)notification
{
  [self endLiveResize:notification];
}

- (void)windowWillMiniaturize:(NSNotification*)notification
{
  [self beginLiveResize:notification];
}

- (void)windowDidMiniaturize:(NSNotification*)notification
{
  [self endLiveResize:notification];
}

- (void)windowWillDeminiaturize:(NSNotification*)notification
{
  [self beginLiveResize:notification];
}

- (void)windowDidDeminiaturize:(NSNotification*)notification
{
  [self endLiveResize:notification];
}

- (void)windowWillEnterFullScreen:(NSNotification*)notification
{
  [self beginLiveResize:notification];
}

- (void)windowDidEnterFullScreen:(NSNotification*)notification
{
  [self endLiveResize:notification];
}

- (void)windowWillExitFullScreen:(NSNotification*)notification
{
  [self beginLiveResize:notification];
}

- (void)windowDidExitFullScreen:(NSNotification*)notification
{
  [self endLiveResize:notification];
}

@end

PuglWorldInternals*
puglInitWorldInternals(PuglWorldType type, PuglWorldFlags PUGL_UNUSED(flags))
{
  PuglWorldInternals* impl =
    (PuglWorldInternals*)calloc(1, sizeof(PuglWorldInternals));

  impl->app = [NSApplication sharedApplication];

  if (mach_timebase_info(&impl->timebaseInfo)) {
    return NULL;
  }

  if (type == PUGL_PROGRAM) {
    impl->autoreleasePool = [NSAutoreleasePool new];

    [impl->app setActivationPolicy:NSApplicationActivationPolicyRegular];
  }

  return impl;
}

void
puglFreeWorldInternals(PuglWorld* world)
{
  if (world->impl->autoreleasePool) {
    [world->impl->autoreleasePool drain];
  }

  free(world->impl);
}

void*
puglGetNativeWorld(PuglWorld* world)
{
  return world->impl->app;
}

PuglInternals*
puglInitViewInternals(PuglWorld* PUGL_UNUSED(world))
{
  PuglInternals* impl = (PuglInternals*)calloc(1, sizeof(PuglInternals));

  impl->cursor = [NSCursor arrowCursor];

  return impl;
}

static NSLayoutConstraint*
puglConstraint(const id                item,
               const NSLayoutAttribute attribute,
               const NSLayoutRelation  relation,
               const float             constant)
{
  return [NSLayoutConstraint constraintWithItem:item
                                      attribute:attribute
                                      relatedBy:relation
                                         toItem:nil
                                      attribute:NSLayoutAttributeNotAnAttribute
                                     multiplier:1.0
                                       constant:(CGFloat)constant];
}

static PuglStatus
updateSizeHint(PuglView* const view, const PuglSizeHint hint)
{
  const PuglSpan width  = view->sizeHints[hint].width;
  const PuglSpan height = view->sizeHints[hint].height;
  if (!puglIsValidSize(view->sizeHints[hint])) {
    return PUGL_FAILURE;
  }

  switch (hint) {
  case PUGL_DEFAULT_SIZE:
    break;

  case PUGL_MIN_SIZE:
    [view->impl->window setContentMinSize:sizePoints(view, width, height)];
    break;

  case PUGL_MAX_SIZE:
    [view->impl->window setContentMaxSize:sizePoints(view, width, height)];
    break;

  case PUGL_FIXED_ASPECT:
    [view->impl->window setContentAspectRatio:sizePoints(view, width, height)];
    break;

  case PUGL_MIN_ASPECT:
  case PUGL_MAX_ASPECT:
    break;
  }

  return PUGL_SUCCESS;
}

static void
updateSizeHints(PuglView* const view)
{
  for (unsigned i = 0U; i < PUGL_NUM_SIZE_HINTS; ++i) {
    updateSizeHint(view, (PuglSizeHint)i);
  }
}

static PuglRect
getInitialFrame(PuglView* const view)
{
  if (view->lastConfigure.type == PUGL_CONFIGURE) {
    // Use the last configured frame
    const PuglRect frame = {view->lastConfigure.x,
                            view->lastConfigure.y,
                            view->lastConfigure.width,
                            view->lastConfigure.height};
    return frame;
  }

  const int x = view->defaultX;
  const int y = view->defaultY;
  if (x >= INT16_MIN && x <= INT16_MAX && y >= INT16_MIN && y <= INT16_MAX) {
    // Use the default position set with puglSetPosition while unrealized
    const PuglRect frame = {(PuglCoord)x,
                            (PuglCoord)y,
                            view->sizeHints[PUGL_DEFAULT_SIZE].width,
                            view->sizeHints[PUGL_DEFAULT_SIZE].height};
    return frame;
  }

  // Get a bounding rect from the transient parent or the screen
  const NSScreen* const screen = viewScreen(view);
  const NSRect          boundsPt =
    rectFromScreen(screen,
                   view->transientParent
                     ? [[(const NSView*)view->transientParent window] frame]
                     : [screen frame]);

  // Center the frame around the center of the bounding rectangle
  const PuglViewSize defaultSize = view->sizeHints[PUGL_DEFAULT_SIZE];
  const NSRect       boundsPx    = nsRectFromPoints(view, boundsPt);
  const double       centerX     = boundsPx.origin.x + boundsPx.size.width / 2;
  const double       centerY     = boundsPx.origin.y + boundsPx.size.height / 2;

  const PuglRect frame = {(PuglCoord)(centerX - (defaultSize.width / 2U)),
                          (PuglCoord)(centerY - (defaultSize.height / 2U)),
                          view->sizeHints[PUGL_DEFAULT_SIZE].width,
                          view->sizeHints[PUGL_DEFAULT_SIZE].height};
  return frame;
}

PuglStatus
puglRealize(PuglView* view)
{
  PuglInternals* impl = view->impl;
  PuglStatus     st   = PUGL_SUCCESS;

  // Ensure that we're unrealized
  if (impl->wrapperView) {
    return PUGL_FAILURE;
  }

  // Check that the basic required configuration has been done
  if ((st = puglPreRealize(view))) {
    return st;
  }

  const NSScreen* const screen      = [NSScreen mainScreen];
  const double          scaleFactor = [screen backingScaleFactor];

  // Getting depth from the display mode seems tedious, just set usual values
  puglEnsureHint(view, PUGL_RED_BITS, 8);
  puglEnsureHint(view, PUGL_GREEN_BITS, 8);
  puglEnsureHint(view, PUGL_BLUE_BITS, 8);
  puglEnsureHint(view, PUGL_ALPHA_BITS, 8);

  CGDirectDisplayID displayId = CGMainDisplayID();
  CGDisplayModeRef  mode      = CGDisplayCopyDisplayMode(displayId);

  // Try to get refresh rate from mode (usually fails)
  view->hints[PUGL_REFRESH_RATE] = (int)CGDisplayModeGetRefreshRate(mode);

  CGDisplayModeRelease(mode);
  if (view->hints[PUGL_REFRESH_RATE] == 0) {
    // Get refresh rate from a display link
    // TODO: Keep and actually use the display link for something?
    CVDisplayLinkRef link;
    CVDisplayLinkCreateWithCGDisplay(displayId, &link);

    const CVTime p = CVDisplayLinkGetNominalOutputVideoRefreshPeriod(link);
    const double r = p.timeScale / (double)p.timeValue;
    view->hints[PUGL_REFRESH_RATE] = (int)lrint(r);

    CVDisplayLinkRelease(link);
  }

  // Get the initial frame to use from the defaults or last configuration
  const PuglRect initialFrame = getInitialFrame(view);

  // Convert frame to points
  const NSRect framePx = rectToNsRect(initialFrame);
  const NSRect framePt = NSMakeRect(framePx.origin.x / scaleFactor,
                                    framePx.origin.y / scaleFactor,
                                    framePx.size.width / scaleFactor,
                                    framePx.size.height / scaleFactor);

  // Create wrapper view to handle input
  impl->wrapperView             = [PuglWrapperView alloc];
  impl->wrapperView->puglview   = view;
  impl->wrapperView->userTimers = [[NSMutableDictionary alloc] init];
  impl->wrapperView->markedText = [[NSMutableAttributedString alloc] init];
  [impl->wrapperView setAutoresizesSubviews:YES];
  [impl->wrapperView initWithFrame:framePt];

  [impl->wrapperView
    addConstraint:puglConstraint(impl->wrapperView,
                                 NSLayoutAttributeWidth,
                                 NSLayoutRelationGreaterThanOrEqual,
                                 view->sizeHints[PUGL_MIN_SIZE].width)];

  [impl->wrapperView
    addConstraint:puglConstraint(impl->wrapperView,
                                 NSLayoutAttributeHeight,
                                 NSLayoutRelationGreaterThanOrEqual,
                                 view->sizeHints[PUGL_MIN_SIZE].height)];

  if (puglIsValidSize(view->sizeHints[PUGL_MAX_SIZE])) {
    [impl->wrapperView
      addConstraint:puglConstraint(impl->wrapperView,
                                   NSLayoutAttributeWidth,
                                   NSLayoutRelationLessThanOrEqual,
                                   view->sizeHints[PUGL_MAX_SIZE].width)];

    [impl->wrapperView
      addConstraint:puglConstraint(impl->wrapperView,
                                   NSLayoutAttributeHeight,
                                   NSLayoutRelationLessThanOrEqual,
                                   view->sizeHints[PUGL_MAX_SIZE].height)];
  }

  // Create draw view to be rendered to
  if ((st = view->backend->configure(view)) ||
      (st = view->backend->create(view))) {
    return st;
  }

  // Add draw view to wrapper view
  [impl->wrapperView addSubview:impl->drawView];
  [impl->wrapperView setHidden:NO];
  [impl->drawView setHidden:NO];

  if (view->parent) {
    NSView* pview = (NSView*)view->parent;
    [pview addSubview:impl->wrapperView];
    [impl->drawView setHidden:NO];
    [[impl->drawView window] makeFirstResponder:impl->wrapperView];
  } else {
    unsigned style =
      (NSClosableWindowMask | NSTitledWindowMask | NSMiniaturizableWindowMask);
    if (view->hints[PUGL_RESIZABLE]) {
      style |= NSResizableWindowMask;
    }

    PuglWindow* window = [[[PuglWindow alloc]
      initWithContentRect:rectToScreen([NSScreen mainScreen], framePt)
                styleMask:style
                  backing:NSBackingStoreBuffered
                    defer:NO] retain];

    window->puglview = view;
    impl->window     = window;
    [window setContentSize:framePt.size];

    const char* const title = view->strings[PUGL_WINDOW_TITLE];
    if (title) {
      NSString* titleString =
        [[NSString alloc] initWithBytes:title
                                 length:strlen(title)
                               encoding:NSUTF8StringEncoding];

      [window setTitle:titleString];
    }

    ((NSWindow*)window).delegate =
      [[PuglWindowDelegate alloc] initWithPuglWindow:window];

    // Set basic window hints and attributes
    puglSetFrame(view, initialFrame);
    puglSetTransientParent(view, view->transientParent);
    updateSizeHints(view);

    [window setContentView:impl->wrapperView];
    [view->world->impl->app activateIgnoringOtherApps:YES];
    [window makeFirstResponder:impl->wrapperView];
    [window makeKeyAndOrderFront:window];
    [impl->window setIsVisible:NO];
  }

  [impl->wrapperView updateTrackingAreas];

  return puglDispatchSimpleEvent(view, PUGL_REALIZE);
}

PuglStatus
puglUnrealize(PuglView* const view)
{
  PuglInternals* const impl = view->impl;
  if (!impl || !impl->wrapperView) {
    return PUGL_FAILURE;
  }

  puglDispatchSimpleEvent(view, PUGL_UNREALIZE);

  if (view->backend) {
    view->backend->destroy(view);
  }

  if (impl->wrapperView) {
    [impl->wrapperView removeFromSuperview];
  }

  if (impl->window) {
    [impl->window close];
  }

  if (impl->wrapperView) {
    [impl->wrapperView release];
    impl->wrapperView = NULL;
  }

  if (impl->window) {
    [impl->window release];
    impl->window = NULL;
  }

  memset(&view->lastConfigure, 0, sizeof(PuglConfigureEvent));
  return PUGL_SUCCESS;
}

PuglStatus
puglShow(PuglView* view, const PuglShowCommand command)
{
  if (!view->impl->wrapperView) {
    const PuglStatus st = puglRealize(view);
    if (st) {
      return st;
    }
  }

  NSWindow* const window = [view->impl->wrapperView window];
  if (![window isVisible]) {
    [window setIsVisible:YES];
    [view->impl->drawView setNeedsDisplay:YES];
  }

  switch (command) {
  case PUGL_SHOW_PASSIVE:
    break;
  case PUGL_SHOW_RAISE:
    [window orderFront:view->impl->wrapperView];
    break;
  case PUGL_SHOW_FORCE_RAISE:
    [window orderFrontRegardless];
    break;
  }

  return PUGL_SUCCESS;
}

PuglStatus
puglHide(PuglView* view)
{
  [view->impl->window setIsVisible:NO];
  return PUGL_SUCCESS;
}

void
puglFreeViewInternals(PuglView* view)
{
  if (view) {
    if (view->backend) {
      view->backend->destroy(view);
    }

    if (view->impl) {
      if (view->impl->wrapperView) {
        [view->impl->wrapperView removeFromSuperview];
        view->impl->wrapperView->puglview = NULL;
      }

      if (view->impl->window) {
        [view->impl->window close];
      }

      if (view->impl->wrapperView) {
        [view->impl->wrapperView release];
      }

      if (view->impl->window) {
        [view->impl->window release];
      }

      free(view->impl);
    }
  }
}

PuglStatus
puglGrabFocus(PuglView* view)
{
  NSWindow* window = [view->impl->wrapperView window];

  [window makeKeyWindow];
  [window makeFirstResponder:view->impl->wrapperView];
  return PUGL_SUCCESS;
}

bool
puglHasFocus(const PuglView* view)
{
  PuglInternals* const impl = view->impl;

  return ([[impl->wrapperView window] isKeyWindow] &&
          [[impl->wrapperView window] firstResponder] == impl->wrapperView);
}

static bool
styleIsMaximized(const PuglViewStyleFlags flags)
{
  return (flags & PUGL_VIEW_STYLE_TALL) && (flags & PUGL_VIEW_STYLE_WIDE);
}

PuglStatus
puglSetViewStyle(PuglView* const view, const PuglViewStyleFlags flags)
{
  NSWindow* const          window   = view->impl->window;
  const PuglViewStyleFlags oldFlags = puglGetViewStyle(view);
  if (!window) {
    return PUGL_FAILURE;
  }

  for (uint32_t mask = 1U; mask <= PUGL_MAX_VIEW_STYLE_FLAG; mask <<= 1U) {
    const bool oldValue = oldFlags & mask;
    const bool newValue = flags & mask;
    if (oldValue == newValue) {
      continue;
    }

    switch (mask) {
    case PUGL_VIEW_STYLE_MODAL:
    case PUGL_VIEW_STYLE_TALL:
    case PUGL_VIEW_STYLE_WIDE:
      break;

    case PUGL_VIEW_STYLE_HIDDEN:
      if (newValue) {
        [window miniaturize:window];
      } else {
        [window deminiaturize:window];
      }
      break;

    case PUGL_VIEW_STYLE_FULLSCREEN:
      if (newValue != ([window styleMask] & NSFullScreenWindowMask)) {
        [window toggleFullScreen:nil];
      }
      break;

    case PUGL_VIEW_STYLE_ABOVE:
    case PUGL_VIEW_STYLE_BELOW:
      break;

    case PUGL_VIEW_STYLE_DEMANDING:
      if (![window isKeyWindow]) {
        [view->world->impl->app requestUserAttention:NSInformationalRequest];
      }
      break;

    case PUGL_VIEW_STYLE_RESIZING:
    case PUGL_VIEW_STYLE_MAPPED:
      break;
    }
  }

  // Handle maximization (MacOS doesn't have tall/wide styles)
  const bool oldMaximized = styleIsMaximized(oldFlags);
  const bool newMaximized = styleIsMaximized(flags);
  if (oldMaximized != newMaximized) {
    [window zoom:window];
  }

  return PUGL_SUCCESS;
}

PuglStatus
puglStartTimer(PuglView* view, uintptr_t id, double timeout)
{
  puglStopTimer(view, id);

  NSNumber* idNumber = [NSNumber numberWithUnsignedLong:id];

  NSTimer* timer = [NSTimer timerWithTimeInterval:timeout
                                           target:view->impl->wrapperView
                                         selector:@selector(timerTick:)
                                         userInfo:idNumber
                                          repeats:YES];

  [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSRunLoopCommonModes];

  view->impl->wrapperView->userTimers[idNumber] = timer;

  return PUGL_SUCCESS;
}

PuglStatus
puglStopTimer(PuglView* view, uintptr_t id)
{
  NSNumber* idNumber = [NSNumber numberWithUnsignedLong:id];
  NSTimer*  timer    = view->impl->wrapperView->userTimers[idNumber];

  if (timer) {
    [view->impl->wrapperView->userTimers removeObjectForKey:timer];
    [timer invalidate];
    return PUGL_SUCCESS;
  }

  return PUGL_UNKNOWN_ERROR;
}

PuglStatus
puglSendEvent(PuglView* view, const PuglEvent* event)
{
  if (event->type == PUGL_CLIENT) {
    PuglEvent copiedEvent = *event;

    CFRunLoopObserverRef observer = CFRunLoopObserverCreateWithHandler(
      NULL,
      kCFRunLoopBeforeSources,
      false,
      0,
      ^(CFRunLoopObserverRef ref, CFRunLoopActivity activity) {
        (void)ref;
        (void)activity;
        puglDispatchEvent(view, &copiedEvent);
      });

    CFRunLoopAddObserver(CFRunLoopGetMain(), observer, kCFRunLoopCommonModes);
    CFRelease(observer);
    return PUGL_SUCCESS;
  }

  if (event->type == PUGL_CLOSE) {
    [view->impl->window close];
    return PUGL_SUCCESS;
  }

  return PUGL_UNSUPPORTED;
}

#ifndef PUGL_DISABLE_DEPRECATED
PuglStatus
puglWaitForEvent(PuglView* view)
{
  return puglPollEvents(view->world, -1.0);
}
#endif

PuglStatus
puglUpdate(PuglWorld* world, const double timeout)
{
  @autoreleasepool {
    if (world->type == PUGL_PROGRAM) {
      NSDate* date =
        ((timeout < 0) ? [NSDate distantFuture]
                       : [NSDate dateWithTimeIntervalSinceNow:timeout]);

      for (NSEvent* ev = NULL;
           (ev = [world->impl->app nextEventMatchingMask:NSAnyEventMask
                                               untilDate:date
                                                  inMode:NSDefaultRunLoopMode
                                                 dequeue:YES]);) {
        [world->impl->app sendEvent:ev];

        if (timeout < 0) {
          // Now that we've got an event, mark the date to avoid looping forever
          date = [NSDate date];
        }
      }
    }

    for (size_t i = 0; i < world->numViews; ++i) {
      PuglView* const view = world->views[i];

      if ([[view->impl->drawView window] isVisible]) {
        puglDispatchSimpleEvent(view, PUGL_UPDATE);
      }

      [view->impl->drawView displayIfNeeded];
    }
  }

  return PUGL_SUCCESS;
}

#ifndef PUGL_DISABLE_DEPRECATED
PuglStatus
puglProcessEvents(PuglView* view)
{
  return puglDispatchEvents(view->world);
}
#endif

double
puglGetTime(const PuglWorld* world)
{
  return ((double)mach_absolute_time() * world->impl->timebaseInfo.denom /
          world->impl->timebaseInfo.numer / 1e9) -
         world->startTime;
}

PuglStatus
puglPostRedisplay(PuglView* view)
{
  [view->impl->drawView setNeedsDisplay:YES];
  return PUGL_SUCCESS;
}

PuglStatus
puglPostRedisplayRect(PuglView* view, const PuglRect rect)
{
  const NSRect rectPx = {
    {(double)rect.x,
     (double)view->lastConfigure.height - (rect.y + rect.height)},
    {(double)rect.width, (double)rect.height},
  };

  [view->impl->drawView setNeedsDisplayInRect:nsRectToPoints(view, rectPx)];

  return PUGL_SUCCESS;
}

PuglNativeView
puglGetNativeView(PuglView* view)
{
  return (PuglNativeView)view->impl->wrapperView;
}

PuglStatus
puglViewStringChanged(PuglView* const      view,
                      const PuglStringHint key,
                      const char* const    value)
{
  if (!view->impl->window) {
    return PUGL_SUCCESS;
  }

  switch (key) {
  case PUGL_CLASS_NAME:
    return PUGL_UNSUPPORTED;

  case PUGL_WINDOW_TITLE:
    if (view->impl->window) {
      NSString* const titleString =
        [[NSString alloc] initWithBytes:value
                                 length:strlen(value)
                               encoding:NSUTF8StringEncoding];

      [view->impl->window setTitle:titleString];
    }
    break;
  }

  return PUGL_SUCCESS;
}

double
puglGetScaleFactor(const PuglView* const view)
{
  return [viewScreen(view) backingScaleFactor];
}

PuglStatus
puglSetFrame(PuglView* view, const PuglRect frame)
{
  PuglInternals* const impl    = view->impl;
  const NSRect         framePx = rectToNsRect(frame);
  const NSRect         framePt = nsRectToPoints(view, framePx);

  if (!impl->wrapperView) {
    // Set defaults to be used when realized
    view->defaultX                            = frame.x;
    view->defaultY                            = frame.y;
    view->sizeHints[PUGL_DEFAULT_SIZE].width  = (PuglSpan)frame.width;
    view->sizeHints[PUGL_DEFAULT_SIZE].height = (PuglSpan)frame.height;
    return PUGL_SUCCESS;
  }

  if (impl->window) {
    const NSRect screenPt = rectToScreen(viewScreen(view), framePt);

    // Move and resize window to fit new content rect
    const NSRect winFrame = [impl->window frameRectForContentRect:screenPt];
    [impl->window setFrame:winFrame display:NO];

    // Resize views
    const NSRect sizePx = NSMakeRect(0, 0, frame.width, frame.height);
    const NSRect sizePt = [impl->drawView convertRectFromBacking:sizePx];
    [impl->wrapperView setFrame:sizePt];
    [impl->drawView setFrame:sizePt];
    [impl->window dispatchCurrentConfiguration];
    return PUGL_SUCCESS;
  }

  // Resize view
  const NSRect sizePx = NSMakeRect(0, 0, frame.width, frame.height);
  const NSRect sizePt = [impl->drawView convertRectFromBacking:sizePx];

  [impl->wrapperView setFrame:framePt];
  [impl->drawView setFrame:sizePt];
  return dispatchCurrentChildViewConfiguration(view);
}

PuglStatus
puglSetPosition(PuglView* const view, const int x, const int y)
{
  if (x < INT16_MIN || x > INT16_MAX || y < INT16_MIN || y > INT16_MAX) {
    return PUGL_BAD_PARAMETER;
  }

  PuglInternals* const impl = view->impl;
  if (!impl->wrapperView) {
    // Set defaults to be used when realized
    view->defaultX = x;
    view->defaultY = y;
    return PUGL_SUCCESS;
  }

  const PuglRect frame = {(PuglCoord)x,
                          (PuglCoord)y,
                          view->lastConfigure.width,
                          view->lastConfigure.height};

  if (impl->window) {
    // Adjust top-level window frame
    return puglSetFrame(view, frame);
  }

  // Set wrapper view origin
  const NSRect framePx = rectToNsRect(frame);
  const NSRect framePt = nsRectToPoints(view, framePx);
  [impl->wrapperView setFrameOrigin:framePt.origin];

  // Set draw view origin
  const NSRect drawPx = NSMakeRect(0, 0, frame.width, frame.height);
  const NSRect drawPt = [impl->drawView convertRectFromBacking:drawPx];
  [impl->drawView setFrameOrigin:drawPt.origin];

  // Dispatch new configuration
  return dispatchCurrentChildViewConfiguration(view);
}

PuglStatus
puglSetSize(PuglView* const view, const unsigned width, const unsigned height)
{
  if (width > INT16_MAX || height > INT16_MAX) {
    return PUGL_BAD_PARAMETER;
  }

  PuglInternals* const impl = view->impl;
  if (!impl->wrapperView) {
    // Set defaults to be used when realized
    view->sizeHints[PUGL_DEFAULT_SIZE].width  = (PuglSpan)width;
    view->sizeHints[PUGL_DEFAULT_SIZE].height = (PuglSpan)height;
    return PUGL_SUCCESS;
  }

  if (impl->window) {
    // Adjust top-level window frame
    PuglRect frame = puglGetFrame(view);
    frame.width    = (PuglSpan)width;
    frame.height   = (PuglSpan)height;
    return puglSetFrame(view, frame);
  }

  // Set wrapper view size
  const double scaleFactor = [viewScreen(view) backingScaleFactor];
  const CGSize frameSizePt = {width / scaleFactor, height / scaleFactor};
  [impl->wrapperView setFrameSize:frameSizePt];

  // Set draw view size
  const NSRect drawPx = NSMakeRect(0, 0, width, height);
  const NSRect drawPt = [impl->drawView convertRectFromBacking:drawPx];
  [impl->drawView setFrameSize:drawPt.size];

  // Dispatch new configuration
  return dispatchCurrentChildViewConfiguration(view);
}

PuglStatus
puglSetSizeHint(PuglView* const    view,
                const PuglSizeHint hint,
                const PuglSpan     width,
                const PuglSpan     height)
{
  if ((unsigned)hint >= PUGL_NUM_SIZE_HINTS) {
    return PUGL_BAD_PARAMETER;
  }

  view->sizeHints[hint].width  = width;
  view->sizeHints[hint].height = height;

  return view->impl->window ? updateSizeHint(view, hint) : PUGL_SUCCESS;
}

PuglStatus
puglSetTransientParent(PuglView* view, PuglNativeView parent)
{
  if (view->parent) {
    return PUGL_FAILURE;
  }

  view->transientParent = parent;

  if (parent && view->impl->window) {
    NSWindow* parentWindow = [(NSView*)parent window];
    if (parentWindow) {
      [parentWindow addChildWindow:view->impl->window ordered:NSWindowAbove];
      return PUGL_SUCCESS;
    }
  }

  return PUGL_FAILURE;
}

PuglStatus
puglPaste(PuglView* const view)
{
  const PuglDataOfferEvent offer = {
    PUGL_DATA_OFFER,
    0,
    puglGetTime(view->world),
  };

  PuglEvent offerEvent;
  offerEvent.offer = offer;
  puglDispatchEvent(view, &offerEvent);
  return PUGL_SUCCESS;
}

uint32_t
puglGetNumClipboardTypes(const PuglView* PUGL_UNUSED(view))
{
  NSPasteboard* const pasteboard = [NSPasteboard generalPasteboard];

  return pasteboard ? (uint32_t)[[pasteboard types] count] : 0;
}

const char*
puglGetClipboardType(const PuglView* PUGL_UNUSED(view),
                     const uint32_t  typeIndex)
{
  NSPasteboard* const pasteboard = [NSPasteboard generalPasteboard];
  if (!pasteboard) {
    return NULL;
  }

  const NSArray<NSPasteboardType>* const types = [pasteboard types];
  if (typeIndex >= [types count]) {
    return NULL;
  }

  NSString* const uti      = [types objectAtIndex:typeIndex];
  NSString* const mimeType = mimeTypeForUti(uti);

  // FIXME: lifetime?
  return mimeType ? [mimeType UTF8String] : [uti UTF8String];
}

PuglStatus
puglAcceptOffer(PuglView* const                 view,
                const PuglDataOfferEvent* const PUGL_UNUSED(offer),
                const uint32_t                  typeIndex)
{
  PuglWrapperView* const wrapper    = view->impl->wrapperView;
  NSPasteboard* const    pasteboard = [NSPasteboard generalPasteboard];
  if (!pasteboard) {
    return PUGL_BAD_PARAMETER;
  }

  const NSArray<NSPasteboardType>* const types = [pasteboard types];
  if (typeIndex >= [types count]) {
    return PUGL_BAD_PARAMETER;
  }

  wrapper->dragOperation = NSDragOperationCopy;
  wrapper->dragTypeIndex = typeIndex;

  const PuglDataEvent data = {
    PUGL_DATA, 0U, puglGetTime(view->world), (uint32_t)typeIndex};

  PuglEvent dataEvent;
  dataEvent.data = data;
  puglDispatchEvent(view, &dataEvent);
  return PUGL_SUCCESS;
}

const void*
puglGetClipboard(PuglView* const view,
                 const uint32_t  typeIndex,
                 size_t* const   len)
{
  *len = 0;

  NSPasteboard* const pasteboard = [NSPasteboard generalPasteboard];
  if (!pasteboard) {
    return NULL;
  }

  const NSArray<NSPasteboardType>* const types = [pasteboard types];
  if (typeIndex >= [types count]) {
    return NULL;
  }

  NSString* const uti = [types objectAtIndex:typeIndex];
  if ([uti isEqualToString:@"public.file-url"] ||
      [uti isEqualToString:@"com.apple.pasteboard.promised-file-url"]) {
    *len = [view->impl->wrapperView->droppedUriList length];
    return [view->impl->wrapperView->droppedUriList UTF8String];
  }

  const NSData* const data = [pasteboard dataForType:uti];

  *len = [data length];
  return [data bytes];
}

static NSCursor*
extendedCursor(const SEL cursorSelector)
{
  if (cursorSelector && [NSCursor respondsToSelector:cursorSelector]) {
    const id object = [NSCursor performSelector:cursorSelector];
    if ([object isKindOfClass:[NSCursor class]]) {
      return (NSCursor*)object;
    }
  }

  return NULL;
}

static NSCursor*
puglGetNsCursor(const PuglCursor cursor)
{
  switch (cursor) {
  case PUGL_CURSOR_ARROW:
    return [NSCursor arrowCursor];
  case PUGL_CURSOR_CARET:
    return [NSCursor IBeamCursor];
  case PUGL_CURSOR_CROSSHAIR:
    return [NSCursor crosshairCursor];
  case PUGL_CURSOR_HAND:
    return [NSCursor pointingHandCursor];
  case PUGL_CURSOR_NO:
    return [NSCursor operationNotAllowedCursor];
  case PUGL_CURSOR_LEFT_RIGHT:
    return [NSCursor resizeLeftRightCursor];
  case PUGL_CURSOR_UP_DOWN:
    return [NSCursor resizeUpDownCursor];
  case PUGL_CURSOR_UP_LEFT_DOWN_RIGHT:
    return extendedCursor(@selector(_windowResizeNorthWestSouthEastCursor));
  case PUGL_CURSOR_UP_RIGHT_DOWN_LEFT:
    return extendedCursor(@selector(_windowResizeNorthEastSouthWestCursor));
  case PUGL_CURSOR_ALL_SCROLL:
    return [NSCursor closedHandCursor];
  }

  return NULL;
}

PuglStatus
puglSetCursor(PuglView* view, PuglCursor cursor)
{
  PuglInternals* const impl = view->impl;
  NSCursor* const      cur  = puglGetNsCursor(cursor);
  if (!cur) {
    return PUGL_FAILURE;
  }

  impl->cursor = cur;

  if (impl->mouseTracked) {
    [cur set];
  }

  return PUGL_SUCCESS;
}

PuglStatus
puglSetClipboard(PuglView*         PUGL_UNUSED(view),
                 const char* const type,
                 const void* const data,
                 const size_t      len)
{
  NSPasteboard* const pasteboard = [NSPasteboard generalPasteboard];
  NSString* const     mimeType   = [NSString stringWithUTF8String:type];
  NSString* const     uti        = utiForMimeType(mimeType);
  NSData* const       blob       = [NSData dataWithBytes:data length:len];

  [pasteboard declareTypes:[NSArray arrayWithObjects:uti, nil] owner:nil];

  if ([pasteboard setData:blob forType:uti]) {
    return PUGL_SUCCESS;
  }

  return PUGL_FAILURE;
}
