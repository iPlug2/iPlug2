// Copyright 2019-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "file_utils.h"

#ifdef _WIN32
#  include <io.h>
#  include <windows.h>
#  define F_OK 0
#else
#  include <libgen.h>
#  include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char*
resourcePath(const char* const programPath, const char* const name)
{
  char* const binary = strdup(programPath);

#ifdef _WIN32
  char programDir[_MAX_DIR];
  _splitpath(binary, programDir, NULL, NULL, NULL);
  _splitpath(binary, NULL, programDir + strlen(programDir), NULL, NULL);
  programDir[strlen(programDir) - 1] = '\0';
#else
  char* const programDir = dirname(binary);
#endif

  const size_t programDirLen = strlen(programDir);
  const size_t nameLen       = strlen(name);
  const size_t totalLen      = programDirLen + nameLen + 4;

  char* const programRelative = (char*)calloc(totalLen, 1);
  snprintf(programRelative, totalLen, "%s/%s", programDir, name);
  if (!access(programRelative, F_OK)) {
    free(binary);
    return programRelative;
  }

  free(programRelative);
  free(binary);

  const size_t sysPathLen = strlen(PUGL_DATA_DIR) + nameLen + 4;
  char* const  sysPath    = (char*)calloc(sysPathLen, 1);
  snprintf(sysPath, sysPathLen, "%s/%s", PUGL_DATA_DIR, name);
  return sysPath;
}
