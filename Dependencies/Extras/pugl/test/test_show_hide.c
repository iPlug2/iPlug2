// Copyright 2020-2023 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

/*
  Tests the basic sanity of view/window create, configure, map, expose, unmap,
  and destroy events.
*/

#undef NDEBUG

#include "test_utils.h"

#include "pugl/pugl.h"
#include "pugl/stub.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
  START,
  REALIZED,
  MAPPED,
  EXPOSED,
  UNMAPPED,
  UNREALIZED,
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
  case PUGL_REALIZE:
    assert(test->state == START);
    test->state = REALIZED;
    break;
  case PUGL_CONFIGURE:
    if (event->configure.style & PUGL_VIEW_STYLE_MAPPED) {
      test->state = MAPPED;
    } else {
      test->state = UNMAPPED;
    }
    break;
  case PUGL_EXPOSE:
    assert(test->state == MAPPED || test->state == EXPOSED);
    test->state = EXPOSED;
    break;
  case PUGL_UNREALIZE:
    assert(test->state == UNMAPPED);
    test->state = UNREALIZED;
    break;
  default:
    break;
  }

  return PUGL_SUCCESS;
}

static void
tick(PuglWorld* world)
{
#ifdef __APPLE__
  // FIXME: Expose events are not events on MacOS, so we can't block
  // indefinitely here since it will block forever
  assert(!puglUpdate(world, 1 / 30.0));
#else
  assert(!puglUpdate(world, -1));
#endif
}

static void
showHide(PuglTest* const test)
{
  // Show and hide window a few times
  for (unsigned i = 0U; i < 3U; ++i) {
    assert(!puglShow(test->view, (PuglShowCommand)i));
    while (test->state != EXPOSED) {
      tick(test->world);
    }

    assert(puglGetVisible(test->view));
    assert(!puglHide(test->view));
    while (test->state != UNMAPPED) {
      tick(test->world);
    }
  }
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
  puglSetViewString(test.view, PUGL_WINDOW_TITLE, "Pugl Show/Hide Test");
  puglSetBackend(test.view, puglStubBackend());
  puglSetHandle(test.view, &test);
  puglSetEventFunc(test.view, onEvent);
  puglSetSizeHint(test.view, PUGL_DEFAULT_SIZE, 256, 256);
  puglSetPosition(test.view, 128, 384);

  // Create initially invisible window
  assert(!puglRealize(test.view));
  assert(!puglGetVisible(test.view));
  while (test.state < REALIZED) {
    tick(test.world);
  }

  // Show and hide window a couple of times
  showHide(&test);

  // Unrealize view
  assert(!puglGetVisible(test.view));
  assert(!puglUnrealize(test.view));
  assert(test.state == UNREALIZED);

  // Realize and show again
  test.state = START;
  assert(!puglRealize(test.view));
  showHide(&test);
  assert(!puglUnrealize(test.view));

  // Tear down
  puglFreeView(test.view);
  assert(test.state == UNREALIZED);
  puglFreeWorld(test.world);

  return 0;
}
