// Copyright 2012-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

// A demonstration of using clipboards for copy/paste and drag and drop

#include "cube_view.h"
#include "test/test_utils.h"

#include "pugl/gl.h"
#include "pugl/pugl.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  PuglView* view;
  double    xAngle;
  double    yAngle;
  double    lastMouseX;
  double    lastMouseY;
  double    lastDrawTime;
  bool      entered;
} CubeView;

typedef struct {
  PuglWorld* world;
  CubeView   cube;
  int        quit;
  bool       continuous;
  bool       verbose;
} PuglTestApp;

static void
onDisplay(PuglView* view)
{
  PuglWorld*   world = puglGetWorld(view);
  PuglTestApp* app   = (PuglTestApp*)puglGetWorldHandle(world);
  CubeView*    cube  = (CubeView*)puglGetHandle(view);

  const double thisTime = puglGetTime(app->world);
  if (app->continuous) {
    const double dTime = thisTime - cube->lastDrawTime;

    cube->xAngle = fmod(cube->xAngle + dTime * 100.0, 360.0);
    cube->yAngle = fmod(cube->yAngle + dTime * 100.0, 360.0);
  }

  displayCube(
    view, 10.0f, (float)cube->xAngle, (float)cube->yAngle, cube->entered);

  cube->lastDrawTime = thisTime;
}

static void
onKeyPress(PuglView* const view, const PuglKeyEvent* const event)
{
  static const char* const copyString = "Pugl test";

  PuglWorld* const   world = puglGetWorld(view);
  PuglTestApp* const app   = (PuglTestApp*)puglGetWorldHandle(world);

  if (event->key == 'q' || event->key == PUGL_KEY_ESCAPE) {
    app->quit = 1;
  } else if ((event->state & PUGL_MOD_CTRL) && event->key == 'c') {
    puglSetClipboard(view, "text/plain", copyString, strlen(copyString));

    fprintf(stderr, "Copy \"%s\"\n", copyString);
  } else if ((event->state & PUGL_MOD_CTRL) && event->key == 'v') {
    puglPaste(view);
  }
}

static void
onDataOffer(PuglView* view, const PuglDataOfferEvent* event)
{
  const uint32_t numTypes = puglGetNumClipboardTypes(view);

  // Print all offered types to be useful as a testing program
  fprintf(stderr, "Offered %u types:\n", numTypes);
  for (uint32_t t = 0; t < numTypes; ++t) {
    const char* type = puglGetClipboardType(view, t);
    fprintf(stderr, "\t%s\n", type);
  }

  // Accept the first type found that we support (namely text)
  for (uint32_t t = 0; t < numTypes; ++t) {
    const char* type = puglGetClipboardType(view, t);
    if (!strncmp(type, "text/", 5)) {
      puglAcceptOffer(view, event, t);
      return;
    }
  }
}

static void
onData(PuglView* view, const PuglDataEvent* event)
{
  const uint32_t typeIndex = event->typeIndex;

  const char* const type = puglGetClipboardType(view, typeIndex);

  fprintf(stderr, "Received data type: %s\n", type);
  if (!strncmp(type, "text/", 5)) {
    // Accept any text type
    size_t      len  = 0;
    const void* data = puglGetClipboard(view, typeIndex, &len);

    fprintf(stderr, "Data:\n%s\n", (const char*)data);
  }
}

static void
redisplayView(PuglTestApp* app, PuglView* view)
{
  if (!app->continuous) {
    puglPostRedisplay(view);
  }
}

