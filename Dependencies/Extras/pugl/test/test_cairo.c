// Copyright 2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

// Tests that creating a view with a Cairo backend works

#undef NDEBUG

#include "test_utils.h"

#include "pugl/cairo.h"
#include "pugl/pugl.h"

#include <cairo.h>

#include <assert.h>
#include <stdbool.h>

typedef struct {
  PuglWorld*      world;
  PuglView*       view;
  PuglTestOptions opts;
  bool            exposed;
} PuglTest;

static void
onExpose(PuglView* const view, const PuglExposeEvent* const event)
{
  cairo_t* const cr = (cairo_t*)puglGetContext(view);

  assert(cr);

  cairo_rectangle(cr, event->x, event->y, event->width, event->height);
  cairo_set_source_rgb(cr, 0, 1, 0);
  cairo_fill(cr);
}

static PuglStatus
onEvent(PuglView* const view, const PuglEvent* const event)
{
  PuglTest* const test = (PuglTest*)puglGetHandle(view);

  if (test->opts.verbose) {
    printEvent(event, "Event: ", true);
  }

  if (event->type == PUGL_EXPOSE) {
    onExpose(view, &event->expose);
    test->exposed = true;
  }

  return PUGL_SUCCESS;
}

int
main(int argc, char** argv)
{
  PuglWorld* const      world = puglNewWorld(PUGL_PROGRAM, 0);
  PuglView* const       view  = puglNewView(world);
  const PuglTestOptions opts  = puglParseTestOptions(&argc, &argv);
  PuglTest              test  = {world, view, opts, false};

  // Set up and show view
  puglSetWorldString(test.world, PUGL_CLASS_NAME, "PuglTest");
  puglSetViewString(test.view, PUGL_WINDOW_TITLE, "Pugl Cairo Test");
  puglSetHandle(test.view, &test);
  puglSetBackend(test.view, puglCairoBackend());
  puglSetEventFunc(test.view, onEvent);
  puglSetSizeHint(test.view, PUGL_DEFAULT_SIZE, 256, 256);
  puglSetPosition(test.view, 128, 896);
  puglShow(test.view, PUGL_SHOW_RAISE);

  // Drive event loop until the view gets exposed
  while (!test.exposed) {
    puglUpdate(test.world, -1.0);
  }

  // Tear down
  puglFreeView(test.view);
  puglFreeWorld(test.world);

  return 0;
}
