// Copyright 2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

/*
  Tests that all hints are set to real values after a view is realized.
*/

#undef NDEBUG

#include "test_utils.h"

#include "pugl/gl.h"
#include "pugl/pugl.h"

#include <assert.h>

static PuglStatus
onEvent(PuglView* view, const PuglEvent* event)
{
  (void)view;
  (void)event;

  return PUGL_SUCCESS;
}

int
main(void)
{
  PuglWorld* const world = puglNewWorld(PUGL_PROGRAM, 0);
  PuglView* const  view  = puglNewView(world);

  // Set up view
  puglSetWorldString(world, PUGL_CLASS_NAME, "PuglTest");
  puglSetViewString(view, PUGL_WINDOW_TITLE, "Pugl OpenGL Hints Test");
  puglSetBackend(view, puglGlBackend());
  puglSetEventFunc(view, onEvent);
  puglSetSizeHint(view, PUGL_DEFAULT_SIZE, 256, 256);
  puglSetPosition(view, 128, 128);

  // Check invalid cases
  assert(puglSetViewHint(view, PUGL_CONTEXT_API, PUGL_DONT_CARE) ==
         PUGL_BAD_PARAMETER);
  assert(puglSetViewHint(view, PUGL_CONTEXT_VERSION_MAJOR, PUGL_DONT_CARE) ==
         PUGL_BAD_PARAMETER);
  assert(puglSetViewHint(view, PUGL_CONTEXT_VERSION_MINOR, PUGL_DONT_CARE) ==
         PUGL_BAD_PARAMETER);
  assert(puglSetViewHint(view, PUGL_CONTEXT_PROFILE, PUGL_DONT_CARE) ==
         PUGL_BAD_PARAMETER);
  assert(puglSetViewHint(view, PUGL_CONTEXT_DEBUG, PUGL_DONT_CARE) ==
         PUGL_BAD_PARAMETER);
  assert(puglSetViewHint(view, PUGL_SWAP_INTERVAL, PUGL_DONT_CARE) ==
         PUGL_BAD_PARAMETER);

  // Set all hints that support it to PUGL_DONT_CARE
  assert(!puglSetViewHint(view, PUGL_RED_BITS, PUGL_DONT_CARE));
  assert(!puglSetViewHint(view, PUGL_GREEN_BITS, PUGL_DONT_CARE));
  assert(!puglSetViewHint(view, PUGL_BLUE_BITS, PUGL_DONT_CARE));
  assert(!puglSetViewHint(view, PUGL_ALPHA_BITS, PUGL_DONT_CARE));
  assert(!puglSetViewHint(view, PUGL_DEPTH_BITS, PUGL_DONT_CARE));
  assert(!puglSetViewHint(view, PUGL_STENCIL_BITS, PUGL_DONT_CARE));
  assert(!puglSetViewHint(view, PUGL_SAMPLE_BUFFERS, PUGL_DONT_CARE));
  assert(!puglSetViewHint(view, PUGL_SAMPLES, PUGL_DONT_CARE));
  assert(!puglSetViewHint(view, PUGL_DOUBLE_BUFFER, PUGL_DONT_CARE));
  assert(!puglSetViewHint(view, PUGL_REFRESH_RATE, PUGL_DONT_CARE));

  // Realize view and print all hints for debugging convenience
  assert(!puglRealize(view));
  printViewHints(view);

  // Check that no hints are set to PUGL_DONT_CARE
  assert(puglGetViewHint(view, PUGL_CONTEXT_API) != PUGL_DONT_CARE);
  assert(puglGetViewHint(view, PUGL_CONTEXT_VERSION_MAJOR) != PUGL_DONT_CARE);
  assert(puglGetViewHint(view, PUGL_CONTEXT_VERSION_MINOR) != PUGL_DONT_CARE);
  assert(puglGetViewHint(view, PUGL_CONTEXT_PROFILE) != PUGL_DONT_CARE);
  assert(puglGetViewHint(view, PUGL_CONTEXT_DEBUG) != PUGL_DONT_CARE);
  assert(puglGetViewHint(view, PUGL_RED_BITS) != PUGL_DONT_CARE);
  assert(puglGetViewHint(view, PUGL_GREEN_BITS) != PUGL_DONT_CARE);
  assert(puglGetViewHint(view, PUGL_BLUE_BITS) != PUGL_DONT_CARE);
  assert(puglGetViewHint(view, PUGL_ALPHA_BITS) != PUGL_DONT_CARE);
  assert(puglGetViewHint(view, PUGL_DEPTH_BITS) != PUGL_DONT_CARE);
  assert(puglGetViewHint(view, PUGL_STENCIL_BITS) != PUGL_DONT_CARE);
  assert(puglGetViewHint(view, PUGL_SAMPLE_BUFFERS) != PUGL_DONT_CARE);
  assert(puglGetViewHint(view, PUGL_SAMPLES) != PUGL_DONT_CARE);
  assert(puglGetViewHint(view, PUGL_DOUBLE_BUFFER) != PUGL_DONT_CARE);
  assert(puglGetViewHint(view, PUGL_SWAP_INTERVAL) != PUGL_DONT_CARE);
  assert(puglGetViewHint(view, PUGL_RESIZABLE) != PUGL_DONT_CARE);
  assert(puglGetViewHint(view, PUGL_IGNORE_KEY_REPEAT) != PUGL_DONT_CARE);
  assert(puglGetViewHint(view, PUGL_REFRESH_RATE) != PUGL_DONT_CARE);

  // Tear down
  puglFreeView(view);
  puglFreeWorld(world);

  return 0;
}
