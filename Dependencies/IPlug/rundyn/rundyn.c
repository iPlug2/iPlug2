/*
See the end of this file for license information.

This program emulates rundll32.exe (from Windows) on unix platforms.
*/


#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PROG_NAME "rundyn"
#define ERR_RET (50)
void print_help();
typedef int (*fn_main_t)(int, char**);

#ifdef _WIN32
#include <windows.h>
typedef HMODULE libref_t;
libref_t _dlopen(const char* path)
{
  return LoadLibraryA(path);
}
void* _dlsym(libref_t module, const char* func_name)
{
  return (void*)GetProcAddress(module, func_name);
}
#else
#include <dlfcn.h>
typedef void* libref_t;
libref_t _dlopen(const char* path)
{
  return dlopen(path, RTLD_NOW | RTLD_GLOBAL);
}
void* _dlsym(libref_t module, const char* func_name)
{
  return dlsym(module, func_name);
}
#endif

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    print_help();
    return ERR_RET + 0;
  }
  if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
  {
    print_help();
    return 0;
  }
  
  // Find last ,
  char *path = argv[1];
  int idx = (int)strlen(path) - 1;
  while (idx >= 0 && path[idx] != ',')
  {
    idx--;
  }

  if (idx == 0)
  {
    fprintf(stderr, "ERROR: No comma (,) in first argument.\n");
    print_help();
    return ERR_RET + 1;
  }

  // Effectively split the string
  path[idx] = 0;

  // Load dll/so/dylib
  libref_t hnd = _dlopen(path);
  if (hnd == NULL)
  {
    fprintf(stderr, "ERROR: Unable to load library \"%s\"\n", path);
    return ERR_RET + 2;
  }

  // Everything after the last comma is the name of the symbol to load
  const char *fn_name = path + idx + 1;
  void* fnptr = _dlsym(hnd, fn_name);
  if (fnptr == NULL)
  {
    fprintf(stderr, "ERROR: Symbol \"%s\" not found\n", fn_name);
    return ERR_RET + 3;
  }

  // Call the main entry function. arg0 should be the path of the 
  // executable and argv+1 will point to "path", so that all works out. 
  return ((fn_main_t)fnptr)(argc - 1, argv + 1);
}

void print_help()
{
  fprintf(stdout, 
    PROG_NAME " <shared_library>,<main_function> <args>...\n\n"
    "The first argument must be a path to the .so (or .dylib, or .dll) you want to run\n"
    "then a comma (,) and then the name of the function to run as \"main\"\n"
    "Example: ./" PROG_NAME " ./test.so,my_main_func argument1 argument2\n"
  );
}

/*
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2020 Andrew Heintz
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
