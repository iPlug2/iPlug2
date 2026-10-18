// Copyright 2012-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_SRC_WIN_H
#define PUGL_SRC_WIN_H

#include "internal.h"

#include "pugl/pugl.h"

#include <windows.h>

#include <stdbool.h>

typedef PIXELFORMATDESCRIPTOR PuglWinPFD;

struct PuglWorldInternalsImpl {
  double timerFrequency;
};

struct PuglInternalsImpl {
  PuglWinPFD      pfd;
  int             pfId;
  HWND            hwnd;
  HCURSOR         cursor;
  HDC             hdc;
  WINDOWPLACEMENT oldPlacement;
  PAINTSTRUCT     paint;
  PuglBlob        clipboard;
  PuglSurface*    surface;
  double          scaleFactor;
  bool            mapped;
  bool            flashing;
  bool            mouseTracked;
  bool            minimized;
  bool            maximized;
  bool            fullscreen;
};

PUGL_API
PuglWinPFD
puglWinGetPixelFormatDescriptor(const PuglHints hints);

PUGL_WARN_UNUSED_RESULT
PUGL_API
PuglStatus
puglWinCreateWindow(PuglView* view, const char* title, HWND* hwnd, HDC* hdc);

PUGL_WARN_UNUSED_RESULT
PUGL_API
PuglStatus
puglWinConfigure(PuglView* view);

PUGL_WARN_UNUSED_RESULT
PUGL_API
PuglStatus
puglWinEnter(PuglView* view, const PuglExposeEvent* expose);

PUGL_WARN_UNUSED_RESULT
PUGL_API
PuglStatus
puglWinLeave(PuglView* view, const PuglExposeEvent* expose);

#endif // PUGL_SRC_WIN_H
