// Copyright 2012-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "stub.h"
#include "types.h"
#include "win.h"

#include "pugl/gl.h"

#include <windows.h>

#include <GL/gl.h>

#include <stdbool.h>
#include <stdlib.h>

#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_ACCELERATION_ARB 0x2003
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_PIXEL_TYPE_ARB 0x2013
#define WGL_RED_BITS_ARB 0x2015
#define WGL_GREEN_BITS_ARB 0x2017
#define WGL_BLUE_BITS_ARB 0x2019
#define WGL_ALPHA_BITS_ARB 0x201b
#define WGL_DEPTH_BITS_ARB 0x2022
#define WGL_STENCIL_BITS_ARB 0x2023
#define WGL_FULL_ACCELERATION_ARB 0x2027
#define WGL_TYPE_RGBA_ARB 0x202b
#define WGL_SAMPLE_BUFFERS_ARB 0x2041
#define WGL_SAMPLES_ARB 0x2042

#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_FLAGS_ARB 0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define WGL_CONTEXT_DEBUG_BIT_ARB 0x00000001

typedef HGLRC(WINAPI* WglCreateContextAttribs)(HDC, HGLRC, const int*);

typedef BOOL(WINAPI* WglSwapInterval)(int);

typedef BOOL(WINAPI* WglChoosePixelFormat)(HDC,
                                           const int*,
                                           const FLOAT*,
                                           UINT,
                                           int*,
                                           UINT*);

typedef struct {
  WglChoosePixelFormat    wglChoosePixelFormat;
  WglCreateContextAttribs wglCreateContextAttribs;
  WglSwapInterval         wglSwapInterval;
} PuglWinGlProcs;

typedef struct {
  PuglWinGlProcs procs;
  HGLRC          hglrc;
} PuglWinGlSurface;

// Struct to manage the fake window used during configuration
typedef struct {
  HWND hwnd;
  HDC  hdc;
} PuglFakeWindow;

static PuglStatus
puglWinError(PuglFakeWindow* fakeWin, const PuglStatus status)
{
  if (fakeWin->hwnd) {
    ReleaseDC(fakeWin->hwnd, fakeWin->hdc);
    DestroyWindow(fakeWin->hwnd);
  }

  return status;
}

static PuglWinGlProcs
puglWinGlGetProcs(void)
{
  const PuglWinGlProcs procs = {
    (WglChoosePixelFormat)(wglGetProcAddress("wglChoosePixelFormatARB")),
    (WglCreateContextAttribs)(wglGetProcAddress("wglCreateContextAttribsARB")),
    (WglSwapInterval)(wglGetProcAddress("wglSwapIntervalEXT"))};

  return procs;
}

static void
ensureHint(PuglView* const view, const PuglViewHint hint, const int value)
{
  if (view->hints[hint] == PUGL_DONT_CARE) {
    view->hints[hint] = value;
  }
}

static PuglStatus
puglWinGlConfigure(PuglView* view)
{
  PuglInternals* impl = view->impl;

  // Set attributes to default if they are unset
  // (There is no GLX_DONT_CARE equivalent on Windows)
  ensureHint(view, PUGL_DEPTH_BITS, 0);
  ensureHint(view, PUGL_STENCIL_BITS, 0);
  ensureHint(view, PUGL_SAMPLES, 0);
  ensureHint(view, PUGL_SAMPLE_BUFFERS, view->hints[PUGL_SAMPLES] > 0);
  ensureHint(view, PUGL_DOUBLE_BUFFER, 1);
  ensureHint(view, PUGL_SWAP_INTERVAL, 1);

  // clang-format off
  const int pixelAttrs[] = {
    WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
    WGL_ACCELERATION_ARB,   WGL_FULL_ACCELERATION_ARB,
    WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
    WGL_DOUBLE_BUFFER_ARB,  view->hints[PUGL_DOUBLE_BUFFER],
    WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
    WGL_SAMPLE_BUFFERS_ARB, view->hints[PUGL_SAMPLE_BUFFERS],
    WGL_SAMPLES_ARB,        view->hints[PUGL_SAMPLES],
    WGL_RED_BITS_ARB,       view->hints[PUGL_RED_BITS],
    WGL_GREEN_BITS_ARB,     view->hints[PUGL_GREEN_BITS],
    WGL_BLUE_BITS_ARB,      view->hints[PUGL_BLUE_BITS],
    WGL_ALPHA_BITS_ARB,     view->hints[PUGL_ALPHA_BITS],
    WGL_DEPTH_BITS_ARB,     view->hints[PUGL_DEPTH_BITS],
    WGL_STENCIL_BITS_ARB,   view->hints[PUGL_STENCIL_BITS],
    0,
  };
  // clang-format on

  PuglWinGlSurface* const surface =
    (PuglWinGlSurface*)calloc(1, sizeof(PuglWinGlSurface));
  impl->surface = surface;

  // Create fake window for getting at GL context
  PuglStatus         st      = PUGL_SUCCESS;
  PuglFakeWindow     fakeWin = {0, 0};
  static const char* title   = "Pugl Configuration";
  if ((st = puglWinCreateWindow(view, title, &fakeWin.hwnd, &fakeWin.hdc))) {
    return puglWinError(&fakeWin, st);
  }

  // Set pixel format for fake window
  const PuglWinPFD fakePfd  = puglWinGetPixelFormatDescriptor(view->hints);
  const int        fakePfId = ChoosePixelFormat(fakeWin.hdc, &fakePfd);
  if (!fakePfId || !SetPixelFormat(fakeWin.hdc, fakePfId, &fakePfd)) {
    return puglWinError(&fakeWin, PUGL_SET_FORMAT_FAILED);
  }

  // Create fake GL context to get at the functions we need
  HGLRC fakeRc = wglCreateContext(fakeWin.hdc);
  if (!fakeRc) {
    return puglWinError(&fakeWin, PUGL_CREATE_CONTEXT_FAILED);
  }

  // Enter fake context and get extension functions
  wglMakeCurrent(fakeWin.hdc, fakeRc);
  surface->procs = puglWinGlGetProcs();

  if (surface->procs.wglChoosePixelFormat) {
    // Choose pixel format based on attributes
    UINT numFormats = 0;
    if (!surface->procs.wglChoosePixelFormat(
          fakeWin.hdc, pixelAttrs, NULL, 1U, &impl->pfId, &numFormats)) {
      return puglWinError(&fakeWin, PUGL_SET_FORMAT_FAILED);
    }

    DescribePixelFormat(impl->hdc, impl->pfId, sizeof(impl->pfd), &impl->pfd);
  } else {
    // Modern extensions not available, use basic pixel format
    impl->pfd  = fakePfd;
    impl->pfId = fakePfId;
  }

  // Dispose of fake window and context
  wglMakeCurrent(NULL, NULL);
  wglDeleteContext(fakeRc);
  ReleaseDC(fakeWin.hwnd, fakeWin.hdc);
  DestroyWindow(fakeWin.hwnd);

  return PUGL_SUCCESS;
}

