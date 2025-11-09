// Copyright 2012-2023 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_GL_HPP
#define PUGL_GL_HPP

#include "pugl/gl.h"
#include "pugl/pugl.h"
#include "pugl/pugl.hpp"

namespace pugl {

/**
   @defgroup puglpp_gl OpenGL
   OpenGL graphics support.
   @ingroup puglpp
   @{
*/

/// @copydoc PuglGlFunc
using GlFunc = PuglGlFunc;

/// @copydoc puglGetProcAddress
inline GlFunc
getProcAddress(const char* name) noexcept
{
  return puglGetProcAddress(name);
}

/// @copydoc puglEnterContext
inline Status
enterContext(View& view) noexcept
{
  return static_cast<Status>(puglEnterContext(view.cobj()));
}

/// @copydoc puglLeaveContext
inline Status
leaveContext(View& view) noexcept
{
  return static_cast<Status>(puglLeaveContext(view.cobj()));
}

/// @copydoc puglGlBackend
inline const PuglBackend*
glBackend() noexcept
{
  return puglGlBackend();
}

/**
   @}
*/

} // namespace pugl

#endif // PUGL_GL_HPP