static PuglStatus
onEvent(PuglView* view, const PuglEvent* event)
{
  PuglWorld*   world = puglGetWorld(view);
  PuglTestApp* app   = (PuglTestApp*)puglGetWorldHandle(world);
  CubeView*    cube  = (CubeView*)puglGetHandle(view);

  printEvent(event, "Event: ", app->verbose);

  switch (event->type) {
  case PUGL_CONFIGURE:
    reshapeCube((float)event->configure.width, (float)event->configure.height);
    break;
  case PUGL_UPDATE:
    if (app->continuous) {
      puglPostRedisplay(view);
    }
    break;
  case PUGL_EXPOSE:
    onDisplay(view);
    break;
  case PUGL_CLOSE:
    app->quit = 1;
    break;
  case PUGL_KEY_PRESS:
    onKeyPress(view, &event->key);
    break;
  case PUGL_MOTION:
#if defined(__GNUC__) && (__GNUC__ >= 5)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif
    if (isnan(cube->lastMouseX)) {
      cube->lastMouseX = event->motion.x;
    }
    if (isnan(cube->lastMouseY)) {
      cube->lastMouseY = event->motion.y;
    }
#if defined(__GNUC__) && (__GNUC__ >= 5)
#  pragma GCC diagnostic pop
#endif
    cube->xAngle -= (event->motion.x - cube->lastMouseX) / 2.0;
    cube->yAngle += (event->motion.y - cube->lastMouseY) / 2.0;
    cube->lastMouseX = event->motion.x;
    cube->lastMouseY = event->motion.y;
    redisplayView(app, view);
    break;
  case PUGL_POINTER_IN:
    cube->entered = true;
    redisplayView(app, view);
    break;
  case PUGL_POINTER_OUT:
    cube->entered = false;
    redisplayView(app, view);
    break;
  case PUGL_FOCUS_IN:
  case PUGL_FOCUS_OUT:
    redisplayView(app, view);
    break;
  case PUGL_DATA_OFFER:
    onDataOffer(view, &event->offer);
    break;
  case PUGL_DATA:
    onData(view, &event->data);
    break;
  default:
    break;
  }

  return PUGL_SUCCESS;
}

int
main(int argc, char** argv)
{
  // Parse command line options
  const PuglTestOptions opts = puglParseTestOptions(&argc, &argv);
  if (opts.help) {
    puglPrintTestUsage(argv[0], "");
    return 1;
  }

  PuglTestApp app = {0};

  app.world           = puglNewWorld(PUGL_PROGRAM, 0);
  app.cube.view       = puglNewView(app.world);
  app.cube.xAngle     = 30.0;
  app.cube.yAngle     = -30.0;
  app.cube.lastMouseX = (double)NAN;
  app.cube.lastMouseY = (double)NAN;
  app.verbose         = opts.verbose;
  app.continuous      = opts.continuous;

  PuglStatus      st   = PUGL_SUCCESS;
  PuglView* const view = app.cube.view;

  puglSetWorldHandle(app.world, &app);
  puglSetWorldString(app.world, PUGL_CLASS_NAME, "Pugl Test");

  puglSetViewString(view, PUGL_WINDOW_TITLE, "Pugl Clipboard Demo");
  puglSetSizeHint(view, PUGL_DEFAULT_SIZE, 512, 512);
  puglSetSizeHint(view, PUGL_MIN_SIZE, 128, 128);
  puglSetBackend(view, puglGlBackend());

  puglSetViewHint(view, PUGL_CONTEXT_DEBUG, opts.errorChecking);
  puglSetViewHint(view, PUGL_RESIZABLE, opts.resizable);
  puglSetViewHint(view, PUGL_SAMPLES, opts.samples);
  puglSetViewHint(view, PUGL_DOUBLE_BUFFER, opts.doubleBuffer);
  puglSetViewHint(view, PUGL_SWAP_INTERVAL, opts.sync);
  puglSetViewHint(view, PUGL_IGNORE_KEY_REPEAT, opts.ignoreKeyRepeat);
  puglSetHandle(view, &app.cube);
  puglSetEventFunc(view, onEvent);

  if ((st = puglRealize(view))) {
    return logError("Failed to realize view (%s)\n", puglStrerror(st));
  }

  if ((st = puglShow(view, PUGL_SHOW_RAISE))) {
    return logError("Failed to show view (%s)\n", puglStrerror(st));
  }

  while (!app.quit) {
    puglUpdate(app.world, app.continuous ? 0.0 : -1.0);
  }

  puglFreeView(app.cube.view);
  puglFreeWorld(app.world);

  return 0;
}
