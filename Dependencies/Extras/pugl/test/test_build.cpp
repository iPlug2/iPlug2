// Copyright 2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

/*
  Tests that C++ headers compile without any warnings.
*/

#define PUGL_DISABLE_DEPRECATED

#include "pugl/cairo.hpp" // IWYU pragma: keep
#include "pugl/gl.hpp"    // IWYU pragma: keep
#include "pugl/pugl.h"    // IWYU pragma: keep
#include "pugl/pugl.hpp"  // IWYU pragma: keep
#include "pugl/stub.hpp"  // IWYU pragma: keep

int
main()
{
  return 0;
}
