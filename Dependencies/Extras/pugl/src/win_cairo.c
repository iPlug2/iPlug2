// Copyright 2012-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "stub.h"
#include "types.h"
#include "win.h"

#include "pugl/cairo.h"

#include <cairo-win32.h>
#include <cairo.h>

#include <stdlib.h>

typedef struct {
  cairo_surface_t* surface;
  cairo_t*         cr;
  HDC              drawDc;
  HBITMAP          drawBitmap;
} PuglWinCairoSurface;

static PuglStatus
puglWinCairoCreateDrawContext(PuglView* view)
{
  PuglInternals* const       impl    = view->impl;
  PuglWinCairoSurface* const surface = (PuglWinCairoSurface*)impl->surface;

  surface->drawDc     = CreateCompatibleDC(impl->hdc);
  surface->drawBitmap = CreateCompatibleBitmap(
    impl->hdc, (int)view->lastConfigure.width, (int)view->lastConfigure.height);

  DeleteObject(SelectObject(surface->drawDc, surface->drawBitmap));

  return PUGL_SUCCESS;
}

static PuglStatus
puglWinCairoDestroyDrawContext(PuglView* view)
{
  PuglInternals* const       impl    = view->impl;
  PuglWinCairoSurface* const surface = (PuglWinCairoSurface*)impl->surface;

  DeleteDC(surface->drawDc);
  DeleteObject(surface->drawBitmap);

  surface->drawDc     = NULL;
  surface->drawBitmap = NULL;

  return PUGL_SUCCESS;
}

static PuglStatus
puglWinCairoConfigure(PuglView* view)
{
  const PuglStatus st = puglWinConfigure(view);

  if (!st) {
    view->impl->surface =
      (PuglWinCairoSurface*)calloc(1, sizeof(PuglWinCairoSurface));
  }

  return st;
}

static void
puglWinCairoClose(PuglView* view)
{
  PuglInternals* const       impl    = view->impl;
  PuglWinCairoSurface* const surface = (PuglWinCairoSurface*)impl->surface;

  cairo_surface_destroy(surface->surface);

  surface->surface = NULL;
}

static PuglStatus
puglWinCairoOpen(PuglView* view)
{
  PuglInternals* const       impl    = view->impl;
  PuglWinCairoSurface* const surface = (PuglWinCairoSurface*)impl->surface;

  if (!(surface->surface = cairo_win32_surface_create(surface->drawDc)) ||
      cairo_surface_status(surface->surface) ||
      !(surface->cr = cairo_create(surface->surface)) ||
      cairo_status(surface->cr)) {
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  return PUGL_SUCCESS;
}

static void
puglWinCairoDestroy(PuglView* view)
{
  PuglInternals* const       impl    = view->impl;
  PuglWinCairoSurface* const surface = (PuglWinCairoSurface*)impl->surface;

  puglWinCairoClose(view);
  puglWinCairoDestroyDrawContext(view);
  free(surface);
  impl->surface = NULL;
}

static PuglStatus
puglWinCairoEnter(PuglView* view, const PuglExposeEvent* expose)
{
  PuglStatus st = PUGL_SUCCESS;

  if (expose && !(st = puglWinCairoCreateDrawContext(view)) &&
      !(st = puglWinCairoOpen(view))) {
    st = puglWinEnter(view, expose);
  }

  return st;
}

static PuglStatus
puglWinCairoLeave(PuglView* view, const PuglExposeEvent* expose)
{
  PuglInternals* const       impl    = view->impl;
  PuglWinCairoSurface* const surface = (PuglWinCairoSurface*)impl->surface;

  if (expose) {
    cairo_surface_flush(surface->surface);
    BitBlt(impl->hdc,
           0,
           0,
           (int)view->lastConfigure.width,
           (int)view->lastConfigure.height,
           surface->drawDc,
           0,
           0,
           SRCCOPY);

    puglWinCairoClose(view);
    puglWinCairoDestroyDrawContext(view);
  }

  return puglWinLeave(view, expose);
}

static void*
puglWinCairoGetContext(PuglView* view)
{
  return ((PuglWinCairoSurface*)view->impl->surface)->cr;
}

const PuglBackend*
puglCairoBackend(void)
{
  static const PuglBackend backend = {puglWinCairoConfigure,
                                      puglStubCreate,
                                      puglWinCairoDestroy,
                                      puglWinCairoEnter,
                                      puglWinCairoLeave,
                                      puglWinCairoGetContext};

  return &backend;
}
