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

#ifndef CHOC_MEMORYDLL_HEADER_INCLUDED
#define CHOC_MEMORYDLL_HEADER_INCLUDED

#include <stddef.h>
#include <memory>
#include <string>

namespace choc::memory
{

/**
    MemoryDLL is an egregious hack that allows you to load DLL files from a chunk
    of memory in the same way you might load them from a file on disk.

    This opens up the ability to do horrible things such as embedding a random DLL
    in a chunk of C++ code, so that it gets baked directly into your executable..
    That means that if your app requires a 3rd-party DLL but you don't want the hassle
    of having to install the DLL file in the right place on your users' machines, you
    could use this trick to embed it invisibly inside your executable...

    Currently this is only implemented for Windows DLLs, but a linux/OSX loader for
    .dylibs is also totally feasible if anyone fancies having a go :)

    @see DynamicLibrary
*/
struct MemoryDLL
{
    MemoryDLL() = default;
    MemoryDLL (MemoryDLL&&) = default;
    MemoryDLL& operator= (MemoryDLL&&) = default;
    ~MemoryDLL();

    /// Attempts to load a chunk of memory that contains a DLL file image.
    MemoryDLL (const void* data, size_t size);

    /// Returns a pointer to the function with this name, or nullptr if not found.
    void* findFunction (std::string_view functionName);

    /// Returns true if the library was successfully loaded
    operator bool() const           { return pimpl != nullptr; }

private:
    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;
};

} // namespace choc::memory



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

#if defined (_WIN32) || defined (_WIN64)

#include <vector>
#include <unordered_map>
#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#undef  NOMINMAX
#define NOMINMAX
#define Rectangle Rectangle_renamed_to_avoid_name_collisions
#include <windows.h>
#undef Rectangle

namespace choc::memory
{

struct MemoryDLL::Pimpl
{
    Pimpl() = default;

    ~Pimpl()
    {
        if (entryFunction != nullptr)
            (*reinterpret_cast<DLLEntryFn> (entryFunction)) ((HINSTANCE) imageData, DLL_PROCESS_DETACH, nullptr);

        for (auto& m : loadedModules)
            FreeLibrary (m);

        if (imageData != nullptr)
            VirtualFree (imageData, 0, MEM_RELEASE);

        for (auto m : virtualBlocks)
            VirtualFree (m, 0, MEM_RELEASE);
    }

    bool initialise (const void* data, size_t size)
    {
        if (size < sizeof (IMAGE_DOS_HEADER))
            return false;

        auto dosHeader = static_cast<const IMAGE_DOS_HEADER*> (data);

        if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE
             || size < static_cast<size_t> (dosHeader->e_lfanew) + sizeof (IMAGE_NT_HEADERS))
            return false;

        const auto& headers = *getOffsetAs<IMAGE_NT_HEADERS> (data, dosHeader->e_lfanew);

        if (headers.Signature != IMAGE_NT_SIGNATURE
             || (headers.OptionalHeader.SectionAlignment & 1) != 0)
            return false;

       #ifdef _M_ARM64
        if (headers.FileHeader.Machine != IMAGE_FILE_MACHINE_ARM64) return false;
       #elif defined (_WIN64)
        if (headers.FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64) return false;
       #else
        if (headers.FileHeader.Machine != IMAGE_FILE_MACHINE_I386) return false;
       #endif

        SYSTEM_INFO systemInfo;
        GetNativeSystemInfo (std::addressof (systemInfo));

        auto alignedImageSize = roundUp (headers.OptionalHeader.SizeOfImage, systemInfo.dwPageSize);

        if (alignedImageSize != roundUp (getLastSectionEnd (headers), systemInfo.dwPageSize))
            return false;

