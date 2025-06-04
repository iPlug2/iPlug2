// Copyright 2012-2020 David Robillard <d@drobilla.net>
// Copyright 2017 Hanspeter Portner <dev@open-music-kontrollers.ch>
// SPDX-License-Identifier: ISC

#ifndef PUGL_SRC_MAC_H
#define PUGL_SRC_MAC_H

#include "pugl/pugl.h"

#import <Cocoa/Cocoa.h>

#include <mach/mach_time.h>

#include <stdint.h>

@interface PuglWrapperView : NSView<NSTextInputClient>

- (void)dispatchExpose:(NSRect)rect;
- (void)setReshaped;

@end

@interface PuglWindow : NSWindow
@end

struct PuglWorldInternalsImpl {
  NSApplication*            app;
  NSAutoreleasePool*        autoreleasePool;
  struct mach_timebase_info timebaseInfo;
};

struct PuglInternalsImpl {
  NSApplication*   app;
  PuglWrapperView* wrapperView;
  NSView*          drawView;
  NSCursor*        cursor;
  PuglWindow*      window;
  uint32_t         mods;
  bool             mouseTracked;
};

#endif // PUGL_SRC_MAC_H
