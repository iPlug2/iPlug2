// Copyright 2012-2023 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

/*
  A demonstration of window types, states, and management.
*/

#include "test/test_utils.h"

#include "pugl/cairo.h"
#include "pugl/pugl.h"

#include <cairo.h>

#include <stdbool.h>
#include <stdio.h>

typedef struct {
  PuglView*   view;
  const char* label;
} LabeledView;

typedef struct {
  PuglWorld*  world;
  LabeledView mainView;
  LabeledView dialogView;
  bool        quit;
  bool        verbose;
} DemoApp;

static PuglStatus
onCommonEvent(PuglView* view, const PuglEvent* event);

static PuglStatus
onMainEvent(PuglView* view, const PuglEvent* event);

static PuglStatus
onExpose(PuglView* const view, const PuglExposeEvent* const event)
{
  PuglWorld* const         world = puglGetWorld(view);
  DemoApp* const           app   = (DemoApp*)puglGetWorldHandle(world);
  const PuglRect           frame = puglGetFrame(view);
  const PuglViewStyleFlags style = puglGetViewStyle(view);
  const PuglCoord          cx    = (PuglCoord)(frame.width / 2U);
  const PuglCoord          cy    = (PuglCoord)(frame.height / 2U);
  cairo_t* const           cr    = (cairo_t*)puglGetContext(view);

  // Clip to expose region
  cairo_rectangle(cr, event->x, event->y, event->width, event->height);
  cairo_clip_preserve(cr);

  // Draw background
  cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
  cairo_set_line_width(cr, 4.0);
  cairo_fill(cr);

  // Set up text renering
  char                 buf[128] = {0};
  cairo_text_extents_t extents  = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  cairo_set_font_size(cr, 30.0);

  // Draw time label
  snprintf(buf, sizeof(buf), "Draw time: %g", puglGetTime(world));
  cairo_text_extents(cr, buf, &extents);
  cairo_move_to(cr, cx - extents.width / 2.0, cy + extents.height / 2.0 - 48.0);
  cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
  cairo_show_text(cr, buf);

  // Draw style label
  snprintf(buf,
           sizeof(buf),
           "Style:%s%s%s%s%s%s%s%s%s",
           style & PUGL_VIEW_STYLE_MODAL ? " modal" : "",
           style & PUGL_VIEW_STYLE_TALL ? " tall" : "",
           style & PUGL_VIEW_STYLE_WIDE ? " wide" : "",
           style & PUGL_VIEW_STYLE_HIDDEN ? " hidden" : "",
           style & PUGL_VIEW_STYLE_FULLSCREEN ? " fullscreen" : "",
           style & PUGL_VIEW_STYLE_ABOVE ? " above" : "",
           style & PUGL_VIEW_STYLE_BELOW ? " below" : "",
           style & PUGL_VIEW_STYLE_DEMANDING ? " demanding" : "",
           style & PUGL_VIEW_STYLE_RESIZING ? " resizing" : "");
  cairo_text_extents(cr, buf, &extents);
  cairo_move_to(cr, cx - extents.width / 2.0, cy + extents.height / 2.0);
  cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
  cairo_show_text(cr, buf);

  if (view == app->mainView.view) {
    // Draw keyboard help label
    snprintf(buf, sizeof(buf), "Keys: Space T W H M F A B D Q");
    cairo_text_extents(cr, buf, &extents);
    cairo_move_to(
      cr, cx - extents.width / 2.0, cy + extents.height / 2.0 + 48.0);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_show_text(cr, buf);
  }

  return PUGL_SUCCESS;
}

static PuglStatus
toggleDialog(DemoApp* const app)
{
  if (app->dialogView.view && puglGetVisible(app->dialogView.view)) {
    return puglUnrealize(app->dialogView.view);
  }

  if (!app->dialogView.view) {
    app->dialogView.view = puglNewView(app->world);

    puglSetBackend(app->dialogView.view, puglCairoBackend());
    puglSetEventFunc(app->dialogView.view, onCommonEvent);
    puglSetHandle(app->dialogView.view, &app->dialogView);
    puglSetTransientParent(app->dialogView.view,
                           puglGetNativeView(app->mainView.view));

    puglSetViewString(app->dialogView.view, PUGL_WINDOW_TITLE, "Dialog");
    puglSetViewHint(app->dialogView.view, PUGL_DARK_FRAME, PUGL_TRUE);
    puglSetSizeHint(app->dialogView.view, PUGL_DEFAULT_SIZE, 320, 240);
    puglSetSizeHint(app->dialogView.view, PUGL_MIN_SIZE, 160, 120);
    puglSetViewHint(app->dialogView.view, PUGL_IGNORE_KEY_REPEAT, true);
    puglSetViewHint(app->dialogView.view, PUGL_RESIZABLE, true);
    puglSetViewHint(
      app->dialogView.view, PUGL_VIEW_TYPE, PUGL_VIEW_TYPE_DIALOG);
  }

  return puglShow(app->dialogView.view, PUGL_SHOW_RAISE);
}

