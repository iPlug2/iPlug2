// Copyright 2012-2023 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "macros.h"
#include "types.h"
#include "x11.h"

#include "pugl/cairo.h"
#include "pugl/pugl.h"

#include <X11/Xutil.h>
#include <cairo-xlib.h>
#include <cairo.h>

#include <stdlib.h>

typedef struct {
  cairo_surface_t* back;
  cairo_surface_t* front;
  cairo_t*         cr;
} PuglX11CairoSurface;

static PuglViewSize
puglX11CairoGetViewSize(const PuglView* const view)
{
  PuglViewSize size = {0U, 0U};

  if (view->lastConfigure.type == PUGL_CONFIGURE) {
    // Use the size of the last configured frame
    size.width  = view->lastConfigure.width;
    size.height = view->lastConfigure.height;
  } else {
    // Use the default size
    size.width  = view->sizeHints[PUGL_DEFAULT_SIZE].width;
    size.height = view->sizeHints[PUGL_DEFAULT_SIZE].height;
  }

  return size;
}

static void
puglX11CairoClose(PuglView* view)
{
  PuglInternals* const       impl    = view->impl;
  PuglX11CairoSurface* const surface = (PuglX11CairoSurface*)impl->surface;

  cairo_surface_destroy(surface->front);
  cairo_surface_destroy(surface->back);
  surface->front = surface->back = NULL;
}

static PuglStatus
puglX11CairoOpen(PuglView* view, const PuglSpan width, const PuglSpan height)
{
  PuglInternals* const       impl    = view->impl;
  PuglX11CairoSurface* const surface = (PuglX11CairoSurface*)impl->surface;

  surface->back = cairo_xlib_surface_create(
    view->world->impl->display, impl->win, impl->vi->visual, width, height);

  surface->front = cairo_surface_create_similar(
    surface->back, cairo_surface_get_content(surface->back), width, height);

  if (cairo_surface_status(surface->back) ||
      cairo_surface_status(surface->front)) {
    puglX11CairoClose(view);
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  return PUGL_SUCCESS;
}

static PuglStatus
puglX11CairoCreate(PuglView* view)
{
  PuglInternals* const impl = view->impl;

  impl->surface = (cairo_surface_t*)calloc(1, sizeof(PuglX11CairoSurface));

  return PUGL_SUCCESS;
}

static void
puglX11CairoDestroy(PuglView* view)
{
  PuglInternals* const       impl    = view->impl;
  PuglX11CairoSurface* const surface = (PuglX11CairoSurface*)impl->surface;

  puglX11CairoClose(view);
  free(surface);
}

static PuglStatus
puglX11CairoEnter(PuglView* view, const PuglExposeEvent* expose)
{
  PuglInternals* const       impl    = view->impl;
  PuglX11CairoSurface* const surface = (PuglX11CairoSurface*)impl->surface;
  PuglStatus                 st      = PUGL_SUCCESS;

  if (expose) {
    const PuglViewSize viewSize      = puglX11CairoGetViewSize(view);
    const PuglSpan     right         = (PuglSpan)(expose->x + expose->width);
    const PuglSpan     bottom        = (PuglSpan)(expose->y + expose->height);
    const PuglSpan     surfaceWidth  = MAX(right, viewSize.width);
    const PuglSpan     surfaceHeight = MAX(bottom, viewSize.height);
    if (!(st = puglX11CairoOpen(view, surfaceWidth, surfaceHeight))) {
      surface->cr = cairo_create(surface->front);
      if (cairo_status(surface->cr)) {
        cairo_destroy(surface->cr);
        surface->cr = NULL;
        st          = PUGL_CREATE_CONTEXT_FAILED;
      }
    }
  }

  return st;
}

static PuglStatus
puglX11CairoLeave(PuglView* view, const PuglExposeEvent* expose)
{
  PuglInternals* const       impl    = view->impl;
  PuglX11CairoSurface* const surface = (PuglX11CairoSurface*)impl->surface;

  if (expose) {
    // Destroy front context and create a new one for drawing to the back
    cairo_destroy(surface->cr);
    surface->cr = cairo_create(surface->back);

    // Clip to expose region
    cairo_rectangle(
      surface->cr, expose->x, expose->y, expose->width, expose->height);
    cairo_clip(surface->cr);

    // Paint front onto back
    cairo_set_source_surface(surface->cr, surface->front, 0, 0);
    cairo_paint(surface->cr);

    // Flush to X and close everything
    cairo_destroy(surface->cr);
    cairo_surface_flush(surface->back);
    puglX11CairoClose(view);
    surface->cr = NULL;
  }

  return PUGL_SUCCESS;
}

static void*
puglX11CairoGetContext(PuglView* view)
{
  PuglInternals* const       impl    = view->impl;
  PuglX11CairoSurface* const surface = (PuglX11CairoSurface*)impl->surface;

  return surface->cr;
}

const PuglBackend*
puglCairoBackend(void)
{
  static const PuglBackend backend = {puglX11Configure,
                                      puglX11CairoCreate,
                                      puglX11CairoDestroy,
                                      puglX11CairoEnter,
                                      puglX11CairoLeave,
                                      puglX11CairoGetContext};

  return &backend;
}