        imageData = VirtualAlloc (reinterpret_cast<void*> (headers.OptionalHeader.ImageBase),
                                  alignedImageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

        if (imageData == nullptr)
        {
            imageData = VirtualAlloc (nullptr, alignedImageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

            if (imageData == nullptr)
                return false;
        }

        while ((((uint64_t) imageData) >> 32) < (((uint64_t) imageData + alignedImageSize) >> 32))
        {
            virtualBlocks.push_back (imageData);
            imageData = VirtualAlloc (nullptr, alignedImageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

            if (imageData == nullptr)
                return false;
        }

        isDLL = (headers.FileHeader.Characteristics & IMAGE_FILE_DLL) != 0;
        pageSize = systemInfo.dwPageSize;

        if (size < headers.OptionalHeader.SizeOfHeaders)
            return false;

        auto newHeaders = VirtualAlloc (imageData, headers.OptionalHeader.SizeOfHeaders, MEM_COMMIT, PAGE_READWRITE);
        memcpy (newHeaders, dosHeader, headers.OptionalHeader.SizeOfHeaders);
        imageHeaders = getOffsetAs<IMAGE_NT_HEADERS> (newHeaders, dosHeader->e_lfanew);
        imageHeaders->OptionalHeader.ImageBase = reinterpret_cast<uintptr_t> (imageData);

        if (copySections (data, size, headers.OptionalHeader.SectionAlignment))
        {
            if (auto locationDelta = (ptrdiff_t) (imageHeaders->OptionalHeader.ImageBase - headers.OptionalHeader.ImageBase))
                performRelocation (locationDelta);

            if (loadImports() && prepareSections())
            {
                executeTLS();
                loadNameTable();

                if (imageHeaders->OptionalHeader.AddressOfEntryPoint != 0)
                {
                    entryFunction = getOffsetAs<char> (imageData, imageHeaders->OptionalHeader.AddressOfEntryPoint);

                    if (isDLL)
                        return (*reinterpret_cast<DLLEntryFn> (entryFunction)) ((HINSTANCE) imageData, DLL_PROCESS_ATTACH, nullptr);

                    return true;
                }
            }
        }

        return false;
    }

    void* findFunction (std::string_view name)
    {
        if (auto found = exportedFunctionOffsets.find (std::string (name)); found != exportedFunctionOffsets.end())
            return getOffsetAs<char> (imageData, found->second);

        return {};
    }

private:
    PIMAGE_NT_HEADERS imageHeaders = {};
    void* imageData = nullptr;
    std::vector<HMODULE> loadedModules;
    uint32_t pageSize = 0;
    bool isDLL = false;
    std::vector<void*> virtualBlocks;
    std::unordered_map<std::string, size_t> exportedFunctionOffsets;
    using DLLEntryFn = BOOL(WINAPI*)(HINSTANCE, DWORD, void*);
    void* entryFunction = {};

    template <typename Type, typename Diff>
    static const Type* getOffsetAs (const void* address, Diff offset)
    {
        return reinterpret_cast<const Type*> (static_cast<const char*> (address) + offset);
    }

    template <typename Type, typename Diff>
    static Type* getOffsetAs (void* address, Diff offset)
    {
        return reinterpret_cast<Type*> (static_cast<char*> (address) + offset);
    }

    template <typename Type>
    Type* getDataDirectoryAddress (int type) const
    {
        auto& dd = imageHeaders->OptionalHeader.DataDirectory[type];

        if (dd.Size > 0)
            return getOffsetAs<Type> (imageData, dd.VirtualAddress);

        return {};
    }

    static size_t getLastSectionEnd (const IMAGE_NT_HEADERS& headers)
    {
        auto section = IMAGE_FIRST_SECTION (&headers);
        auto optionalSectionSize = headers.OptionalHeader.SectionAlignment;
        size_t lastSectionEnd = 0;

        for (uint32_t i = 0; i < headers.FileHeader.NumberOfSections; ++i, ++section)
        {
            auto end = section->VirtualAddress + (section->SizeOfRawData == 0 ? optionalSectionSize
                                                                              : section->SizeOfRawData);
            if (end > lastSectionEnd)
                lastSectionEnd = end;
        }

        return lastSectionEnd;
    }

    void loadNameTable()
    {
        if (auto exports = getDataDirectoryAddress<IMAGE_EXPORT_DIRECTORY> (IMAGE_DIRECTORY_ENTRY_EXPORT))
        {
            if (exports->NumberOfNames > 0 && exports->NumberOfFunctions > 0)
            {
                auto name    = getOffsetAs<const DWORD> (imageData, exports->AddressOfNames);
                auto ordinal = getOffsetAs<const WORD>  (imageData, exports->AddressOfNameOrdinals);

                exportedFunctionOffsets.reserve (exports->NumberOfNames);

                for (size_t i = 0; i < exports->NumberOfNames; ++i, ++name, ++ordinal)
                    if (*ordinal <= exports->NumberOfFunctions)
                        exportedFunctionOffsets[std::string (getOffsetAs<const char> (imageData, *name))]
                           = getOffsetAs<DWORD> (imageData, exports->AddressOfFunctions)[*ordinal];
            }
        }
    }

    void executeTLS()
    {
        if (auto tls = getDataDirectoryAddress<IMAGE_TLS_DIRECTORY> (IMAGE_DIRECTORY_ENTRY_TLS))
            if (auto callback = reinterpret_cast<PIMAGE_TLS_CALLBACK*> (tls->AddressOfCallBacks))
                while (*callback != nullptr)
                    (*callback++) ((void*) imageData, DLL_PROCESS_ATTACH, nullptr);
    }

    bool prepareSections()
    {
        auto section         = IMAGE_FIRST_SECTION (imageHeaders);
        auto size            = getSectionSize (*section);
        auto address         = getSectionAddress (*section);
        auto addressPage     = getPageBase (address);
        auto characteristics = section->Characteristics;

        for (WORD i = 1; i < imageHeaders->FileHeader.NumberOfSections; ++i)
        {
            ++section;
            auto nextSize        = getSectionSize (*section);
            auto nextAddress     = getSectionAddress (*section);
            auto nextAddressPage = getPageBase (nextAddress);

            if (addressPage == nextAddressPage || address + size > nextAddressPage)
            {
                if ((section->Characteristics & IMAGE_SCN_MEM_DISCARDABLE) == 0 || (characteristics & IMAGE_SCN_MEM_DISCARDABLE) == 0)
                    characteristics = (characteristics | section->Characteristics) & ~static_cast<DWORD> (IMAGE_SCN_MEM_DISCARDABLE);
                else
                    characteristics |= section->Characteristics;

                size = static_cast<size_t> ((nextAddress + nextSize) - address);
                continue;
            }

            if (! setProtectionFlags (size, characteristics, address, addressPage, false))
                return false;

            size = nextSize;
            address = nextAddress;
            addressPage = nextAddressPage;
            characteristics = section->Characteristics;
        }

        return setProtectionFlags (size, characteristics, address, addressPage, true);
    }

    bool setProtectionFlags (size_t sectionSize, DWORD sectionCharacteristics, void* sectionAddress, void* addressPage, bool isLast)
    {
        if (sectionSize == 0)
            return true;

        if ((sectionCharacteristics & IMAGE_SCN_MEM_DISCARDABLE) != 0)
        {
            if (sectionAddress == addressPage
                 && (isLast || imageHeaders->OptionalHeader.SectionAlignment == pageSize || (sectionSize % pageSize) == 0))
                VirtualFree (sectionAddress, sectionSize, MEM_DECOMMIT);

            return true;
        }

        auto getProtectionFlags = [] (DWORD type)
        {
            return ((type & IMAGE_SCN_MEM_NOT_CACHED) ? PAGE_NOCACHE : 0)
                    | ((type & IMAGE_SCN_MEM_EXECUTE)
                        ? ((type & IMAGE_SCN_MEM_READ)
                            ? ((type & IMAGE_SCN_MEM_WRITE) ? PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ)
                            : ((type & IMAGE_SCN_MEM_WRITE) ? PAGE_EXECUTE_WRITECOPY : PAGE_EXECUTE))
                        : ((type & IMAGE_SCN_MEM_READ)
                            ? ((type & IMAGE_SCN_MEM_WRITE) ? PAGE_READWRITE : PAGE_READONLY)
                            : ((type & IMAGE_SCN_MEM_WRITE) ? PAGE_WRITECOPY : PAGE_NOACCESS)));
        };

        DWORD oldProtectValue;
        return VirtualProtect (sectionAddress, sectionSize,
                               static_cast<DWORD> (getProtectionFlags (sectionCharacteristics)),
                               std::addressof (oldProtectValue)) != 0;
    }

    bool loadImports()
    {
        auto imp = getDataDirectoryAddress<IMAGE_IMPORT_DESCRIPTOR> (IMAGE_DIRECTORY_ENTRY_IMPORT);

        if (imp == nullptr)
            return false;

        while (! IsBadReadPtr (imp, sizeof (IMAGE_IMPORT_DESCRIPTOR)) && imp->Name != 0)
        {
            auto handle = LoadLibraryA (getOffsetAs<const char> (imageData, imp->Name));

            if (handle == nullptr)
                return false;

            loadedModules.push_back (handle);
            auto thunkRef = getOffsetAs<uintptr_t> (imageData, imp->OriginalFirstThunk ? imp->OriginalFirstThunk
                                                                                       : imp->FirstThunk);
            auto funcRef = getOffsetAs<FARPROC> (imageData, imp->FirstThunk);

            for (; *thunkRef != 0; ++thunkRef, ++funcRef)
            {
                auto name = IMAGE_SNAP_BY_ORDINAL(*thunkRef)
                              ? (LPCSTR) IMAGE_ORDINAL(*thunkRef)
                              : (LPCSTR) std::addressof (getOffsetAs<IMAGE_IMPORT_BY_NAME> (imageData, *thunkRef)->Name);

                *funcRef = GetProcAddress (handle, name);

                if (*funcRef == 0)
                    return false;
            }

            ++imp;
        }

        return true;
    }

    size_t getSectionSize (const IMAGE_SECTION_HEADER& section) const
    {
        if (auto size = section.SizeOfRawData)
            return size;

        if (section.Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)
            return imageHeaders->OptionalHeader.SizeOfInitializedData;

        if (section.Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
            return imageHeaders->OptionalHeader.SizeOfUninitializedData;

        return 0;
    }

    static size_t roundUp (size_t value, size_t alignment)  { return (value + alignment - 1) & ~(alignment - 1); }
    char* getPageBase (void* address) const  { return (char*) (reinterpret_cast<uintptr_t> (address) & ~static_cast<uintptr_t> (pageSize - 1)); }

    char* getSectionAddress (const IMAGE_SECTION_HEADER& section) const
    {
       #ifdef _WIN64
        return reinterpret_cast<char*> (static_cast<uintptr_t> (section.Misc.PhysicalAddress)
                                         | (static_cast<uintptr_t> (imageHeaders->OptionalHeader.ImageBase & 0xffffffff00000000)));
       #else
        return reinterpret_cast<char*> (section.Misc.PhysicalAddress);
       #endif
    }

    bool copySections (const void* data, size_t size, size_t sectionAlignment) const
    {
        auto section = IMAGE_FIRST_SECTION (imageHeaders);

        for (int i = 0; i < imageHeaders->FileHeader.NumberOfSections; ++i, ++section)
        {
            if (section->SizeOfRawData == 0)
            {
                if (sectionAlignment == 0)
                    continue;

                if (auto dest = VirtualAlloc (getOffsetAs<char> (imageData, section->VirtualAddress),
                                                sectionAlignment, MEM_COMMIT, PAGE_READWRITE))
                {
                    dest = getOffsetAs<char> (imageData, section->VirtualAddress);
                    section->Misc.PhysicalAddress = static_cast<uint32_t> (reinterpret_cast<uintptr_t> (dest));
                    memset (dest, 0, sectionAlignment);
                    continue;
                }

                return false;
            }

            if (size < section->PointerToRawData + section->SizeOfRawData)
                return false;

            if (auto dest = VirtualAlloc (getOffsetAs<char> (imageData, section->VirtualAddress),
                                          section->SizeOfRawData, MEM_COMMIT, PAGE_READWRITE))
            {
                dest = getOffsetAs<char> (imageData, section->VirtualAddress);
                memcpy (dest, static_cast<const char*> (data) + section->PointerToRawData, section->SizeOfRawData);
                section->Misc.PhysicalAddress = static_cast<uint32_t> (reinterpret_cast<uintptr_t> (dest));
                continue;
            }

            return false;
        }

        return true;
    }

    void performRelocation (ptrdiff_t delta)
    {
        auto directory = imageHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];

        if (directory.Size != 0)
        {
            auto relocation = getOffsetAs<IMAGE_BASE_RELOCATION> (imageData, directory.VirtualAddress);

            while (relocation->VirtualAddress > 0)
            {
                auto dest = getOffsetAs<char> (imageData, relocation->VirtualAddress);
                auto offset = getOffsetAs<uint16_t> (relocation, sizeof (IMAGE_BASE_RELOCATION));

                for (uint32_t i = 0; i < (relocation->SizeOfBlock - sizeof (IMAGE_BASE_RELOCATION)) / 2; ++i, ++offset)
                {
                    switch (*offset >> 12)
                    {
                        case IMAGE_REL_BASED_HIGHLOW:   addDelta<uint32_t> (dest + (*offset & 0xfff), delta); break;
                        case IMAGE_REL_BASED_DIR64:     addDelta<uint64_t> (dest + (*offset & 0xfff), delta); break;
                        case IMAGE_REL_BASED_ABSOLUTE:
                        default:                        break;
                    }
                }

                relocation = getOffsetAs<IMAGE_BASE_RELOCATION> (relocation, relocation->SizeOfBlock);
            }
        }
    }

    template <typename Type>
    static void addDelta (char* addr, ptrdiff_t delta)
    {
        *reinterpret_cast<Type*> (addr) = static_cast<Type> (static_cast<ptrdiff_t> (*reinterpret_cast<Type*> (addr)) + delta);
    }
};

#else

#include "choc_Assert.h"

namespace choc::memory
{
struct MemoryDLL::Pimpl
{
    bool initialise (const void*, size_t)   { CHOC_ASSERT (false); return {}; } // Only available on Windows!
    void* findFunction (std::string_view)   { CHOC_ASSERT (false); return {}; } // Only available on Windows!
};

#endif

inline MemoryDLL::~MemoryDLL() = default;

inline MemoryDLL::MemoryDLL (const void* data, size_t size) : pimpl (std::make_unique<Pimpl>())
{
    if (! pimpl->initialise (data, size))
        pimpl.reset();
}

inline void* MemoryDLL::findFunction (std::string_view name)
{
    return pimpl != nullptr ? pimpl->findFunction (name) : nullptr;
}

} // namespace choc::memory

#endif // CHOC_MEMORYDLL_HEADER_INCLUDED
