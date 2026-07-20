// Copyright 2019-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef EXAMPLES_FILE_UTILS_H
#define EXAMPLES_FILE_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
   Return the path to a resource file.

   This takes a name like "shaders/something.glsl" and returns the actual
   path that can be used to load that resource, which may be relative to the
   current executable (for running in bundles or the build directory), or a
   shared system directory for installs.

   The returned path must be freed with free().
*/
char*
resourcePath(const char* programPath, const char* name);

#ifdef __cplusplus
}
#endif

#endif // EXAMPLES_FILE_UTILS_H
