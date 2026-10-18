// Copyright 2012-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "test/test_utils.h"

#include "pugl/pugl.h"
#include "pugl/stub.h"

#include <stdbool.h>
#include <stdio.h>

typedef struct {
  PuglWorld* world;
  PuglView*  view;
  int        quit;
} PuglPrintEventsApp;

static PuglStatus
onEvent(PuglView* view, const PuglEvent* event)
{
  PuglPrintEventsApp* app = (PuglPrintEventsApp*)puglGetHandle(view);

  printEvent(event, "Event: ", true);

  switch (event->type) {
  case PUGL_CLOSE:
    app->quit = 1;
    break;
  default:
    break;
  }

  return PUGL_SUCCESS;
}

int
main(void)
{
  PuglPrintEventsApp app = {NULL, NULL, 0};

  app.world = puglNewWorld(PUGL_PROGRAM, 0);
  app.view  = puglNewView(app.world);

  puglSetWorldString(app.world, PUGL_CLASS_NAME, "PuglPrintEvents");
  puglSetViewString(app.view, PUGL_WINDOW_TITLE, "Pugl Event Printer");
  puglSetSizeHint(app.view, PUGL_DEFAULT_SIZE, 512, 512);
  puglSetBackend(app.view, puglStubBackend());
  puglSetHandle(app.view, &app);
  puglSetEventFunc(app.view, onEvent);

  PuglStatus st = puglRealize(app.view);
  if (st) {
    return logError("Failed to create window (%s)\n", puglStrerror(st));
  }

  puglShow(app.view, PUGL_SHOW_RAISE);

  while (!app.quit) {
    puglUpdate(app.world, -1.0);
  }

  puglFreeView(app.view);
  puglFreeWorld(app.world);

  return 0;
}
