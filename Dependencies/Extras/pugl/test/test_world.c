// Copyright 2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

// Tests basic functionality of, and access to, the world.

#undef NDEBUG

#include "pugl/pugl.h"

#include <assert.h>
#include <stdint.h>

int
main(void)
{
  PuglWorld* const world = puglNewWorld(PUGL_PROGRAM, 0);
  PuglView* const  view  = puglNewView(world);

  // Check that the world can be accessed from the view
  assert(puglGetWorld(view) == world);

  // Check that puglGetNativeWorld() returns something
  assert(puglGetNativeWorld(world));

  // Set and get world handle
  uintptr_t data = 1234;
  puglSetWorldHandle(world, &data);
  assert(puglGetWorldHandle(world) == &data);

  // Tear down
  puglFreeView(view);
  puglFreeWorld(world);

  return 0;
}
