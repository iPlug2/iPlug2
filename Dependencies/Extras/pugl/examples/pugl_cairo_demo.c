// Copyright 2012-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "demo_utils.h"
#include "test/test_utils.h"

#include "pugl/cairo.h"
#include "pugl/pugl.h"

#include <cairo.h>

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  PuglWorld*      world;
  PuglTestOptions opts;
  double          currentMouseX;
  double          currentMouseY;
  double          lastDrawnMouseX;
  double          lastDrawnMouseY;
  unsigned        framesDrawn;
  int             quit;
  bool            entered;
  bool            mouseDown;
} PuglTestApp;

typedef struct {
  double x;
  double y;
} ViewScale;

typedef struct {
  int         x;
  int         y;
  int         w;
  int         h;
  const char* label;
} Button;

static const Button buttons[] = {{128, 128, 64, 64, "1"},
                                 {384, 128, 64, 64, "2"},
                                 {128, 384, 64, 64, "3"},
                                 {384, 384, 64, 64, "4"},
                                 {0, 0, 0, 0, NULL}};

static ViewScale
getScale(const PuglView* const view)
{
  const PuglRect  frame = puglGetFrame(view);
  const ViewScale scale = {(frame.width - (512.0 / frame.width)) / 512.0,
                           (frame.height - (512.0 / frame.height)) / 512.0};
  return scale;
}

