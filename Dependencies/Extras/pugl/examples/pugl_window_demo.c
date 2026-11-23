// Copyright 2012-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

/*
  A demonstration of using multiple top-level windows.
*/

#include "cube_view.h"
#include "demo_utils.h"
#include "test/test_utils.h"

#include "pugl/gl.h"
#include "pugl/pugl.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct {
  PuglView* view;
  double    xAngle;
  double    yAngle;
  double    lastMouseX;
  double    lastMouseY;
  double    lastDrawTime;
  float     dist;
  bool      entered;
} CubeView;

typedef struct {
  PuglWorld* world;
  CubeView   cubes[2];
  int        quit;
  bool       continuous;
  bool       verbose;
} PuglTestApp;

static const uint8_t pad = 64U;

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
    view, cube->dist, (float)cube->xAngle, (float)cube->yAngle, cube->entered);

  cube->lastDrawTime = thisTime;
}

static void
onKeyPress(PuglView* view, const PuglKeyEvent* event)
{
  PuglWorld*   world = puglGetWorld(view);
  PuglTestApp* app   = (PuglTestApp*)puglGetWorldHandle(world);
  PuglRect     frame = puglGetFrame(view);

  if (event->key == 'q' || event->key == PUGL_KEY_ESCAPE) {
    app->quit = 1;
  } else if (event->state & PUGL_MOD_SHIFT) {
    if (event->key == PUGL_KEY_UP) {
      puglSetSize(view, frame.width, frame.height - 10U);
    } else if (event->key == PUGL_KEY_DOWN) {
      puglSetSize(view, frame.width, frame.height + 10U);
    } else if (event->key == PUGL_KEY_LEFT) {
      puglSetSize(view, frame.width - 10U, frame.height);
    } else if (event->key == PUGL_KEY_RIGHT) {
      puglSetSize(view, frame.width + 10U, frame.height);
    }
  } else {
    if (event->key == PUGL_KEY_UP) {
      puglSetPosition(view, frame.x, frame.y - 10);
    } else if (event->key == PUGL_KEY_DOWN) {
      puglSetPosition(view, frame.x, frame.y + 10);
    } else if (event->key == PUGL_KEY_LEFT) {
      puglSetPosition(view, frame.x - 10, frame.y);
    } else if (event->key == PUGL_KEY_RIGHT) {
      puglSetPosition(view, frame.x + 10, frame.y);
    }
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

  const char* const prefix = cube == &app->cubes[0] ? "View 1: " : "View 2: ";
  printEvent(event, prefix, app->verbose);

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
    cube->xAngle -= event->motion.x - cube->lastMouseX;
    cube->yAngle += event->motion.y - cube->lastMouseY;
    cube->lastMouseX = event->motion.x;
    cube->lastMouseY = event->motion.y;
    redisplayView(app, view);
    break;
  case PUGL_SCROLL:
    cube->dist = fmaxf(10.0f, cube->dist + (float)event->scroll.dy);
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
  default:
    break;
  }

  return PUGL_SUCCESS;
}

int
main(int argc, char** argv)
{
  PuglTestApp app = {0};

  const PuglTestOptions opts = puglParseTestOptions(&argc, &argv);
  if (opts.help) {
    puglPrintTestUsage(argv[0], "");
    return 1;
  }

  app.continuous = opts.continuous;
  app.verbose    = opts.verbose;

  app.world         = puglNewWorld(PUGL_PROGRAM, 0);
  app.cubes[0].view = puglNewView(app.world);
  app.cubes[1].view = puglNewView(app.world);

  puglSetWorldHandle(app.world, &app);
  puglSetWorldString(app.world, PUGL_CLASS_NAME, "PuglWindowDemo");

  PuglStatus st = PUGL_SUCCESS;
  for (unsigned i = 0; i < 2; ++i) {
    CubeView* cube = &app.cubes[i];
    PuglView* view = cube->view;

    cube->dist = 10;

    puglSetViewString(view, PUGL_WINDOW_TITLE, "Pugl Window Demo");
    puglSetPosition(view,
                    (PuglCoord)(pad + (128U + pad) * i),
                    (PuglCoord)(pad + (128U + pad) * i));

    puglSetSizeHint(view, PUGL_DEFAULT_SIZE, 512, 512);
    puglSetSizeHint(view, PUGL_MIN_SIZE, 128, 128);
    puglSetSizeHint(view, PUGL_MAX_SIZE, 2048, 2048);
    puglSetBackend(view, puglGlBackend());

    puglSetViewHint(view, PUGL_CONTEXT_DEBUG, opts.errorChecking);
    puglSetViewHint(view, PUGL_RESIZABLE, opts.resizable);
    puglSetViewHint(view, PUGL_SAMPLES, opts.samples);
    puglSetViewHint(view, PUGL_DOUBLE_BUFFER, opts.doubleBuffer);
    puglSetViewHint(view, PUGL_SWAP_INTERVAL, opts.sync);
    puglSetViewHint(view, PUGL_IGNORE_KEY_REPEAT, opts.ignoreKeyRepeat);
    puglSetHandle(view, cube);
    puglSetEventFunc(view, onEvent);

    if (i == 1) {
      puglSetTransientParent(app.cubes[1].view,
                             puglGetNativeView(app.cubes[0].view));
    }

    if ((st = puglRealize(view))) {
      return logError("Failed to create window (%s)\n", puglStrerror(st));
    }

    puglShow(view, PUGL_SHOW_RAISE);
  }

  PuglFpsPrinter fpsPrinter  = {puglGetTime(app.world)};
  unsigned       framesDrawn = 0;
  while (!app.quit) {
    puglUpdate(app.world, app.continuous ? 0.0 : -1.0);
    ++framesDrawn;

    if (app.continuous) {
      puglPrintFps(app.world, &fpsPrinter, &framesDrawn);
    }
  }

  for (size_t i = 0; i < 2; ++i) {
    puglFreeView(app.cubes[i].view);
  }

  puglFreeWorld(app.world);

  return 0;
}
