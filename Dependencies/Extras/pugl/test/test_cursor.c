// Copyright 2021-2023 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

// Basic test that ensures changing the cursor seems to work

#undef NDEBUG

#include "test_utils.h"

#include "pugl/pugl.h"
#include "pugl/stub.h"

#include <assert.h>
#include <stdbool.h>

typedef struct {
  PuglWorld*      world;
  PuglView*       view;
  PuglTestOptions opts;
  bool            exposed;
} PuglTest;

static PuglStatus
onEvent(PuglView* const view, const PuglEvent* const event)
{
  PuglTest* const test = (PuglTest*)puglGetHandle(view);

  if (test->opts.verbose) {
    printEvent(event, "Event: ", true);
  }

  if (event->type == PUGL_EXPOSE) {
    assert(!puglGetContext(view));
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
  puglSetViewString(test.view, PUGL_WINDOW_TITLE, "Pugl Cursor Test");
  puglSetHandle(test.view, &test);
  puglSetBackend(test.view, puglStubBackend());
  puglSetEventFunc(test.view, onEvent);
  puglSetSizeHint(test.view, PUGL_DEFAULT_SIZE, 256, 256);
  puglSetPosition(test.view, 896, 640);
  puglShow(test.view, PUGL_SHOW_RAISE);

  // Drive event loop until the view gets exposed
  while (!test.exposed) {
    puglUpdate(test.world, -1.0);
  }

  // Change the cursor, updating each time
  assert(puglSetCursor(test.view, (PuglCursor)-1));
  for (unsigned i = 0; i < (unsigned)PUGL_CURSOR_ALL_SCROLL; ++i) {
    assert(!puglSetCursor(test.view, (PuglCursor)i));
    assert(!puglUpdate(test.world, 0.1));
  }

  // Tear down
  puglFreeView(test.view);
  puglFreeWorld(test.world);

  return 0;
}
