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

#ifdef OS_WIN
#include <windows.h>
#include <Shlobj.h>
#include <Shlwapi.h>

using namespace iplug;

// Unicode helpers
void iplug::UTF8ToUTF16(wchar_t* utf16Str, const char* utf8Str, int maxLen)
{
  int requiredSize = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, NULL, 0);

  if (requiredSize > 0 && requiredSize <= maxLen)
  {
    MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, utf16Str, requiredSize);
    return;
  }

  utf16Str[0] = 0;
}

void iplug::UTF16ToUTF8(WDL_String& utf8Str, const wchar_t* utf16Str)
{
  int requiredSize = WideCharToMultiByte(CP_UTF8, 0, utf16Str, -1, NULL, 0, NULL, NULL);

  if (requiredSize > 0 && utf8Str.SetLen(requiredSize))
  {
    WideCharToMultiByte(CP_UTF8, 0, utf16Str, -1, utf8Str.Get(), requiredSize, NULL, NULL);
    return;
  }

  utf8Str.Set("");
}

 // Helper for getting a known folder in UTF8
void iplug::GetKnownFolder(WDL_String &path, int identifier, int flags)
{
  wchar_t wideBuffer[1024];

  SHGetFolderPathW(NULL, identifier, NULL, flags, wideBuffer);
  UTF16ToUTF8(path, wideBuffer);
}

static void GetModulePath(HMODULE hModule, WDL_String& path)
{
  path.Set("");
  char pathCStr[MAX_WIN32_PATH_LEN];
  pathCStr[0] = '\0';
  if (GetModuleFileName(hModule, pathCStr, MAX_WIN32_PATH_LEN))
  {
    int s = -1;
    for (int i = 0; i < strlen(pathCStr); ++i)
    {
      if (pathCStr[i] == '\\')
      {
        s = i;
      }
    }
    if (s >= 0 && s + 1 < strlen(pathCStr))
    {
      path.Set(pathCStr, s + 1);
    }
  }
}

void iplug::HostPath(WDL_String& path, const char* bundleID)
{
  GetModulePath(0, path);
}

void iplug::PluginPath(WDL_String& path, HMODULE pExtra)
{
  GetModulePath(pExtra, path);
}

void iplug::BundleResourcePath(WDL_String& path, HMODULE pExtra)
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

void iplug::DesktopPath(WDL_String& path)
{
  GetKnownFolder(path, CSIDL_DESKTOP);
}

void iplug::UserHomePath(WDL_String & path)
{
  GetKnownFolder(path, CSIDL_PROFILE);
}

void iplug::AppSupportPath(WDL_String& path, bool isSystem)
{
  GetKnownFolder(path, isSystem ? CSIDL_COMMON_APPDATA : CSIDL_LOCAL_APPDATA);
}

void iplug::VST3PresetsPath(WDL_String& path, const char* mfrName, const char* pluginName, bool isSystem)
{
  if (!isSystem)
    GetKnownFolder(path, CSIDL_PERSONAL, SHGFP_TYPE_CURRENT);
  else
    AppSupportPath(path, true);
  
  path.AppendFormatted(MAX_WIN32_PATH_LEN, "\\VST3 Presets\\%s\\%s", mfrName, pluginName);
}

void iplug::SandboxSafeAppSupportPath(WDL_String& path, const char* appGroupID)
{
  AppSupportPath(path);
}

void iplug::INIPath(WDL_String& path, const char * pluginName)
{
  GetKnownFolder(path, CSIDL_LOCAL_APPDATA);

  path.AppendFormatted(MAX_WIN32_PATH_LEN, "\\%s", pluginName);
}

static BOOL iplug::EnumResNameProc(HANDLE module, LPCTSTR type, LPTSTR name, LONG_PTR param)
{
  if (IS_INTRESOURCE(name)) return true; // integer resources not wanted
  else {
    WDL_String* search = (WDL_String*)param;
    if (search != 0 && name != 0)
    {
      //strip off extra quotes
      WDL_String strippedName(strlwr(name + 1));
      strippedName.SetLen(strippedName.GetLength() - 1);

      if (strcmp(strlwr(search->Get()), strippedName.Get()) == 0) // if we are looking for a resource with this name
      {
        search->SetFormatted(strippedName.GetLength() + 7, "found: %s", strippedName.Get());
        return false;
      }
    }
  }

  return true; // keep enumerating
}

EResourceLocation iplug::LocateResource(const char* name, const char* type, WDL_String& result, const char*, void* pHInstance)
{
  if (CStringHasContents(name))
  {
    WDL_String search(name);
    WDL_String typeUpper(type);

    HMODULE hInstance = static_cast<HMODULE>(pHInstance);

    EnumResourceNames(hInstance, _strupr(typeUpper.Get()), (ENUMRESNAMEPROC)EnumResNameProc, (LONG_PTR)&search);

    if (strstr(search.Get(), "found: ") != 0)
    {
      result.SetFormatted(MAX_PATH, "\"%s\"", search.Get() + 7, search.GetLength() - 7); // 7 = strlen("found: ")
      return EResourceLocation::kWinBinary;
    }
    else
    {
      if (PathFileExists(name))
      {
        result.Set(name);
        return EResourceLocation::kAbsolutePath;
      }
    }
  }
  return EResourceLocation::kNotFound;
}

const void* iplug::LoadWinResource(const char* resid, const char* type, int& sizeInBytes, void* pHInstance)
{
  WDL_String typeUpper(type);

  HMODULE hInstance = static_cast<HMODULE>(pHInstance);

  HRSRC hResource = FindResource(hInstance, resid, _strupr(typeUpper.Get()));

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

void iplug::AppSupportPath(WDL_String& path, bool isSystem)
{
  path.Set("Settings");
}

void iplug::SandboxSafeAppSupportPath(WDL_String& path)
{
  path.Set("Settings");
}

void iplug::DesktopPath(WDL_String& path)
{
  path.Set("");
}

void iplug::VST3PresetsPath(WDL_String& path, const char* mfrName, const char* pluginName, bool isSystem)
{
  path.Set("Presets");
}

#include <emscripten/val.h>

using namespace iplug;
using namespace emscripten;

EResourceLocation iplug::LocateResource(const char* name, const char* type, WDL_String& result, const char*, void*)
{
  if (CStringHasContents(name))
  {
    WDL_String plusSlash;
    
    bool foundResource = false;
    
    //TODO: FindResource is not sufficient here
    
    if(strcmp(type, "png") == 0) { //TODO: lowercase/uppercase png
      plusSlash.SetFormatted(strlen("/resources/img/") + strlen(name) + 1, "/resources/img/%s", name);
      foundResource = val::global("Module")["preloadedImages"].call<bool>("hasOwnProperty", std::string(plusSlash.Get()));
    }
    else if(strcmp(type, "ttf") == 0) { //TODO: lowercase/uppercase ttf
      plusSlash.SetFormatted(strlen("/resources/fonts/") + strlen(name) + 1, "/resources/fonts/%s", name);
      foundResource = true; // TODO: check ttf
    }
    else if(strcmp(type, "svg") == 0) { //TODO: lowercase/uppercase svg
      plusSlash.SetFormatted(strlen("/resources/img/") + strlen(name) + 1, "/resources/img/%s", name);
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

#endif