static PuglStatus
onKeyPress(PuglView* view, const PuglKeyEvent* event)
{
  PuglWorld* const         world = puglGetWorld(view);
  DemoApp* const           app   = (DemoApp*)puglGetWorldHandle(world);
  const PuglViewStyleFlags flags = puglGetViewStyle(view);

  switch (event->key) {
  case ' ':
    toggleDialog(app);
    break;
  case 't':
    return puglSetViewStyle(view, flags ^ PUGL_VIEW_STYLE_TALL);
  case 'w':
    return puglSetViewStyle(view, flags ^ PUGL_VIEW_STYLE_WIDE);
  case 'h':
    return puglSetViewStyle(view, flags ^ PUGL_VIEW_STYLE_HIDDEN);
  case 'm':
    if ((flags & PUGL_VIEW_STYLE_TALL) && (flags & PUGL_VIEW_STYLE_WIDE)) {
      return puglSetViewStyle(
        view, flags & ~(unsigned)(PUGL_VIEW_STYLE_TALL | PUGL_VIEW_STYLE_WIDE));
    }

    return puglSetViewStyle(
      view, flags | PUGL_VIEW_STYLE_TALL | PUGL_VIEW_STYLE_WIDE);
  case 'f':
    return puglSetViewStyle(view, flags ^ PUGL_VIEW_STYLE_FULLSCREEN);
  case 'a':
    return puglSetViewStyle(view, flags ^ PUGL_VIEW_STYLE_ABOVE);
  case 'b':
    return puglSetViewStyle(view, flags ^ PUGL_VIEW_STYLE_BELOW);
  case 'd':
    return puglSetViewStyle(view, flags ^ PUGL_VIEW_STYLE_DEMANDING);
  case 'q':
  case PUGL_KEY_ESCAPE:
    app->quit = true;
    break;
  }

  return PUGL_SUCCESS;
}

static PuglStatus
onCommonEvent(PuglView* view, const PuglEvent* const event)
{
  PuglWorld* const   world = puglGetWorld(view);
  DemoApp* const     app   = (DemoApp*)puglGetWorldHandle(world);
  LabeledView* const data  = (LabeledView*)puglGetHandle(view);

  const char* const prefix = data->label;
  printEvent(event, prefix, app->verbose);

  switch (event->type) {
  case PUGL_CLOSE:
    if (view == app->dialogView.view) {
      puglUnrealize(app->dialogView.view);
    }
    break;
  case PUGL_CONFIGURE:
    return puglPostRedisplay(view);
  case PUGL_EXPOSE:
    return onExpose(view, &event->expose);
  case PUGL_KEY_PRESS:
    return onKeyPress(view, &event->key);
  default:
    break;
  }

  return PUGL_SUCCESS;
}

static PuglStatus
onMainEvent(PuglView* view, const PuglEvent* const event)
{
  PuglWorld* const world = puglGetWorld(view);
  DemoApp* const   app   = (DemoApp*)puglGetWorldHandle(world);

  switch (event->type) {
  case PUGL_CLOSE:
    app->quit = true;
    return PUGL_SUCCESS;
  default:
    break;
  }

  return onCommonEvent(view, event);
}

int
main(int argc, char** argv)
{
  DemoApp app = {0};

  const PuglTestOptions opts = puglParseTestOptions(&argc, &argv);
  if (opts.help) {
    puglPrintTestUsage(argv[0], "");
    return 1;
  }

  app.verbose = opts.verbose;

  app.world = puglNewWorld(PUGL_PROGRAM, 0);

  puglSetWorldHandle(app.world, &app);
  puglSetWorldString(app.world, PUGL_CLASS_NAME, "PuglDemoApp");

  app.mainView.view    = puglNewView(app.world);
  app.mainView.label   = "Main: ";
  app.dialogView.label = "Dialog: ";

  // Set up main view
  puglSetBackend(app.mainView.view, puglCairoBackend());
  puglSetEventFunc(app.mainView.view, onMainEvent);
  puglSetHandle(app.mainView.view, &app.mainView);
  puglSetSizeHint(app.mainView.view, PUGL_DEFAULT_SIZE, 640, 480);
  puglSetSizeHint(app.mainView.view, PUGL_MIN_SIZE, 320, 240);
  puglSetViewHint(app.mainView.view, PUGL_DARK_FRAME, PUGL_TRUE);
  puglSetViewHint(app.mainView.view, PUGL_IGNORE_KEY_REPEAT, true);
  puglSetViewHint(app.mainView.view, PUGL_RESIZABLE, true);
  puglSetViewHint(app.mainView.view, PUGL_VIEW_TYPE, PUGL_VIEW_TYPE_NORMAL);
  puglSetViewString(app.mainView.view, PUGL_WINDOW_TITLE, "Main Window");

  PuglStatus st = PUGL_SUCCESS;
  if ((st = puglRealize(app.mainView.view))) {
    return logError("Failed to realize view (%s)\n", puglStrerror(st));
  }

  puglShow(app.mainView.view, PUGL_SHOW_RAISE);

  while (!app.quit) {
    puglUpdate(app.world, -1.0);
  }

  puglFreeView(app.mainView.view);
  puglFreeWorld(app.world);
  return 0;
}
