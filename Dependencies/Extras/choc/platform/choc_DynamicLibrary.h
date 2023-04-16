//
//    ██████ ██   ██  ██████   ██████
//   ██      ██   ██ ██    ██ ██            ** Classy Header-Only Classes **
//   ██      ███████ ██    ██ ██
//   ██      ██   ██ ██    ██ ██           https://github.com/Tracktion/choc
//    ██████ ██   ██  ██████   ██████
//
//   CHOC is (C)2022 Tracktion Corporation, and is offered under the terms of the ISC license:
//
//   Permission to use, copy, modify, and/or distribute this software for any purpose with or
//   without fee is hereby granted, provided that the above copyright notice and this permission
//   notice appear in all copies. THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
//   AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
//   CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
//   WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
//   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#ifndef CHOC_DYNAMIC_LIBRARY_HEADER_INCLUDED
#define CHOC_DYNAMIC_LIBRARY_HEADER_INCLUDED

#include <string>

namespace choc::file
{

//==============================================================================
/**
    A minimal cross-platform loader for .dll/.so files.
*/
struct DynamicLibrary
{
    DynamicLibrary() = default;

    /// Attempts to laod a library with the given name or path.
    DynamicLibrary (std::string_view library);

    DynamicLibrary (const DynamicLibrary&) = delete;
    DynamicLibrary& operator= (const DynamicLibrary&) = delete;
    DynamicLibrary (DynamicLibrary&&);
    DynamicLibrary& operator= (DynamicLibrary&&);

    /// On destruction, this object releases the library that was loaded
    ~DynamicLibrary();

    /// Returns a pointer to the function with this name, or nullptr if not found.
    void* findFunction (std::string_view functionName);

    /// Returns true if the library was successfully loaded
    operator bool() const noexcept              { return handle != nullptr; }

    /// Releases any handle that this object is holding
    void close();

    /// platform-specific handle. Will be nullptr if not loaded
    void* handle = nullptr;
};



//==============================================================================
//        _        _           _  _
//     __| |  ___ | |_   __ _ (_)| | ___
//    / _` | / _ \| __| / _` || || |/ __|
//   | (_| ||  __/| |_ | (_| || || |\__ \ _  _  _
//    \__,_| \___| \__| \__,_||_||_||___/(_)(_)(_)
//
//   Code beyond this point is implementation detail...
//
//==============================================================================

inline DynamicLibrary::~DynamicLibrary()
{
    close();
}

inline DynamicLibrary::DynamicLibrary (DynamicLibrary&& other) : handle (other.handle)
{
    other.handle = nullptr;
}

inline DynamicLibrary& DynamicLibrary::operator= (DynamicLibrary&& other)
{
    close();
    handle = other.handle;
    other.handle = nullptr;
    return *this;
}

} // namespace choc::file

#if ! (defined (_WIN32) || defined (_WIN64))

#include <dlfcn.h>

inline choc::file::DynamicLibrary::DynamicLibrary (std::string_view library)
{
    handle = ::dlopen (std::string (library).c_str(), RTLD_LOCAL | RTLD_NOW);
}

inline void choc::file::DynamicLibrary::close()
{
    if (handle != nullptr)
    {
        ::dlclose (handle);
        handle = nullptr;
    }
}

inline void* choc::file::DynamicLibrary::findFunction (std::string_view name)
{
    if (handle != nullptr)
        return ::dlsym (handle, std::string (name).c_str());

    return {};
}

#else

//==============================================================================
namespace choc::win32_defs
{
#if ! (defined (_WINDOWS_) || defined (_APISETLIBLOADER_)) // only use these local definitions if windows.h isn't already included
  using CHOC_HMODULE = void*;
 #ifdef _MSC_VER
  extern "C" CHOC_HMODULE __stdcall choc_LoadLibraryA (const char*);
  extern "C" int          __stdcall choc_FreeLibrary (CHOC_HMODULE);
  extern "C" void*        __stdcall choc_GetProcAddress (CHOC_HMODULE, const char*);
  #pragma comment(linker,"/alternatename:choc_LoadLibraryA=LoadLibraryA")
  #pragma comment(linker,"/alternatename:choc_FreeLibrary=FreeLibrary")
  #pragma comment(linker,"/alternatename:choc_GetProcAddress=GetProcAddress")
  static inline CHOC_HMODULE  LoadLibraryA (const char* l)                    { return choc_LoadLibraryA (l); }
  static inline int           FreeLibrary (CHOC_HMODULE m)                    { return choc_FreeLibrary (m); }
  static inline void*         GetProcAddress (CHOC_HMODULE m, const char* f)  { return choc_GetProcAddress (m, f); }
 #else
  extern "C" __declspec(dllimport)  CHOC_HMODULE __stdcall LoadLibraryA (const char*);
  extern "C" __declspec(dllimport)  int          __stdcall FreeLibrary (CHOC_HMODULE);
  extern "C" __declspec(dllimport)  void*        __stdcall GetProcAddress (CHOC_HMODULE, const char*);
 #endif
#else
 using CHOC_HMODULE = HMODULE;
 static inline CHOC_HMODULE LoadLibraryA (const char* l)                    { return ::LoadLibraryA (l); }
 static inline int          FreeLibrary (CHOC_HMODULE m)                    { return ::FreeLibrary (m); }
 static inline void*        GetProcAddress (CHOC_HMODULE m, const char* f)  { return (void*) ::GetProcAddress (m, f); }
#endif
}

inline choc::file::DynamicLibrary::DynamicLibrary (std::string_view library)
{
    handle = (void*) win32_defs::LoadLibraryA (std::string (library).c_str());
}

inline void choc::file::DynamicLibrary::close()
{
    if (handle != nullptr)
    {
        win32_defs::FreeLibrary ((win32_defs::CHOC_HMODULE) handle);
        handle = nullptr;
    }
}

inline void* choc::file::DynamicLibrary::findFunction (std::string_view name)
{
    if (handle != nullptr)
        return (void*) win32_defs::GetProcAddress ((win32_defs::CHOC_HMODULE) handle, std::string (name).c_str());

    return {};
}

#endif

#endif  // CHOC_DYNAMIC_LIBRARY_HEADER_INCLUDED