static void
roundedBox(cairo_t* cr, double x, double y, double w, double h)
{
  static const double radius  = 10;
  static const double degrees = 3.14159265 / 180.0;

  cairo_new_sub_path(cr);
  cairo_arc(cr, x + w - radius, y + radius, radius, -90 * degrees, 0 * degrees);

  cairo_arc(
    cr, x + w - radius, y + h - radius, radius, 0 * degrees, 90 * degrees);

  cairo_arc(
    cr, x + radius, y + h - radius, radius, 90 * degrees, 180 * degrees);

  cairo_arc(cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
  cairo_close_path(cr);
}

static void
buttonDraw(PuglTestApp* app, cairo_t* cr, const Button* but, const double time)
{
  cairo_save(cr);
  cairo_translate(cr, but->x, but->y);
  cairo_rotate(cr, sin(time) * 3.141592);

  // Draw base
  if (app->mouseDown) {
    cairo_set_source_rgba(cr, 0.4, 0.9, 0.1, 1);
  } else {
    cairo_set_source_rgba(cr, 0.3, 0.5, 0.1, 1);
  }
  roundedBox(cr, 0, 0, but->w, but->h);
  cairo_fill_preserve(cr);

  // Draw border
  cairo_set_source_rgba(cr, 0.4, 0.9, 0.1, 1);
  cairo_set_line_width(cr, 4.0);
  cairo_stroke(cr);

  // Draw label
  cairo_text_extents_t extents;
  cairo_set_font_size(cr, 32.0);
  cairo_text_extents(cr, but->label, &extents);
  cairo_move_to(cr,
                (but->w / 2.0) - extents.width / 2,
                (but->h / 2.0) + extents.height / 2);
  cairo_set_source_rgba(cr, 0, 0, 0, 1);
  cairo_show_text(cr, but->label);

  cairo_restore(cr);
}

static void
postButtonRedisplay(PuglView* view)
{
  const ViewScale scale = getScale(view);

  for (const Button* b = buttons; b->label; ++b) {
    const double   span = sqrt(b->w * b->w + b->h * b->h);
    const PuglRect rect = {(PuglCoord)((b->x - span) * scale.x),
                           (PuglCoord)((b->y - span) * scale.y),
                           (PuglSpan)ceil(span * 2.0 * scale.x),
                           (PuglSpan)ceil(span * 2.0 * scale.y)};

    puglPostRedisplayRect(view, rect);
  }
}

static void
onDisplay(PuglTestApp* app, PuglView* view, const PuglExposeEvent* event)
{
  cairo_t* cr = (cairo_t*)puglGetContext(view);

  cairo_rectangle(cr, event->x, event->y, event->width, event->height);
  cairo_clip_preserve(cr);

  // Draw background
  if (app->entered) {
    cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
  } else {
    cairo_set_source_rgb(cr, 0, 0, 0);
  }
  cairo_fill(cr);

  // Scale to view size
  const ViewScale scale = getScale(view);
  cairo_scale(cr, scale.x, scale.y);

  // Draw button
  for (const Button* b = buttons; b->label; ++b) {
    buttonDraw(
      app, cr, b, app->opts.continuous ? puglGetTime(app->world) : 0.0);
  }

  // Draw mouse cursor
  const double mouseX = app->currentMouseX / scale.x;
  const double mouseY = app->currentMouseY / scale.y;
  cairo_set_line_width(cr, 2.0);
  cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
  cairo_move_to(cr, mouseX - 8.0, mouseY);
  cairo_line_to(cr, mouseX + 8.0, mouseY);
  cairo_move_to(cr, mouseX, mouseY - 8.0);
  cairo_line_to(cr, mouseX, mouseY + 8.0);
  cairo_stroke(cr);

  ++app->framesDrawn;
}

static void
onClose(PuglView* view)
{
  PuglTestApp* app = (PuglTestApp*)puglGetHandle(view);

  app->quit = 1;
}

static PuglRect
mouseCursorViewBounds(const PuglView* const view,
                      const double          mouseX,
                      const double          mouseY)
{
  const ViewScale scale = getScale(view);
  const PuglRect  rect  = {(PuglCoord)floor(mouseX - (10.0 * scale.x)),
                           (PuglCoord)floor(mouseY - (10.0 * scale.y)),
                           (PuglSpan)ceil(20.0 * scale.x),
                           (PuglSpan)ceil(20.0 * scale.y)};

  return rect;
}

static PuglStatus
onEvent(PuglView* view, const PuglEvent* event)
{
  PuglTestApp* app = (PuglTestApp*)puglGetHandle(view);

  printEvent(event, "Event: ", app->opts.verbose);

  switch (event->type) {
  case PUGL_KEY_PRESS:
    if (event->key.key == 'q' || event->key.key == PUGL_KEY_ESCAPE) {
      app->quit = 1;
    }
    break;
  case PUGL_BUTTON_PRESS:
    app->mouseDown = true;
    postButtonRedisplay(view);
    break;
  case PUGL_BUTTON_RELEASE:
    app->mouseDown = false;
    postButtonRedisplay(view);
    break;
  case PUGL_MOTION:
    // Redisplay to clear the old cursor position
    puglPostRedisplayRect(
      view,
      mouseCursorViewBounds(view, app->lastDrawnMouseX, app->lastDrawnMouseY));

    // Redisplay to show the new cursor position
    app->currentMouseX = event->motion.x;
    app->currentMouseY = event->motion.y;
    puglPostRedisplayRect(
      view,
      mouseCursorViewBounds(view, app->currentMouseX, app->currentMouseY));

    app->lastDrawnMouseX = app->currentMouseX;
    app->lastDrawnMouseY = app->currentMouseY;
    break;
  case PUGL_POINTER_IN:
    app->entered = true;
    puglPostRedisplay(view);
    break;
  case PUGL_POINTER_OUT:
    app->entered = false;
    puglPostRedisplay(view);
    break;
  case PUGL_UPDATE:
    if (app->opts.continuous) {
      puglPostRedisplay(view);
    }
    break;
  case PUGL_EXPOSE:
    onDisplay(app, view, &event->expose);
    break;
  case PUGL_CLOSE:
    onClose(view);
    break;
  default:
    break;
  }

  return PUGL_SUCCESS;
}

int
main(int argc, char** argv)
{
  PuglTestApp app;
  memset(&app, 0, sizeof(app));

  app.opts = puglParseTestOptions(&argc, &argv);
  if (app.opts.help) {
    puglPrintTestUsage("pugl_test", "");
    return 1;
  }

  app.world = puglNewWorld(PUGL_PROGRAM, 0);
  puglSetWorldString(app.world, PUGL_CLASS_NAME, "PuglCairoDemo");

  PuglView* view = puglNewView(app.world);

  puglSetViewString(view, PUGL_WINDOW_TITLE, "Pugl Cairo Demo");
  puglSetSizeHint(view, PUGL_DEFAULT_SIZE, 512, 512);
  puglSetSizeHint(view, PUGL_MIN_SIZE, 256, 256);
  puglSetSizeHint(view, PUGL_MAX_SIZE, 2048, 2048);
  puglSetViewHint(view, PUGL_RESIZABLE, app.opts.resizable);
  puglSetHandle(view, &app);
  puglSetBackend(view, puglCairoBackend());
  puglSetViewHint(view, PUGL_IGNORE_KEY_REPEAT, app.opts.ignoreKeyRepeat);
  puglSetEventFunc(view, onEvent);

  PuglStatus st = puglRealize(view);
  if (st) {
    return logError("Failed to create window (%s)\n", puglStrerror(st));
  }

  puglShow(view, PUGL_SHOW_RAISE);

  PuglFpsPrinter fpsPrinter = {puglGetTime(app.world)};
  const double   timeout    = app.opts.continuous ? (1 / 60.0) : -1.0;
  while (!app.quit) {
    puglUpdate(app.world, timeout);

    if (app.opts.continuous) {
      puglPrintFps(app.world, &fpsPrinter, &app.framesDrawn);
    }
  }

  puglFreeView(view);
  puglFreeWorld(app.world);
  return 0;
}
