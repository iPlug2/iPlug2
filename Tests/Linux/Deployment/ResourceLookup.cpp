#include "IPlugPaths.h"
#include <cstdio>
#include <cstring>

extern "C" bool LinuxResourceExists()
{
  WDL_String path;
  return iplug::LocateResource("late.txt", "txt", path, "", nullptr, nullptr) !=
    iplug::EResourceLocation::kNotFound;
}

extern "C" bool LinuxResourceMatches(const char* expected)
{
  WDL_String path;
  if (iplug::LocateResource("late.txt", "txt", path, "", nullptr, nullptr) ==
      iplug::EResourceLocation::kNotFound)
    return false;
  FILE* file = fopen(path.Get(), "rb");
  if (!file)
  {
    perror(path.Get());
    return false;
  }
  char contents[128] {};
  const size_t length = fread(contents, 1, sizeof(contents), file);
  const bool complete = fgetc(file) == EOF;
  const bool error = ferror(file);
  if (error)
    perror(path.Get());
  fclose(file);
  return !error && complete && length == strlen(expected) && memcmp(contents, expected, length) == 0;
}
