// Copyright 2019-2023 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_STUB_H
#define PUGL_STUB_H

#include "pugl/pugl.h"

PUGL_BEGIN_DECLS

/**
   @defgroup pugl_stub Stub
   Native graphics support.
   @ingroup pugl
   @{
*/

/**
   Stub graphics backend accessor.

   This backend just creates a simple native window without setting up any
   portable graphics API.
*/
PUGL_CONST_API
const PuglBackend*
puglStubBackend(void);

/**
   @}
*/

PUGL_END_DECLS

#endif // PUGL_STUB_H
