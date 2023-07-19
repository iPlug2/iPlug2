// Copyright 2019-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "internal.h"
#include "mac.h"
#include "stub.h"

#include "pugl/gl.h"

#ifndef __MAC_10_10
#  define NSOpenGLProfileVersion4_1Core NSOpenGLProfileVersion3_2Core
#endif

static void
ensureHint(PuglView* const view, const PuglViewHint hint, const int value)
{
  if (view->hints[hint] == PUGL_DONT_CARE) {
    view->hints[hint] = value;
  }
}

@interface PuglOpenGLView : NSOpenGLView
@end

@implementation PuglOpenGLView {
@public
  PuglView* puglview;
}

- (id)initWithFrame:(NSRect)frame
{
  const bool compat =
    puglview->hints[PUGL_CONTEXT_PROFILE] == PUGL_OPENGL_COMPATIBILITY_PROFILE;

  const unsigned samples = (unsigned)puglview->hints[PUGL_SAMPLES];
  const int      major   = puglview->hints[PUGL_CONTEXT_VERSION_MAJOR];
  const unsigned profile =
    ((compat || major < 3) ? NSOpenGLProfileVersionLegacy
                           : (major >= 4 ? NSOpenGLProfileVersion4_1Core
                                         : NSOpenGLProfileVersion3_2Core));

  // Set attributes to default if they are unset
  // (There is no GLX_DONT_CARE equivalent on MacOS)
  ensureHint(puglview, PUGL_DEPTH_BITS, 0);
  ensureHint(puglview, PUGL_STENCIL_BITS, 0);
  ensureHint(puglview, PUGL_SAMPLES, 1);
  ensureHint(puglview, PUGL_SAMPLE_BUFFERS, puglview->hints[PUGL_SAMPLES] > 0);
  ensureHint(puglview, PUGL_DOUBLE_BUFFER, 1);
  ensureHint(puglview, PUGL_SWAP_INTERVAL, 1);

  const unsigned colorSize = (unsigned)(puglview->hints[PUGL_RED_BITS] +
                                        puglview->hints[PUGL_BLUE_BITS] +
                                        puglview->hints[PUGL_GREEN_BITS] +
                                        puglview->hints[PUGL_ALPHA_BITS]);

  // clang-format off
  NSOpenGLPixelFormatAttribute pixelAttribs[17] = {
    NSOpenGLPFADoubleBuffer,
    NSOpenGLPFAAccelerated,
    NSOpenGLPFAOpenGLProfile, profile,
    NSOpenGLPFAColorSize,     colorSize,
    NSOpenGLPFADepthSize,     (unsigned)puglview->hints[PUGL_DEPTH_BITS],
    NSOpenGLPFAStencilSize,   (unsigned)puglview->hints[PUGL_STENCIL_BITS],
    NSOpenGLPFAMultisample,   samples ? 1U : 0U,
    NSOpenGLPFASampleBuffers, samples ? 1U : 0U,
    NSOpenGLPFASamples,       samples,
    0};
  // clang-format on

  NSOpenGLPixelFormat* pixelFormat =
    [[NSOpenGLPixelFormat alloc] initWithAttributes:pixelAttribs];

  if (pixelFormat) {
    self = [super initWithFrame:frame pixelFormat:pixelFormat];
    [pixelFormat release];
  } else {
    self = [super initWithFrame:frame];
  }

  [self setWantsBestResolutionOpenGLSurface:YES];

  if (self) {
    [[self openGLContext] makeCurrentContext];
    [self reshape];
    [NSOpenGLContext clearCurrentContext];
  }
  return self;
}

- (void)reshape
{
  PuglWrapperView* wrapper = (PuglWrapperView*)[self superview];

  [super reshape];
  [wrapper setReshaped];
}

- (void)drawRect:(NSRect)rect
{
  PuglWrapperView* wrapper = (PuglWrapperView*)[self superview];
  [wrapper dispatchExpose:rect];
}

@end

static PuglStatus
puglMacGlCreate(PuglView* view)
{
  PuglInternals*  impl     = view->impl;
  PuglOpenGLView* drawView = [PuglOpenGLView alloc];

  drawView->puglview = view;
  [drawView initWithFrame:[impl->wrapperView bounds]];
  if (view->hints[PUGL_RESIZABLE]) {
    [drawView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
  } else {
    [drawView setAutoresizingMask:NSViewNotSizable];
  }

  impl->drawView = drawView;
  return PUGL_SUCCESS;
}

static void
puglMacGlDestroy(PuglView* view)
{
  PuglOpenGLView* const drawView = (PuglOpenGLView*)view->impl->drawView;

  [drawView removeFromSuperview];
  [drawView release];

  view->impl->drawView = nil;
}

static PuglStatus
puglMacGlEnter(PuglView* view, const PuglExposeEvent* PUGL_UNUSED(expose))
{
  PuglOpenGLView* const drawView = (PuglOpenGLView*)view->impl->drawView;
  if (!drawView) {
    return PUGL_FAILURE;
  }

  [[drawView openGLContext] makeCurrentContext];
  return PUGL_SUCCESS;
}

static PuglStatus
puglMacGlLeave(PuglView* view, const PuglExposeEvent* expose)
{
  PuglOpenGLView* const drawView = (PuglOpenGLView*)view->impl->drawView;

  if (expose) {
    [[drawView openGLContext] flushBuffer];
  }

  [NSOpenGLContext clearCurrentContext];

  return PUGL_SUCCESS;
}

PuglGlFunc
puglGetProcAddress(const char* name)
{
  CFBundleRef framework =
    CFBundleGetBundleWithIdentifier(CFSTR("com.apple.opengl"));

  CFStringRef symbol = CFStringCreateWithCString(
    kCFAllocatorDefault, name, kCFStringEncodingASCII);

  PuglGlFunc func =
    (PuglGlFunc)CFBundleGetFunctionPointerForName(framework, symbol);

  CFRelease(symbol);

  return func;
}

PuglStatus
puglEnterContext(PuglView* view)
{
  return view->backend->enter(view, NULL);
}

PuglStatus
puglLeaveContext(PuglView* view)
{
  return view->backend->leave(view, NULL);
}

const PuglBackend*
puglGlBackend(void)
{
  static const PuglBackend backend = {puglStubConfigure,
                                      puglMacGlCreate,
                                      puglMacGlDestroy,
                                      puglMacGlEnter,
                                      puglMacGlLeave,
                                      puglStubGetContext};

  return &backend;
}
