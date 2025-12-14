/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

/**
 * @file
 * @brief IPlugPaths implementation for Windows and Linux
 */

#include "IPlugPlatform.h"
#include "IPlugConstants.h"
#include "IPlugPaths.h"

#if defined OS_WEB
#include <emscripten/val.h>
#elif defined OS_WIN
#include <windows.h>
#include <Shlobj.h>
#include <Shlwapi.h>
#elif defined OS_LINUX
#include <dlfcn.h>
#include <limits.h>
#include <sys/stat.h>
#endif



BEGIN_IPLUG_NAMESPACE

extern "C" {
struct embedded_file { const char* name; const uint32_t size; const unsigned char* data; };
}

// OS-specific implementation to dynamically get the embed list symbol.
static void* GetEmbedListPtr();

static bool FindEmbedResource(const char* name, const void** dataOut, size_t* dataSz)
{
  static void* sEmbedList = GetEmbedListPtr();
  if (!sEmbedList) return false;

  // cast
  auto embedList = reinterpret_cast<const embedded_file*>(sEmbedList);
  // zero output values
  *dataOut = nullptr;
  *dataSz = 0;
  // Determine name length
  const size_t nameLen = strnlen(name, PATH_MAX);
  for (int i = 0; embedList[i].name; ++i)
  {
    if (strncmp(name, embedList[i].name, nameLen) == 0)
    {
      *dataOut = embedList[i].data;
      *dataSz = embedList[i].size;
      return true;
    }
  }
  return false;
}

// Base resource deconstructor assumes there's nothing else to do.
Resource::~Resource() {}

std::shared_ptr<Resource> Resource::fromBuffer(const void* data, size_t len)
{
  auto res = std::shared_ptr<Resource>(new Resource());
  res->mData = reinterpret_cast<const uint8_t*>(data);
  res->mSize = len;
  return res;
}

struct DeallocResource : public Resource
{
  DeallocResource(const uint8_t* data, size_t len)
  {
    mData = data;
    mSize = len;
  }

  ~DeallocResource() override
  {
    free((void*)mData);
  }
};

std::shared_ptr<Resource> Resource::fromFile(const char* path)
{
  FILE* f = fopenUTF8(path, "rb");
  long totalSize = 0;
  size_t totalSizeU = 0;
  uint8_t *dataPtr = nullptr;

  if (!f) return nullptr;
  // Seek to end to determine the file size.
  if (fseek(f, 0, SEEK_END) != 0) goto fail;
  totalSize = ftell(f);
  if (totalSize == -1L) goto fail;
  // Seek back to the start.
  if (fseek(f, 0, SEEK_SET) != 0) goto fail;
  // Try to allocate memory for the resource.
  totalSizeU = static_cast<size_t>(totalSize);
  dataPtr = (uint8_t*)::malloc(totalSizeU);
  // Check allocation success.
  if (!dataPtr) goto fail;
  // Read data.
  if (fread(dataPtr, 1, totalSizeU, f) != totalSizeU) goto fail;
  // Close the file.
  fclose(f);
  // Success.
  return std::make_shared<DeallocResource>(dataPtr, totalSizeU);

  fail:
  if (dataPtr) ::free(dataPtr);
  if (f) ::fclose(f);
  return nullptr;
}

#if defined OS_WIN
#pragma mark - OS_WIN

 // Helper for getting a known folder in UTF8
void GetKnownFolder(WDL_String &path, int identifier, int flags = 0)
{
  wchar_t wideBuffer[1024];

  SHGetFolderPathW(NULL, identifier, NULL, flags, wideBuffer);
  UTF16ToUTF8(path, wideBuffer);
}

static void GetModulePath(HMODULE hModule, WDL_String& path)
{
  wchar_t pathCStrW[MAX_WIN32_PATH_LEN] = {'\0'};

  path.Set("");

  if (GetModuleFileNameW(hModule, pathCStrW, MAX_WIN32_PATH_LEN))
  {
    UTF16AsUTF8 pathTemp(pathCStrW);

    int s = -1;
    for (int i = 0; i < strlen(pathTemp.Get()); ++i)
    {
      if (pathTemp.Get()[i] == '\\')
      {
        s = i;
      }
    }
    if (s >= 0 && s + 1 < strlen(pathTemp.Get()))
    {
      path.Set(pathTemp.Get(), s + 1);
    }
  }
}

void HostPath(WDL_String& path, const char* bundleID)
{
  GetModulePath(0, path);
}

void PluginPath(WDL_String& path, HMODULE pExtra)
{
  GetModulePath(pExtra, path);
}

void BundleResourcePath(WDL_String& path, HMODULE pExtra)
{
#ifdef VST3_API
  GetModulePath(pExtra, path);
#ifdef ARCH_64BIT
  path.SetLen(path.GetLength() - strlen("x86_64-win/"));
#else
  path.SetLen(path.GetLength() - strlen("x86-win/"));
#endif
  path.Append("Resources\\");
#endif
}

