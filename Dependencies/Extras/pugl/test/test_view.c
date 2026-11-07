// Copyright 2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

// Tests basic view setup

#undef NDEBUG

#include "test_utils.h"

#include "pugl/pugl.h"
#include "pugl/stub.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

typedef enum {
  START,
  REALIZED,
  CONFIGURED,
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
    assert(test->state == REALIZED);
    test->state = CONFIGURED;
    break;
  case PUGL_UNREALIZE:
    test->state = UNREALIZED;
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
  puglSetViewString(test.view, PUGL_WINDOW_TITLE, "Pugl View Test");
  puglSetBackend(test.view, puglStubBackend());
  puglSetHandle(test.view, &test);
  puglSetEventFunc(test.view, onEvent);
  puglSetSizeHint(test.view, PUGL_DEFAULT_SIZE, 256, 256);
  puglSetPosition(test.view, 384, 640);

  // Check basic accessors
  assert(puglGetBackend(test.view) == puglStubBackend());
  assert(!strcmp(puglGetWorldString(test.world, PUGL_CLASS_NAME), "PuglTest"));
  assert(
    !strcmp(puglGetViewString(test.view, PUGL_WINDOW_TITLE), "Pugl View Test"));

  // Create and show window
  assert(!puglRealize(test.view));
  assert(!puglShow(test.view, PUGL_SHOW_RAISE));
  while (test.state < CONFIGURED) {
    assert(!puglUpdate(test.world, -1.0));
  }

  // Check that puglGetNativeView() returns something
  assert(puglGetNativeView(test.view));

  // Tear down
  puglFreeView(test.view);
  puglFreeWorld(test.world);

  return 0;
}
