// Copyright 2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

// Tests puglStrerror

#undef NDEBUG

#include "pugl/pugl.h"

#include <assert.h>
#include <ctype.h>
#include <string.h>

int
main(void)
{
  for (unsigned i = 0; i <= PUGL_NO_MEMORY; ++i) {
    const char* const string = puglStrerror((PuglStatus)i);

    assert(isupper(string[0]));
    assert(string[strlen(string) - 1] != '.');
    assert(strcmp(string, "Unknown error"));
  }

  assert(!strcmp(puglStrerror((PuglStatus)999), "Unknown error"));

  return 0;
}
