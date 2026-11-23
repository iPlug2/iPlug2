// Copyright 2012-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "cube_view.h"
#include "demo_utils.h"
#include "test/test_utils.h"

#include "pugl/gl.h"
#include "pugl/pugl.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

static const uint8_t   borderWidth    = 64U;
static const uintptr_t reverseTimerId = 1U;

typedef struct {
  PuglWorld* world;
  PuglView*  parent;
  PuglView*  child;
  double     xAngle;
  double     yAngle;
  double     lastMouseX;
  double     lastMouseY;
  double     lastDrawTime;
  float      dist;
  int        quit;
  bool       continuous;
  bool       mouseEntered;
  bool       verbose;
  bool       reversing;
} PuglTestApp;

// clang-format off

static const float backgroundVertices[] = {
  -1.0f,  1.0f,  -1.0f, // Top left
   1.0f,  1.0f,  -1.0f, // Top right
  -1.0f, -1.0f,  -1.0f, // Bottom left
   1.0f, -1.0f,  -1.0f, // Bottom right
};

static const float backgroundColorVertices[] = {
  0.25f, 0.25f, 0.25f, // Top left
  0.25f, 0.50f, 0.25f, // Top right
  0.25f, 0.50f, 0.25f, // Bottom left
  0.25f, 0.75f, 0.5f,  // Bottom right
};

// clang-format on

static PuglRect
getChildFrame(const PuglRect parentFrame)
{
  const PuglRect childFrame = {
    borderWidth,
    borderWidth,
    (PuglSpan)(parentFrame.width - 2 * borderWidth),
    (PuglSpan)(parentFrame.height - 2 * borderWidth)};

  return childFrame;
}

static void
onDisplay(PuglView* view)
{
  PuglTestApp* app = (PuglTestApp*)puglGetHandle(view);

  const double thisTime = puglGetTime(app->world);
  if (app->continuous) {
    const double dTime =
      (thisTime - app->lastDrawTime) * (app->reversing ? -1.0 : 1.0);

    app->xAngle = fmod(app->xAngle + dTime * 100.0, 360.0);
    app->yAngle = fmod(app->yAngle + dTime * 100.0, 360.0);
  }

  displayCube(
    view, app->dist, (float)app->xAngle, (float)app->yAngle, app->mouseEntered);

  app->lastDrawTime = thisTime;
}

static void
swapFocus(PuglTestApp* app)
{
  if (puglHasFocus(app->parent)) {
    puglGrabFocus(app->child);
  } else {
    puglGrabFocus(app->parent);
  }

  if (!app->continuous) {
    puglPostRedisplay(app->parent);
    puglPostRedisplay(app->child);
  }
}