void DesktopPath(WDL_String& path)
{
  GetKnownFolder(path, CSIDL_DESKTOP);
}

void UserHomePath(WDL_String & path)
{
  GetKnownFolder(path, CSIDL_PROFILE);
}

void AppSupportPath(WDL_String& path, bool isSystem)
{
  GetKnownFolder(path, isSystem ? CSIDL_COMMON_APPDATA : CSIDL_LOCAL_APPDATA);
}

void VST3PresetsPath(WDL_String& path, const char* mfrName, const char* pluginName, bool isSystem)
{
  if (!isSystem)
    GetKnownFolder(path, CSIDL_PERSONAL, SHGFP_TYPE_CURRENT);
  else
    AppSupportPath(path, true);

  path.AppendFormatted(MAX_WIN32_PATH_LEN, "\\VST3 Presets\\%s\\%s", mfrName, pluginName);
}

void INIPath(WDL_String& path, const char * pluginName)
{
  GetKnownFolder(path, CSIDL_LOCAL_APPDATA);

  path.AppendFormatted(MAX_WIN32_PATH_LEN, "\\%s", pluginName);
}

void WebViewCachePath(WDL_String& path)
{
  GetKnownFolder(path, CSIDL_APPDATA);
  path.Append("\\iPlug2\\WebViewCache"); // tmp
}

struct WinResourceSearch
{
  WinResourceSearch(const char* name)
  {
    UTF16ToUTF8(mName, UTF8AsUTF16(name).ToLowerCase().Get());
  }

  WDL_String mName;
  bool mFound = false;
};

static BOOL CALLBACK EnumResNameProc(HMODULE module, LPCWSTR type, LPWSTR name, LONG_PTR param)
{
  if (IS_INTRESOURCE(name))
    return true; // integer resources not wanted
  else
  {
    WinResourceSearch* search = reinterpret_cast<WinResourceSearch*>(param);

    if (search != nullptr && name != nullptr)
    {
      WDL_String searchName(search->mName);

      //strip off extra quotes
      WDL_String strippedName((UTF16AsUTF8(name).Get() + 1));
      strippedName.SetLen(strippedName.GetLength() - 1);

      // convert the stripped name to lower case (the search is already lower case)
      UTF16ToUTF8(strippedName, UTF8AsUTF16(strippedName).ToLowerCase().Get());

      if (strcmp(searchName.Get(), strippedName.Get()) == 0) // if we are looking for a resource with this name
      {
        UTF16ToUTF8(search->mName, name);
        search->mFound = true;
        return false;
      }
    }
  }

  return true; // keep enumerating
}

static UTF8AsUTF16 TypeToUpper(const char* type)
{
  return UTF8AsUTF16(type).ToUpperCase();
}

EResourceLocation LocateResource(const char* name, const char* type, WDL_String& result, const char*, void* pHInstance, const char*)
{
  if (CStringHasContents(name))
  {
    WinResourceSearch search(name);
    auto typeUpper = TypeToUpper(type);

    HMODULE hInstance = static_cast<HMODULE>(pHInstance);

    EnumResourceNamesW(hInstance, typeUpper.Get(), EnumResNameProc, (LONG_PTR) &search);

    if (search.mFound)
    {
      result.Set(search.mName.Get());
      return EResourceLocation::kWinBinary;
    }
    else
    {
      if (PathFileExistsW(UTF8AsUTF16(name).Get()))
      {
        result.Set(name);
        return EResourceLocation::kAbsolutePath;
      }
    }
  }
  return EResourceLocation::kNotFound;
}

const void* LoadWinResource(const char* resid, const char* type, int& sizeInBytes, void* pHInstance)
{
  auto typeUpper = TypeToUpper(type);

  HMODULE hInstance = static_cast<HMODULE>(pHInstance);

  HRSRC hResource = FindResourceW(hInstance, UTF8AsUTF16(resid).Get(), typeUpper.Get());

  if (!hResource)
    return NULL;

  DWORD size = SizeofResource(hInstance, hResource);

  if (size < 8)
    return NULL;

  HGLOBAL res = LoadResource(hInstance, hResource);

  const void* pResourceData = LockResource(res);

  if (!pResourceData)
  {
    sizeInBytes = 0;
    return NULL;
  }
  else
  {
    sizeInBytes = size;
    return pResourceData;
  }
}

#elif defined OS_WEB
#pragma mark - OS_WEB

void AppSupportPath(WDL_String& path, bool isSystem)
{
  path.Set("Settings");
}

void DesktopPath(WDL_String& path)
{
  path.Set("");
}

void VST3PresetsPath(WDL_String& path, const char* mfrName, const char* pluginName, bool isSystem)
{
  path.Set("Presets");
}

