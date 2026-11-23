// Copyright 2012-2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "pugl/stub.h"

#include "stub.h"
#include "types.h"
#include "x11.h"

#include "pugl/pugl.h"

const PuglBackend*
puglStubBackend(void)
{
  static const PuglBackend backend = {
    puglX11Configure,
    puglStubCreate,
    puglStubDestroy,
    puglStubEnter,
    puglStubLeave,
    puglStubGetContext,
  };

  return &backend;
}
