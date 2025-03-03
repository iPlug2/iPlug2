// Copyright 2012-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "test/test_utils.h"

#include "pugl/gl.h"
#include "pugl/pugl.h"

#include <stdbool.h>

static const int N_CURSORS = 10;
static const int N_ROWS    = 2;
static const int N_COLS    = 5;

typedef struct {
  PuglWorld*      world;
  PuglTestOptions opts;
  bool            quit;
} PuglTestApp;

static void
onConfigure(const double width, const double height)
{
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glViewport(0, 0, (int)width, (int)height);
}

static void
onExpose(void)
{
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glColor3f(0.6f, 0.6f, 0.6f);

  for (int row = 1; row < N_ROWS; ++row) {
    const float y = (float)row * (2.0f / (float)N_ROWS) - 1.0f;
    glBegin(GL_LINES);
    glVertex2f(-1.0f, y);
    glVertex2f(1.0f, y);
    glEnd();
  }

  for (int col = 1; col < N_COLS; ++col) {
    const float x = (float)col * (2.0f / (float)N_COLS) - 1.0f;
    glBegin(GL_LINES);
    glVertex2f(x, -1.0f);
    glVertex2f(x, 1.0f);
    glEnd();
  }
}

static void
onMotion(PuglView* view, double x, double y)
{
  const PuglRect frame = puglGetFrame(view);
  int            row   = (int)(y * N_ROWS / frame.height);
  int            col   = (int)(x * N_COLS / frame.width);

  row = (row < 0) ? 0 : (row >= N_ROWS) ? (N_ROWS - 1) : row;
  col = (col < 0) ? 0 : (col >= N_COLS) ? (N_COLS - 1) : col;

  const PuglCursor cursor = (PuglCursor)((row * N_COLS + col) % N_CURSORS);
  puglSetCursor(view, cursor);
}

static PuglStatus
onEvent(PuglView* view, const PuglEvent* event)
{
  PuglTestApp* app = (PuglTestApp*)puglGetHandle(view);

  printEvent(event, "Event: ", app->opts.verbose);

  switch (event->type) {
  case PUGL_CONFIGURE:
    onConfigure(event->configure.width, event->configure.height);
    break;
  case PUGL_KEY_PRESS:
    if (event->key.key == 'q' || event->key.key == PUGL_KEY_ESCAPE) {
      app->quit = 1;
    }
    break;
  case PUGL_MOTION:
    onMotion(view, event->motion.x, event->motion.y);
    break;
  case PUGL_EXPOSE:
    onExpose();
    break;
  case PUGL_POINTER_OUT:
    puglSetCursor(view, PUGL_CURSOR_ARROW);
    break;
  case PUGL_CLOSE:
    app->quit = 1;
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

  app.opts = puglParseTestOptions(&argc, &argv);
  if (app.opts.help) {
    puglPrintTestUsage(argv[0], "");
    return 1;
  }

  app.world = puglNewWorld(PUGL_PROGRAM, 0);

  puglSetWorldHandle(app.world, &app);
  puglSetWorldString(app.world, PUGL_CLASS_NAME, "PuglCursorDemo");

  PuglView* view = puglNewView(app.world);

  puglSetViewString(view, PUGL_WINDOW_TITLE, "Pugl Cursor Demo");
  puglSetSizeHint(view, PUGL_DEFAULT_SIZE, 512, 256);
  puglSetSizeHint(view, PUGL_MIN_SIZE, 128, 64);
  puglSetSizeHint(view, PUGL_MAX_SIZE, 512, 256);
  puglSetBackend(view, puglGlBackend());

  puglSetViewHint(view, PUGL_CONTEXT_DEBUG, app.opts.errorChecking);
  puglSetViewHint(view, PUGL_RESIZABLE, app.opts.resizable);
  puglSetViewHint(view, PUGL_SAMPLES, app.opts.samples);
  puglSetViewHint(view, PUGL_DOUBLE_BUFFER, app.opts.doubleBuffer);
  puglSetViewHint(view, PUGL_SWAP_INTERVAL, app.opts.sync);
  puglSetViewHint(view, PUGL_IGNORE_KEY_REPEAT, app.opts.ignoreKeyRepeat);
  puglSetHandle(view, &app);
  puglSetEventFunc(view, onEvent);

  const PuglStatus st = puglRealize(view);
  if (st) {
    return logError("Failed to create window (%s)\n", puglStrerror(st));
  }

  puglShow(view, PUGL_SHOW_RAISE);

  while (!app.quit) {
    puglUpdate(app.world, -1.0);
  }

  puglFreeView(view);
  puglFreeWorld(app.world);

  return 0;
}
