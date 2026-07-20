// Copyright 2012-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

// The API that a platform implementation must define

#ifndef PUGL_PLATFORM_H
#define PUGL_PLATFORM_H

#include "types.h"

#include "pugl/pugl.h"

PUGL_BEGIN_DECLS

/// Allocate and initialise world internals (implemented once per platform)
PUGL_MALLOC_FUNC
PuglWorldInternals*
puglInitWorldInternals(PuglWorldType type, PuglWorldFlags flags);

/// Destroy and free world internals (implemented once per platform)
void
puglFreeWorldInternals(PuglWorld* world);

/// Allocate and initialise view internals (implemented once per platform)
PUGL_MALLOC_FUNC
PuglInternals*
puglInitViewInternals(PuglWorld* world);

/// Destroy and free view internals (implemented once per platform)
void
puglFreeViewInternals(PuglView* view);

PUGL_END_DECLS

#endif // PUGL_PLATFORM_H