static void
onKeyPress(PuglView* view, const PuglKeyEvent* event)
{
  PuglTestApp* app   = (PuglTestApp*)puglGetHandle(view);
  PuglRect     frame = puglGetFrame(view);

  if (event->key == '\t') {
    swapFocus(app);
  } else if (event->key == 'q' || event->key == PUGL_KEY_ESCAPE) {
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

static PuglStatus
onParentEvent(PuglView* view, const PuglEvent* event)
{
  PuglTestApp*   app         = (PuglTestApp*)puglGetHandle(view);
  const PuglRect parentFrame = puglGetFrame(view);

  printEvent(event, "Parent: ", app->verbose);

  switch (event->type) {
  case PUGL_CONFIGURE:
    reshapeCube((float)event->configure.width, (float)event->configure.height);

    puglSetFrame(app->child, getChildFrame(parentFrame));
    break;
  case PUGL_UPDATE:
    if (app->continuous) {
      puglPostRedisplay(view);
    }
    break;
  case PUGL_EXPOSE:
    if (puglHasFocus(app->parent)) {
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glEnableClientState(GL_VERTEX_ARRAY);
      glEnableClientState(GL_COLOR_ARRAY);
      glVertexPointer(3, GL_FLOAT, 0, backgroundVertices);
      glColorPointer(3, GL_FLOAT, 0, backgroundColorVertices);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      glDisableClientState(GL_COLOR_ARRAY);
      glDisableClientState(GL_VERTEX_ARRAY);
    } else {
      glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    break;
  case PUGL_KEY_PRESS:
    onKeyPress(view, &event->key);
    break;
  case PUGL_MOTION:
    break;
  case PUGL_CLOSE:
    app->quit = 1;
    break;
  default:
    break;
  }

  return PUGL_SUCCESS;
}

static PuglStatus
onEvent(PuglView* view, const PuglEvent* event)
{
  PuglTestApp* app = (PuglTestApp*)puglGetHandle(view);

  printEvent(event, "Child:  ", app->verbose);

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
    app->xAngle -= event->motion.x - app->lastMouseX;
    app->yAngle += event->motion.y - app->lastMouseY;
    app->lastMouseX = event->motion.x;
    app->lastMouseY = event->motion.y;
    if (!app->continuous) {
      puglPostRedisplay(view);
      puglPostRedisplay(app->parent);
    }
    break;
  case PUGL_SCROLL:
    app->dist = fmaxf(10.0f, app->dist + (float)event->scroll.dy);
    if (!app->continuous) {
      puglPostRedisplay(view);
    }
    break;
  case PUGL_POINTER_IN:
    app->mouseEntered = true;
    break;
  case PUGL_POINTER_OUT:
    app->mouseEntered = false;
    break;
  case PUGL_TIMER:
    app->reversing = !app->reversing;
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

  app.dist = 10;

  const PuglTestOptions opts = puglParseTestOptions(&argc, &argv);
  if (opts.help) {
    puglPrintTestUsage("pugl_test", "");
    return 1;
  }

  app.continuous = opts.continuous;
  app.verbose    = opts.verbose;

  app.world  = puglNewWorld(PUGL_PROGRAM, 0);
  app.parent = puglNewView(app.world);
  app.child  = puglNewView(app.world);

  puglSetWorldString(app.world, PUGL_CLASS_NAME, "PuglEmbedDemo");

  const PuglRect parentFrame = {0, 0, 512, 512};
  puglSetSizeHint(app.parent, PUGL_DEFAULT_SIZE, 512, 512);
  puglSetSizeHint(app.parent, PUGL_MIN_SIZE, 192, 192);
  puglSetSizeHint(app.parent, PUGL_MAX_SIZE, 1024, 1024);
  puglSetSizeHint(app.parent, PUGL_MIN_ASPECT, 1, 1);
  puglSetSizeHint(app.parent, PUGL_MAX_ASPECT, 16, 9);
  puglSetBackend(app.parent, puglGlBackend());

  puglSetViewHint(app.parent, PUGL_CONTEXT_DEBUG, opts.errorChecking);
  puglSetViewHint(app.parent, PUGL_RESIZABLE, opts.resizable);
  puglSetViewHint(app.parent, PUGL_SAMPLES, opts.samples);
  puglSetViewHint(app.parent, PUGL_DOUBLE_BUFFER, opts.doubleBuffer);
  puglSetViewHint(app.parent, PUGL_SWAP_INTERVAL, opts.sync);
  puglSetViewHint(app.parent, PUGL_IGNORE_KEY_REPEAT, opts.ignoreKeyRepeat);
  puglSetHandle(app.parent, &app);
  puglSetEventFunc(app.parent, onParentEvent);

  PuglStatus    st      = PUGL_SUCCESS;
  const uint8_t title[] = {
    'P', 'u', 'g', 'l', ' ', 'P', 'r', 0xC3, 0xBC, 'f', 'u', 'n', 'g', 0};

  puglSetViewString(app.parent, PUGL_WINDOW_TITLE, (const char*)title);

  if ((st = puglRealize(app.parent))) {
    return logError("Failed to create parent window (%s)\n", puglStrerror(st));
  }

  puglSetFrame(app.child, getChildFrame(parentFrame));
  puglSetParentWindow(app.child, puglGetNativeView(app.parent));

  puglSetViewHint(app.child, PUGL_CONTEXT_DEBUG, opts.errorChecking);
  puglSetViewHint(app.child, PUGL_SAMPLES, opts.samples);
  puglSetViewHint(app.child, PUGL_DOUBLE_BUFFER, opts.doubleBuffer);
  puglSetViewHint(app.child, PUGL_SWAP_INTERVAL, opts.sync);
  puglSetBackend(app.child, puglGlBackend());
  puglSetViewHint(app.child, PUGL_IGNORE_KEY_REPEAT, opts.ignoreKeyRepeat);
  puglSetHandle(app.child, &app);
  puglSetEventFunc(app.child, onEvent);

  if ((st = puglRealize(app.child))) {
    return logError("Failed to create child window (%s)\n", puglStrerror(st));
  }

  puglShow(app.parent, PUGL_SHOW_RAISE);
  puglShow(app.child, PUGL_SHOW_RAISE);

  puglStartTimer(app.child, reverseTimerId, 3.6);

  PuglFpsPrinter fpsPrinter         = {puglGetTime(app.world)};
  unsigned       framesDrawn        = 0;
  bool           requestedAttention = false;
  while (!app.quit) {
    const double thisTime = puglGetTime(app.world);

    puglUpdate(app.world, app.continuous ? 0.0 : -1.0);
    ++framesDrawn;

    if (!requestedAttention && thisTime > 5.0) {
      puglSetViewStyle(
        app.parent, puglGetViewStyle(app.parent) | PUGL_VIEW_STYLE_DEMANDING);
      requestedAttention = true;
    }

    if (app.continuous) {
      puglPrintFps(app.world, &fpsPrinter, &framesDrawn);
    }
  }

  puglFreeView(app.child);
  puglFreeView(app.parent);
  puglFreeWorld(app.world);

  return 0;
}
