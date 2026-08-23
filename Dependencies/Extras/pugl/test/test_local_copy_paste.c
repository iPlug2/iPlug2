// Copyright 2020-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

// Tests copy and paste within the same view

#undef NDEBUG

#include "test_utils.h"

#include "pugl/pugl.h"
#include "pugl/stub.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uintptr_t timerId = 1U;

typedef enum {
  START,
  EXPOSED,
  PASTED,
  RECEIVED_OFFER,
  RECEIVED_DATA,
  FINISHED,
} State;

typedef struct {
  PuglWorld*      world;
  PuglView*       view;
  PuglTestOptions opts;
  size_t          iteration;
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
    if (test->state < EXPOSED) {
      // Start timer on first expose
      assert(!puglStartTimer(view, timerId, 1 / 60.0));
      test->state = EXPOSED;
    }
    break;

  case PUGL_TIMER:
    assert(event->timer.id == timerId);

    if (test->iteration == 0) {
      puglSetClipboard(
        view, "text/plain", "Copied Text", strlen("Copied Text") + 1);

      // Check that the new type is available immediately
      assert(puglGetNumClipboardTypes(view) >= 1);
      assert(!strcmp(puglGetClipboardType(view, 0), "text/plain"));

      size_t      len  = 0;
      const char* text = (const char*)puglGetClipboard(view, 0, &len);

      // Check that the new contents are available immediately
      assert(text);
      assert(!strcmp(text, "Copied Text"));

    } else if (test->iteration == 1) {
      size_t      len  = 0;
      const char* text = (const char*)puglGetClipboard(view, 0, &len);

      // Check that the contents we pasted last iteration are still there
      assert(text);
      assert(!strcmp(text, "Copied Text"));

    } else if (test->iteration == 2) {
      // Start a "proper" paste
      test->state = PASTED;
      assert(!puglPaste(view));
    }

    ++test->iteration;
    break;

  case PUGL_DATA_OFFER:
    if (test->state == PASTED) {
      test->state = RECEIVED_OFFER;

      assert(!puglAcceptOffer(view, &event->offer, 0));
    }
    break;

  case PUGL_DATA:
    if (test->state == RECEIVED_OFFER) {
      size_t      len  = 0;
      const char* text = (const char*)puglGetClipboard(view, 0, &len);

      // Check that the offered data is what we copied earlier
      assert(text);
      assert(!strcmp(text, "Copied Text"));

      test->state = FINISHED;
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
  PuglTest app = {puglNewWorld(PUGL_PROGRAM, 0),
                  NULL,
                  puglParseTestOptions(&argc, &argv),
                  0,
                  START};

  // Set up view
  app.view = puglNewView(app.world);
  puglSetWorldString(app.world, PUGL_CLASS_NAME, "PuglTest");
  puglSetViewString(app.view, PUGL_WINDOW_TITLE, "Pugl Copy/Paste Test");
  puglSetBackend(app.view, puglStubBackend());
  puglSetHandle(app.view, &app);
  puglSetEventFunc(app.view, onEvent);
  puglSetSizeHint(app.view, PUGL_DEFAULT_SIZE, 256, 256);
  puglSetPosition(app.view, 384, 128);

  // Create and show window
  assert(!puglRealize(app.view));
  assert(!puglShow(app.view, PUGL_SHOW_RAISE));

  // Run until the test is finished
  while (app.state != FINISHED) {
    assert(!puglUpdate(app.world, 1 / 15.0));
  }

  puglFreeView(app.view);
  puglFreeWorld(app.world);

  return 0;
}
