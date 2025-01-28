// Copyright 2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

/*
  Tests that update events are received and that the redisplays they trigger
  happen immediately in the same event loop iteration.
*/

#undef NDEBUG

#include "test_utils.h"

#include "pugl/pugl.h"
#include "pugl/stub.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __APPLE__
static const double timeout = 1 / 60.0;
#else
static const double timeout = -1.0;
#endif

typedef enum {
  START,
  EXPOSED1,
  UPDATED,
  EXPOSED2,
} State;

typedef struct {
  PuglWorld*      world;
  PuglView*       view;
  PuglTestOptions opts;
  State           state;
} PuglTest;

static PuglStatus
onEvent(PuglView* view, const PuglEvent* event)
{
  PuglTest* test = (PuglTest*)puglGetHandle(view);

  if (test->opts.verbose) {
    printEvent(event, "Event: ", true);
  }

  switch (event->type) {
  case PUGL_EXPOSE:
    switch (test->state) {
    case START:
      test->state = EXPOSED1;
      break;
    case UPDATED:
      test->state = EXPOSED2;
      break;
    default:
      break;
    }
    break;

  case PUGL_UPDATE:
    if (test->state == EXPOSED1) {
      test->state = UPDATED;
      puglPostRedisplay(view);
    }
    break;

  default:
    break;
  }

  return PUGL_SUCCESS;
}

int
main(int argc, char** argv)
{
  PuglTest test = {puglNewWorld(PUGL_PROGRAM, 0),
                   NULL,
                   puglParseTestOptions(&argc, &argv),
                   START};

  // Set up view
  test.view = puglNewView(test.world);
  puglSetWorldString(test.world, PUGL_CLASS_NAME, "PuglTest");
  puglSetViewString(test.view, PUGL_WINDOW_TITLE, "Pugl Update Test");
  puglSetBackend(test.view, puglStubBackend());
  puglSetHandle(test.view, &test);
  puglSetEventFunc(test.view, onEvent);
  puglSetSizeHint(test.view, PUGL_DEFAULT_SIZE, 256, 256);
  puglSetPosition(test.view, 128, 640);

  // Create and show window
  assert(!puglRealize(test.view));
  assert(!puglShow(test.view, PUGL_SHOW_RAISE));

  // Tick until an expose happens
  while (test.state < EXPOSED1) {
    assert(!puglUpdate(test.world, timeout));
    assert(test.state != UPDATED);
  }

  // Tick once and ensure the update and the expose it posted both happened
  assert(!puglUpdate(test.world, 0.0));
  assert(test.state == EXPOSED2);

  // Tear down
  puglFreeView(test.view);
  puglFreeWorld(test.world);

  return 0;
}
