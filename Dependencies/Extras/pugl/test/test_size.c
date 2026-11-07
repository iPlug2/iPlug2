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
  PuglRect        configuredFrame;
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
    if (test->state == REALIZED) {
      test->state = CONFIGURED;
    }
    test->configuredFrame.x      = event->configure.x;
    test->configuredFrame.y      = event->configure.y;
    test->configuredFrame.width  = event->configure.width;
    test->configuredFrame.height = event->configure.height;
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
  static const PuglSpan minSize     = 128;
  static const PuglSpan defaultSize = 256;
  static const PuglSpan maxSize     = 512;

  PuglTest test = {puglNewWorld(PUGL_PROGRAM, 0),
                   NULL,
                   puglParseTestOptions(&argc, &argv),
                   START,
                   {0, 0, 0U, 0U}};

  // Set up view with size bounds and an aspect ratio
  test.view = puglNewView(test.world);
  puglSetWorldString(test.world, PUGL_CLASS_NAME, "PuglTest");
  puglSetViewString(test.view, PUGL_WINDOW_TITLE, "Pugl Size Test");
  puglSetBackend(test.view, puglStubBackend());
  puglSetHandle(test.view, &test);
  puglSetEventFunc(test.view, onEvent);
  puglSetViewHint(test.view, PUGL_RESIZABLE, PUGL_TRUE);
  puglSetSizeHint(test.view, PUGL_DEFAULT_SIZE, defaultSize, defaultSize);
  puglSetSizeHint(test.view, PUGL_MIN_SIZE, minSize, minSize);
  puglSetSizeHint(test.view, PUGL_MAX_SIZE, maxSize, maxSize);
  puglSetSizeHint(test.view, PUGL_FIXED_ASPECT, 1, 1);
  puglSetPosition(test.view, 384, 384);

  // Create and show window
  assert(!puglRealize(test.view));
  assert(!puglShow(test.view, PUGL_SHOW_RAISE));
  while (test.state < CONFIGURED) {
    assert(!puglUpdate(test.world, -1.0));
  }

  // Check that the frame matches the last configure event
  const PuglRect frame = puglGetFrame(test.view);
  assert(frame.x == test.configuredFrame.x);
  assert(frame.y == test.configuredFrame.y);
  assert(frame.width == test.configuredFrame.width);
  assert(frame.height == test.configuredFrame.height);

#if defined(_WIN32) || defined(__APPLE__)
  /* Some window managers on Linux (particularly tiling ones) just disregard
     these hints entirely, so we only check that the size is in bounds on MacOS
     and Windows where this is more or less universally supported. */

  assert(frame.width >= minSize);
  assert(frame.height >= minSize);
  assert(frame.width <= maxSize);
  assert(frame.height <= maxSize);
#endif

  // Tear down
  puglFreeView(test.view);
  puglFreeWorld(test.world);

  return 0;
}
