#include <cstdio>
#include <cstring>
#include <dlfcn.h>

int main(int argc, char** argv)
{
  if (argc < 2 || argc > 3)
    return 1;
  void* module = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
  if (!module)
  {
    fprintf(stderr, "Could not load resource fixture: %s\n", dlerror());
    return 1;
  }
  const auto exists = reinterpret_cast<bool (*)()>(dlsym(module, "LinuxResourceExists"));
  const auto matches = reinterpret_cast<bool (*)(const char*)>(dlsym(module, "LinuxResourceMatches"));
  const bool missing = argc == 3 && strcmp(argv[2], "--missing") == 0;
  bool found = false;
  if (argc == 2)
    found = exists && exists();
  else if (missing)
    found = exists && !exists();
  else
    found = matches && matches(argv[2]);
  if (!found)
    fprintf(stderr, "Resource lookup failed through '%s'\n", argv[1]);
  dlclose(module);
  return found ? 0 : 1;
}
