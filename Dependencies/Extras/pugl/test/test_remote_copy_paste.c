// Copyright 2020-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

// Tests copy and paste from one view to another

#undef NDEBUG

#include "test_utils.h"

#include "pugl/pugl.h"
#include "pugl/stub.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uintptr_t copierTimerId = 1U;
static const uintptr_t pasterTimerId = 2U;

typedef enum {
  START,
  EXPOSED,
  COPIED,
  PASTED,
  RECEIVED_OFFER,
  FINISHED,
} State;

typedef struct {
  PuglWorld*      world;
  PuglView*       copierView;
  PuglView*       pasterView;
  PuglTestOptions opts;
  State           state;
  bool            copierStarted;
  bool            pasterStarted;
} PuglTest;

static PuglStatus
onCopierEvent(PuglView* const view, const PuglEvent* const event)
{
  PuglTest* const test = (PuglTest*)puglGetHandle(view);

  if (test->opts.verbose) {
    printEvent(event, "Copier Event: ", true);
  }

  switch (event->type) {
  case PUGL_EXPOSE:
    if (!test->copierStarted) {
      // Start timer on first expose
      assert(!puglStartTimer(view, copierTimerId, 1 / 15.0));
      test->copierStarted = true;
    }
    break;

  case PUGL_TIMER:
    assert(event->timer.id == copierTimerId);

    if (test->state < COPIED) {
      puglSetClipboard(
        view, "text/plain", "Copied Text", strlen("Copied Text") + 1);

      test->state = COPIED;
    }

    break;

  default:
    break;
  }

  return PUGL_SUCCESS;
}

static PuglStatus
onPasterEvent(PuglView* const view, const PuglEvent* const event)
{
  PuglTest* const test = (PuglTest*)puglGetHandle(view);

  if (test->opts.verbose) {
    printEvent(event, "Paster Event: ", true);
  }

  switch (event->type) {
  case PUGL_EXPOSE:
    if (!test->pasterStarted) {
      // Start timer on first expose
      assert(!puglStartTimer(view, pasterTimerId, 1 / 60.0));
      test->pasterStarted = true;
    }
    break;

  case PUGL_TIMER:
    assert(event->timer.id == pasterTimerId);
    if (test->state == COPIED) {
      test->state = PASTED;
      assert(!puglPaste(view));
    }
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
                  NULL,
                  puglParseTestOptions(&argc, &argv),
                  START,
                  false,
                  false};

  // Set up copier view
  app.copierView = puglNewView(app.world);
  puglSetWorldString(app.world, PUGL_CLASS_NAME, "PuglTest");
  puglSetViewString(app.copierView, PUGL_WINDOW_TITLE, "Pugl Copy Test");
  puglSetBackend(app.copierView, puglStubBackend());
  puglSetHandle(app.copierView, &app);
  puglSetEventFunc(app.copierView, onCopierEvent);
  puglSetSizeHint(app.copierView, PUGL_DEFAULT_SIZE, 256, 256);
  puglSetPosition(app.copierView, 640, 896);

  // Set up paster view
  app.pasterView = puglNewView(app.world);
  puglSetWorldString(app.world, PUGL_CLASS_NAME, "PuglTest");
  puglSetViewString(app.pasterView, PUGL_WINDOW_TITLE, "Pugl Paste Test");
  puglSetBackend(app.pasterView, puglStubBackend());
  puglSetHandle(app.pasterView, &app);
  puglSetEventFunc(app.pasterView, onPasterEvent);
  puglSetSizeHint(app.pasterView, PUGL_DEFAULT_SIZE, 256, 256);
  puglSetPosition(app.pasterView, 896, 896);

  // Create and show both views
  assert(!puglShow(app.copierView, PUGL_SHOW_RAISE));
  assert(!puglShow(app.pasterView, PUGL_SHOW_RAISE));

  // Run until the test is finished
  while (app.state != FINISHED) {
    assert(!puglUpdate(app.world, 1 / 60.0));
  }

  puglFreeView(app.copierView);
  puglFreeView(app.pasterView);
  puglFreeWorld(app.world);

  return 0;
}
