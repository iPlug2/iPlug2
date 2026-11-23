// Copyright 2012-2023 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_GL_H
#define PUGL_GL_H

#include "pugl/pugl.h"

// IWYU pragma: begin_exports

/* Unfortunately, GL includes vary across platforms, so include them here to
   enable pure portable programs. */

#ifndef PUGL_NO_INCLUDE_GL_H
#  ifdef __APPLE__
#    include <OpenGL/gl.h>
#  else
#    ifdef _WIN32
#      include <windows.h>
#    endif
#    include <GL/gl.h>
#  endif
#endif

// IWYU pragma: end_exports

PUGL_BEGIN_DECLS

/**
   @defgroup pugl_gl OpenGL
   OpenGL graphics support.
   @ingroup pugl
   @{
*/

/**
   OpenGL extension function.
*/
typedef void (*PuglGlFunc)(void);

/**
   Return the address of an OpenGL extension function.
*/
PUGL_API
PuglGlFunc
puglGetProcAddress(const char* name);

/**
   Enter the OpenGL context.

   This can be used to enter the graphics context in unusual situations, for
   doing things like loading textures.  Note that this must not be used for
   drawing, which may only be done while processing an expose event.
*/
PUGL_API
PuglStatus
puglEnterContext(PuglView* view);

/**
   Leave the OpenGL context.

   This must only be called after puglEnterContext().
*/
PUGL_API
PuglStatus
puglLeaveContext(PuglView* view);

/**
   OpenGL graphics backend.

   Pass the returned value to puglSetBackend() to draw to a view with OpenGL.
*/
PUGL_CONST_API
const PuglBackend*
puglGlBackend(void);

PUGL_END_DECLS

/**
   @}
*/

#endif // PUGL_GL_H
