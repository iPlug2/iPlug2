// Copyright 2012-2023 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_GLU_H
#define PUGL_GLU_H

// IWYU pragma: begin_exports

#ifndef PUGL_NO_INCLUDE_GLU_H
#  ifdef __APPLE__
#    include <OpenGL/glu.h>
#  else
#    ifdef _WIN32
#      include <windows.h>
#    endif
#    include <GL/glu.h>
#  endif
#endif

// IWYU pragma: end_exports

#endif // PUGL_GLU_H
