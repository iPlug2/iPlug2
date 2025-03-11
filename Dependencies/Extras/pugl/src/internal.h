// Copyright 2012-2023 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

// Internal utilities available to platform implementations

#ifndef PUGL_INTERNAL_H
#define PUGL_INTERNAL_H

#include "attributes.h"
#include "types.h"

#include "pugl/attributes.h"
#include "pugl/pugl.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

PUGL_BEGIN_DECLS

/// Return true if `size` is a valid view size
bool
puglIsValidSize(PuglViewSize size);

/// Set hint to a default value if it is unset (PUGL_DONT_CARE)
void
puglEnsureHint(PuglView* view, PuglViewHint hint, int value);

/// Set `blob` to `data` with length `len`, reallocating if necessary
PuglStatus
puglSetBlob(PuglBlob* dest, const void* data, size_t len);

/// Reallocate and set `*dest` to `string`
void
puglSetString(char** dest, const char* string);

/// Handle a changed string property
PUGL_API
PuglStatus
puglViewStringChanged(PuglView* view, PuglStringHint key, const char* value);

/// Return the Unicode code point for `buf` or the replacement character
uint32_t
puglDecodeUTF8(const uint8_t* buf);

/// Prepare a view to be realized by the platform implementation if possible
PuglStatus
puglPreRealize(PuglView* view);

/// Dispatch an event with a simple `type` to `view`
PuglStatus
puglDispatchSimpleEvent(PuglView* view, PuglEventType type);

/// Process configure event while already in the graphics context
PUGL_WARN_UNUSED_RESULT
PuglStatus
puglConfigure(PuglView* view, const PuglEvent* event);

/// Dispatch `event` to `view`, entering graphics context if necessary
PuglStatus
puglDispatchEvent(PuglView* view, const PuglEvent* event);

PUGL_END_DECLS

#endif // PUGL_INTERNAL_H