EResourceLocation LocateResource(const char* name, const char* type, WDL_String& result, const char*, void*, const char*)
{
  if (CStringHasContents(name))
  {
    WDL_String plusSlash;
    WDL_String path(name);
    const char* file = path.get_filepart();

    bool foundResource = false;

    //TODO: FindResource is not sufficient here

    if(strcmp(type, "png") == 0) { //TODO: lowercase/uppercase png
      plusSlash.SetFormatted(strlen("/resources/img/") + strlen(file) + 1, "/resources/img/%s", file);
      foundResource = emscripten::val::global("Browser")["preloadedImages"].call<bool>("hasOwnProperty", std::string(plusSlash.Get()));
    }
    else if(strcmp(type, "ttf") == 0) { //TODO: lowercase/uppercase ttf
      plusSlash.SetFormatted(strlen("/resources/fonts/") + strlen(file) + 1, "/resources/fonts/%s", file);
      foundResource = true; // TODO: check ttf
    }
    else if(strcmp(type, "svg") == 0) { //TODO: lowercase/uppercase svg
      plusSlash.SetFormatted(strlen("/resources/img/") + strlen(file) + 1, "/resources/img/%s", file);
      foundResource = true; // TODO: check svg
    }

    if(foundResource)
    {
      result.Set(plusSlash.Get());
      return EResourceLocation::kAbsolutePath;
    }
  }
  return EResourceLocation::kNotFound;
}

#elif defined OS_LINUX
#pragma mark - OS_LINUX

void* GetEmbedListPtr()
{
  Dl_info info;
  auto codePtr = reinterpret_cast<const void*>(&GetEmbedListPtr);
  if (dladdr(codePtr, &info) == 0) return nullptr;
  void* hDll = dlopen(info.dli_fname, RTLD_LOCAL);
  if (!hDll) return nullptr;
  void* hEmbedList = dlsym(hDll, "EMBED_LIST");
  return hEmbedList;
}

// Get the file path for the module where specified symbol is defined
static bool GetFileNameFor(void* code, char* path, int size)
{
  Dl_info info;

  if(!code || !path || !size)
  {
    return false;
  }

  if(!dladdr(code, &info) || (strlen(info.dli_fname) + 1 > size))
  {
    *path = 0; // the path is too long
    return false;
  }

  strcpy(path, info.dli_fname);
  return true;
}

EResourceLocation LocateResource(const char* name, const char* type, WDL_String& result, const char*, void* pHInstance, const char*)
{
  char path[PATH_MAX];
  if (CStringHasContents(name) && GetFileNameFor(reinterpret_cast<void*>(GetFileNameFor), path, PATH_MAX))
  {
    for (char *s = path + strlen(path) - 1; s >= path; --s)
    {
      if(*s == WDL_DIRCHAR)
      {
        *s = 0;
        break;
      }
    }

    const char *subdir = "";
    if(strcmp(type, "png") == 0) //TODO: lowercase/uppercase png
      subdir = "img";
    else if(strcmp(type, "ttf") == 0) //TODO: lowercase/uppercase ttf
      subdir = "fonts";
    else if(strcmp(type, "svg") == 0) //TODO: lowercase/uppercase svg
      subdir = "img";

    result.SetFormatted(PATH_MAX, "%s%s%s%s%s", path, WDL_DIRCHAR_STR "resources" WDL_DIRCHAR_STR, subdir, WDL_DIRCHAR_STR, name);
    struct stat st;

    if (!stat(result.Get(), &st))
      return EResourceLocation::kAbsolutePath;

#ifdef VST3_API
    // VST3 bundle since 3.6.10
    result.SetFormatted(PATH_MAX, "%s%s%s%s%s", path, WDL_DIRCHAR_STR ".." WDL_DIRCHAR_STR "Resources" WDL_DIRCHAR_STR, subdir, WDL_DIRCHAR_STR, name);
    if (!stat(result.Get(), &st))
      return EResourceLocation::kAbsolutePath;
#endif
  }
  return EResourceLocation::kNotFound;
}

std::shared_ptr<Resource> LoadRcResource(const char* fileOrResId, const char* type)
{
  // No passed name? Return an empty shared_ptr.
  if (!CStringHasContents(fileOrResId)) return nullptr;

  // Try to load an embedded resource
  const void* pData = nullptr;
  size_t dataSize = 0;
  if (FindEmbedResource(fileOrResId, &pData, &dataSize))
  {
    return Resource::fromBuffer(pData, dataSize);
  }

  WDL_String fullPath;
  const EResourceLocation location = LocateResource(fileOrResId, type, fullPath, "", nullptr, nullptr);
  if ((location == kNotFound) || (location != kAbsolutePath) )
  {
    return nullptr;
  }
  // Load from a file
  return Resource::fromFile(fullPath.Get());
}

bool AppIsSandboxed()
{
  return false;
}

void AppSupportPath(WDL_String& path, bool isSystem)
{
  path.Set("~");
}

void SandboxSafeAppSupportPath(WDL_String& path, const char* appGroupID)
{
  AppSupportPath(path);
}

void DesktopPath(WDL_String& path)
{
  path.Set("");
}

void VST3PresetsPath(WDL_String& path, const char* mfrName, const char* pluginName, bool isSystem)
{
  path.Set("~/Presets");
}

#endif

END_IPLUG_NAMESPACE
