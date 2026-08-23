// Copyright 2012-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_SRC_TYPES_H
#define PUGL_SRC_TYPES_H

#include "attributes.h"

#include "pugl/pugl.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// Platform-specific world internals
typedef struct PuglWorldInternalsImpl PuglWorldInternals;

/// Platform-specific view internals
typedef struct PuglInternalsImpl PuglInternals;

/// View hints
typedef int PuglHints[PUGL_NUM_VIEW_HINTS];

/// View position (both X and Y coordinates) or general point
typedef struct {
  PuglCoord x;
  PuglCoord y;
} PuglPoint;

/// View size (both X and Y coordinates)
typedef struct {
  PuglSpan width;
  PuglSpan height;
} PuglViewSize;

/// Blob of arbitrary data
typedef struct {
  void*  data; ///< Dynamically allocated data
  size_t len;  ///< Length of data in bytes
} PuglBlob;

/// Stage of a view along its lifespan
typedef enum {
  PUGL_VIEW_STAGE_ALLOCATED,
  PUGL_VIEW_STAGE_REALIZED,
  PUGL_VIEW_STAGE_CONFIGURED,
} PuglViewStage;

/// Cross-platform view definition
struct PuglViewImpl {
  PuglWorld*         world;
  const PuglBackend* backend;
  PuglInternals*     impl;
  PuglHandle         handle;
  PuglEventFunc      eventFunc;
  PuglNativeView     parent;
  uintptr_t          transientParent;
  PuglConfigureEvent lastConfigure;
  PuglHints          hints;
  PuglViewSize       sizeHints[PUGL_NUM_SIZE_HINTS];
  char*              strings[PUGL_NUM_STRING_HINTS];
  int                defaultX;
  int                defaultY;
  PuglViewStage      stage;
  bool               resizing;
};

/// Cross-platform world definition
struct PuglWorldImpl {
  PuglWorldInternals* impl;
  PuglWorldHandle     handle;
  double              startTime;
  size_t              numViews;
  PuglView**          views;
  char*               strings[PUGL_NUM_STRING_HINTS];
  PuglWorldType       type;
};

/// Opaque surface used by graphics backend
typedef void PuglSurface;

/// Graphics backend interface
struct PuglBackendImpl {
  /// Get visual information from display and setup view as necessary
  PUGL_WARN_UNUSED_RESULT
  PuglStatus (*configure)(PuglView*);

  /// Create surface and drawing context
  PUGL_WARN_UNUSED_RESULT
  PuglStatus (*create)(PuglView*);

  /// Destroy surface and drawing context
  void (*destroy)(PuglView*);

  /// Enter drawing context, for drawing if expose is non-null
  PUGL_WARN_UNUSED_RESULT
  PuglStatus (*enter)(PuglView*, const PuglExposeEvent*);

  /// Leave drawing context, after drawing if expose is non-null
  PUGL_WARN_UNUSED_RESULT
  PuglStatus (*leave)(PuglView*, const PuglExposeEvent*);

  /// Return the puglGetContext() handle for the application, if any
  void* (*getContext)(PuglView*);
};

#endif // PUGL_SRC_TYPES_H
