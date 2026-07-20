// Copyright 2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

// Tests that the implementation compiles as included ObjC++

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#  pragma clang diagnostic ignored "-Wold-style-cast"
#endif

#include "../src/common.c"   // IWYU pragma: keep
#include "../src/internal.c" // IWYU pragma: keep
#include "../src/mac.h"      // IWYU pragma: keep
#include "../src/mac.m"      // IWYU pragma: keep
#include "../src/mac_stub.m" // IWYU pragma: keep

#if defined(WITH_CAIRO)
#  include "../src/mac_cairo.m" // IWYU pragma: keep
#endif

#if defined(WITH_OPENGL)
#  include "../src/mac_gl.m" // IWYU pragma: keep
#endif

#if defined(WITH_VULKAN)
#  include "../src/mac_vulkan.m" // IWYU pragma: keep
#endif

#if defined(__clang__)
#  pragma clang diagnostic pop
#endif

int
main(void)
{
  return 0;
}
