// Copyright 2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

/*
  Tests that C headers compile without any warnings.
*/

#define PUGL_DISABLE_DEPRECATED

#include "pugl/cairo.h" // IWYU pragma: keep
#include "pugl/gl.h"    // IWYU pragma: keep
#include "pugl/glu.h"   // IWYU pragma: keep
#include "pugl/pugl.h"  // IWYU pragma: keep
#include "pugl/stub.h"  // IWYU pragma: keep

int
main(void)
{
  return 0;
}