static PuglStatus
puglWinGlCreate(PuglView* view)
{
  PuglInternals* const    impl    = view->impl;
  PuglWinGlSurface* const surface = (PuglWinGlSurface*)impl->surface;
  PuglStatus              st      = PUGL_SUCCESS;

  const int contextAttribs[] = {
    WGL_CONTEXT_MAJOR_VERSION_ARB,
    view->hints[PUGL_CONTEXT_VERSION_MAJOR],

    WGL_CONTEXT_MINOR_VERSION_ARB,
    view->hints[PUGL_CONTEXT_VERSION_MINOR],

    WGL_CONTEXT_FLAGS_ARB,
    (view->hints[PUGL_CONTEXT_DEBUG] ? WGL_CONTEXT_DEBUG_BIT_ARB : 0),

    WGL_CONTEXT_PROFILE_MASK_ARB,
    ((view->hints[PUGL_CONTEXT_PROFILE] == PUGL_OPENGL_COMPATIBILITY_PROFILE)
       ? WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB
       : WGL_CONTEXT_CORE_PROFILE_BIT_ARB),

    0};

  // Create real window with desired pixel format
  if ((st = puglWinCreateWindow(view, "Pugl", &impl->hwnd, &impl->hdc))) {
    return st;
  }

  if (!SetPixelFormat(impl->hdc, impl->pfId, &impl->pfd)) {
    ReleaseDC(impl->hwnd, impl->hdc);
    DestroyWindow(impl->hwnd);
    impl->hwnd = NULL;
    impl->hdc  = NULL;
    return PUGL_SET_FORMAT_FAILED;
  }

  // Create GL context
  if (surface->procs.wglCreateContextAttribs &&
      !(surface->hglrc = surface->procs.wglCreateContextAttribs(
          impl->hdc, 0, contextAttribs))) {
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  if (!(surface->hglrc = wglCreateContext(impl->hdc))) {
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  // Enter context and set swap interval
  wglMakeCurrent(impl->hdc, surface->hglrc);
  const int swapInterval = view->hints[PUGL_SWAP_INTERVAL];
  if (surface->procs.wglSwapInterval && swapInterval != PUGL_DONT_CARE) {
    surface->procs.wglSwapInterval(swapInterval);
  }

  return PUGL_SUCCESS;
}

static void
puglWinGlDestroy(PuglView* view)
{
  PuglWinGlSurface* surface = (PuglWinGlSurface*)view->impl->surface;
  if (surface) {
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(surface->hglrc);
    free(surface);
    view->impl->surface = NULL;
  }
}

static PuglStatus
puglWinGlEnter(PuglView* view, const PuglExposeEvent* expose)
{
  PuglWinGlSurface* surface = (PuglWinGlSurface*)view->impl->surface;
  if (!surface || !surface->hglrc) {
    return PUGL_FAILURE;
  }

  const PuglStatus st = puglWinEnter(view, expose);
  wglMakeCurrent(view->impl->hdc, surface->hglrc);
  return st;
}

static PuglStatus
puglWinGlLeave(PuglView* view, const PuglExposeEvent* expose)
{
  if (expose) {
    SwapBuffers(view->impl->hdc);
  }

  wglMakeCurrent(NULL, NULL);
  return puglWinLeave(view, expose);
}

PuglGlFunc
puglGetProcAddress(const char* name)
{
  const PuglGlFunc func = (PuglGlFunc)wglGetProcAddress(name);

  /* Windows has the annoying property that wglGetProcAddress returns NULL
     for functions from OpenGL 1.1, so we fall back to pulling them directly
     from opengl32.dll */

  return func
           ? func
           : (PuglGlFunc)GetProcAddress(GetModuleHandle("opengl32.dll"), name);
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
  static const PuglBackend backend = {puglWinGlConfigure,
                                      puglWinGlCreate,
                                      puglWinGlDestroy,
                                      puglWinGlEnter,
                                      puglWinGlLeave,
                                      puglStubGetContext};

  return &backend;
}
