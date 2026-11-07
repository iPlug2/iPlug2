// Copyright 2012-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_SRC_X11_H
#define PUGL_SRC_X11_H

#include "attributes.h"
#include "types.h"

#include "pugl/attributes.h"
#include "pugl/pugl.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  Atom CLIPBOARD;
  Atom UTF8_STRING;
  Atom WM_CLIENT_MACHINE;
  Atom WM_PROTOCOLS;
  Atom WM_DELETE_WINDOW;
  Atom PUGL_CLIENT_MSG;
  Atom NET_CLOSE_WINDOW;
  Atom NET_FRAME_EXTENTS;
  Atom NET_WM_NAME;
  Atom NET_WM_PID;
  Atom NET_WM_PING;
  Atom NET_WM_STATE;
  Atom NET_WM_STATE_ABOVE;
  Atom NET_WM_STATE_BELOW;
  Atom NET_WM_STATE_DEMANDS_ATTENTION;
  Atom NET_WM_STATE_FULLSCREEN;
  Atom NET_WM_STATE_HIDDEN;
  Atom NET_WM_STATE_MAXIMIZED_HORZ;
  Atom NET_WM_STATE_MAXIMIZED_VERT;
  Atom NET_WM_STATE_MODAL;
  Atom NET_WM_WINDOW_TYPE;
  Atom NET_WM_WINDOW_TYPE_DIALOG;
  Atom NET_WM_WINDOW_TYPE_NORMAL;
  Atom NET_WM_WINDOW_TYPE_UTILITY;
  Atom TARGETS;
  Atom text_uri_list;
} PuglX11Atoms;

typedef struct {
  XID       alarm;
  PuglView* view;
  uintptr_t id;
} PuglTimer;

typedef struct {
  Atom          selection;
  Atom          property;
  Window        source;
  Atom*         formats;
  char**        formatStrings;
  unsigned long numFormats;
  uint32_t      acceptedFormatIndex;
  Atom          acceptedFormat;
  PuglBlob      data;
} PuglX11Clipboard;

struct PuglWorldInternalsImpl {
  Display*     display;
  PuglX11Atoms atoms;
  XIM          xim;
  double       scaleFactor;
  PuglTimer*   timers;
  size_t       numTimers;
  XID          serverTimeCounter;
  int          syncEventBase;
  bool         syncSupported;
  bool         dispatchingEvents;
};

struct PuglInternalsImpl {
  XVisualInfo*     vi;
  Window           win;
  XIC              xic;
  PuglSurface*     surface;
  PuglEvent        pendingConfigure;
  PuglEvent        pendingExpose;
  PuglX11Clipboard clipboard;
  long             frameExtentLeft;
  long             frameExtentTop;
  int              screen;
  const char*      cursorName;
  bool             mapped;
};

PUGL_WARN_UNUSED_RESULT
PUGL_API
PuglStatus
puglX11Configure(PuglView* view);

#endif // PUGL_SRC_X11_H
