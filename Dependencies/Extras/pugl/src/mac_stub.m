// Copyright 2019-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "internal.h"
#include "mac.h"
#include "stub.h"

#include "pugl/stub.h"

#import <Cocoa/Cocoa.h>

@interface PuglStubView : NSView
@end

@implementation PuglStubView {
@public
  PuglView* puglview;
}

- (void)resizeWithOldSuperviewSize:(NSSize)oldSize
{
  PuglWrapperView* wrapper = (PuglWrapperView*)[self superview];

  [super resizeWithOldSuperviewSize:oldSize];
  [wrapper setReshaped];
}

- (void)drawRect:(NSRect)rect
{
  PuglWrapperView* wrapper = (PuglWrapperView*)[self superview];

  [wrapper dispatchExpose:rect];
}

@end

static PuglStatus
puglMacStubCreate(PuglView* view)
{
  PuglInternals* impl     = view->impl;
  PuglStubView*  drawView = [PuglStubView alloc];

  drawView->puglview = view;
  [drawView initWithFrame:NSMakeRect(0,
                                     0,
                                     view->lastConfigure.width,
                                     view->lastConfigure.height)];

  if (view->hints[PUGL_RESIZABLE]) {
    [drawView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
  } else {
    [drawView setAutoresizingMask:NSViewNotSizable];
  }

  impl->drawView = drawView;
  return PUGL_SUCCESS;
}

static void
puglMacStubDestroy(PuglView* view)
{
  PuglStubView* const drawView = (PuglStubView*)view->impl->drawView;

  [drawView removeFromSuperview];
  [drawView release];

  view->impl->drawView = nil;
}

const PuglBackend*
puglStubBackend(void)
{
  static const PuglBackend backend = {puglStubConfigure,
                                      puglMacStubCreate,
                                      puglMacStubDestroy,
                                      puglStubEnter,
                                      puglStubLeave,
                                      puglStubGetContext};

  return &backend;
}
