// Copyright 2012-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef PUGL_ATTRIBUTES_H
#define PUGL_ATTRIBUTES_H

// Public declaration scope
#ifdef __cplusplus
#  define PUGL_BEGIN_DECLS extern "C" {
#  define PUGL_END_DECLS }
#else
#  define PUGL_BEGIN_DECLS ///< Begin public API definitions
#  define PUGL_END_DECLS   ///< End public API definitions
#endif

// Symbol exposed in the public API
#ifndef PUGL_API
#  if defined(_WIN32) && !defined(PUGL_STATIC) && defined(PUGL_INTERNAL)
#    define PUGL_API __declspec(dllexport)
#  elif defined(_WIN32) && !defined(PUGL_STATIC)
#    define PUGL_API __declspec(dllimport)
#  elif defined(__GNUC__)
#    define PUGL_API __attribute__((visibility("default")))
#  else
#    define PUGL_API
#  endif
#endif

// Deprecated API
#ifndef PUGL_DISABLE_DEPRECATED
#  if defined(__clang__)
#    define PUGL_DEPRECATED_BY(rep) __attribute__((deprecated("", rep)))
#  elif defined(__GNUC__)
#    define PUGL_DEPRECATED_BY(rep) __attribute__((deprecated("Use " rep)))
#  else
#    define PUGL_DEPRECATED_BY(rep)
#  endif
#endif

// GCC function attributes
#if defined(__GNUC__)
#  define PUGL_CONST_FUNC __attribute__((const))
#  define PUGL_MALLOC_FUNC __attribute__((malloc))
#else
#  define PUGL_CONST_FUNC  ///< Only reads its parameters
#  define PUGL_MALLOC_FUNC ///< Allocates memory
#endif

/// A const function in the public API that only reads parameters
#define PUGL_CONST_API \
  PUGL_API             \
  PUGL_CONST_FUNC

/// A malloc function in the public API that returns allocated memory
#define PUGL_MALLOC_API \
  PUGL_API              \
  PUGL_MALLOC_FUNC

#endif // PUGL_ATTRIBUTES_H
