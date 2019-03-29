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

// Unicode helpers
void UTF8ToUTF16(wchar_t* utf16Str, const char* utf8Str, int maxLen)
{
  int requiredSize = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, NULL, 0);

  if (requiredSize > 0 && requiredSize <= maxLen)
  {
    MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, utf16Str, requiredSize);
    return;
  }

  utf16Str[0] = 0;
}

void UTF16ToUTF8(WDL_String& utf8Str, const wchar_t* utf16Str)
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
void GetKnownFolder(WDL_String &path, int identifier, int flags = 0)
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

void HostPath(WDL_String& path, const char* bundleID)
{
  GetModulePath(0, path);
}

void PluginPath(WDL_String& path, void* pExtra)
{
  GetModulePath((HMODULE) pExtra, path);
}

void BundleResourcePath(WDL_String& path, void* pExtra)
{
#ifdef VST3_API
  GetModulePath((HMODULE)pExtra, path);
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

void SandboxSafeAppSupportPath(WDL_String& path)
{
  AppSupportPath(path);
}

void INIPath(WDL_String& path, const char * pluginName)
{
  GetKnownFolder(path, CSIDL_LOCAL_APPDATA);

  path.AppendFormatted(MAX_WIN32_PATH_LEN, "\\%s", pluginName);
}

#elif defined OS_WEB

void AppSupportPath(WDL_String& path, bool isSystem)
{
  path.Set("Settings");
}

void SandboxSafeAppSupportPath(WDL_String& path)
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

#endif
