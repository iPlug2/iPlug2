// Copyright 2012-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_SRC_ATTRIBUTES_H
#define PUGL_SRC_ATTRIBUTES_H

// Unused parameter macro to suppresses warnings and make it impossible to use
#if defined(__cplusplus)
#  define PUGL_UNUSED(name)
#elif defined(__GNUC__) || defined(__clang__)
#  define PUGL_UNUSED(name) name##_unused __attribute__((__unused__))
#else
#  define PUGL_UNUSED(name) name
#endif

// Unused result macro to warn when an important return status is ignored
#ifndef _MSC_VER
#  define PUGL_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#  define PUGL_WARN_UNUSED_RESULT
#endif

#endif // PUGL_SRC_ATTRIBUTES_H
