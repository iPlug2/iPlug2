// Copyright 2012-2023 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_CAIRO_H
#define PUGL_CAIRO_H

#include "pugl/pugl.h"

PUGL_BEGIN_DECLS

/**
   @defgroup pugl_cairo Cairo
   Cairo graphics support.
   @ingroup pugl
   @{
*/

/**
   Cairo graphics backend accessor.

   Pass the returned value to puglSetBackend() to draw to a view with Cairo.
*/
PUGL_CONST_API
const PuglBackend*
puglCairoBackend(void);

/**
   @}
*/

PUGL_END_DECLS

#endif // PUGL_CAIRO_H
