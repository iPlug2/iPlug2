// Copyright 2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

/*
  Tests that redisplays posted in the event handler are dispatched at the end
  of the same event loop iteration.
*/

#undef NDEBUG

#include "test_utils.h"

#include "pugl/pugl.h"
#include "pugl/stub.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __APPLE__
static const double timeout = 1 / 60.0;
#else
static const double timeout = -1.0;
#endif

#define STATES        \
  X(START)            \
  X(EXPOSED)          \
  X(SHOULD_REDISPLAY) \
  X(POSTED_REDISPLAY) \
  X(REDISPLAYED)      \
  X(REREDISPLAYED)

#define X(state) state,

typedef enum { STATES } State;

#undef X

#define X(state) #state,

static const char* const state_names[] = {STATES};

#undef X

typedef struct {
  PuglWorld*      world;
  PuglView*       view;
  PuglTestOptions opts;
  State           state;
} PuglTest;

static const PuglRect  redisplayRect   = {2, 4, 8, 16};
static const uintptr_t postRedisplayId = 42;

static PuglStatus
onEvent(PuglView* view, const PuglEvent* event)
{
  PuglTest* test = (PuglTest*)puglGetHandle(view);

  if (test->opts.verbose) {
    fprintf(stderr, "%-16s", state_names[test->state]);
    printEvent(event, " ", true);
  }

  switch (event->type) {
  case PUGL_UPDATE:
    if (test->state == SHOULD_REDISPLAY) {
      puglPostRedisplayRect(view, redisplayRect);
      test->state = POSTED_REDISPLAY;
    }
    break;

  case PUGL_EXPOSE:
    if (test->state == START) {
      test->state = EXPOSED;
    } else if (test->state == POSTED_REDISPLAY &&
               event->expose.x <= redisplayRect.x &&
               event->expose.y <= redisplayRect.y &&
               (event->expose.x + event->expose.width >=
                redisplayRect.x + redisplayRect.width) &&
               (event->expose.y + event->expose.height >=
                redisplayRect.y + redisplayRect.height)) {
      test->state = REDISPLAYED;
    } else if (test->state == REDISPLAYED) {
      test->state = REREDISPLAYED;
    }
    break;

  case PUGL_CLIENT:
    if (event->client.data1 == postRedisplayId) {
      test->state = SHOULD_REDISPLAY;
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
  puglSetViewString(test.view, PUGL_WINDOW_TITLE, "Pugl Redisplay Test");
  puglSetBackend(test.view, puglStubBackend());
  puglSetHandle(test.view, &test);
  puglSetEventFunc(test.view, onEvent);
  puglSetSizeHint(test.view, PUGL_DEFAULT_SIZE, 256, 256);
  puglSetPosition(test.view, 896, 128);

  // Create and show window
  assert(!puglRealize(test.view));
  assert(!puglShow(test.view, PUGL_SHOW_RAISE));
  while (test.state != EXPOSED) {
    assert(!puglUpdate(test.world, timeout));
  }

  // Send a custom event to trigger a redisplay in the event loop
  PuglEvent client_event    = {{PUGL_CLIENT, 0}};
  client_event.client.data1 = postRedisplayId;
  client_event.client.data2 = 0;
  assert(!puglSendEvent(test.view, &client_event));

  // Loop until an expose happens in the same iteration as the redisplay
  test.state = SHOULD_REDISPLAY;
  while (test.state != REDISPLAYED) {
    assert(!puglUpdate(test.world, timeout));
    assert(test.state != POSTED_REDISPLAY);
  }

  // Redisplay from outside the event handler
  puglPostRedisplay(test.view);
  while (test.state != REREDISPLAYED) {
    assert(!puglUpdate(test.world, timeout));
  }

  // Tear down
  puglFreeView(test.view);
  puglFreeWorld(test.world);

  return 0;
}
