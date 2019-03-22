/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

/**
 * @file
 * @brief Common paths useful for plug-ins
 */

#include "string.h"
#include "wdlstring.h"



#ifdef OS_WIN
#include <windows.h>
 // Unicode helpers
void UTF8ToUTF16(wchar_t* utf16Str, const char* utf8Str, int maxLen);
void UTF16ToUTF8(WDL_String& utf8Str, const wchar_t* utf16Str);
#endif

/** @param path WDL_String reference where the path will be put on success or empty string on failure */
extern void HostPath(WDL_String& path, const char* bundleID = 0);

/** @param path WDL_String reference where the path will be put on success or empty string on failure
 *  @param pExtra This should either be a const char* to bundleID (macOS) or an HMODULE handle (windows) */
extern void PluginPath(WDL_String& path, void* pExtra);

/** @param path WDL_String reference where the path will be put on success or empty string on failure
 *  @param pExtra This should either be a const char* to bundleID (macOS) or an HMODULE handle (windows) */
extern void BundleResourcePath(WDL_String& path, void* pExtra = 0);

/** @param path WDL_String reference where the path will be put on success or empty string on failure */
extern void DesktopPath(WDL_String& path);

/** @param path WDL_String reference where the path will be put on success or empty string on failure */
extern void UserHomePath(WDL_String& path);

/** @param path WDL_String reference where the path will be put on success or empty string on failure
 * @param isSystem Set \c true if you want to obtain the system-wide path, otherwise the path will be in the user's home folder */
extern void AppSupportPath(WDL_String& path, bool isSystem = false);

/** @param path WDL_String reference where the path will be put on success or empty string on failure */
extern void SandboxSafeAppSupportPath(WDL_String& path);

/** @param path WDL_String reference where the path will be put on success or empty string on failure
 * @param mfrName CString to specify the manufacturer name, which will be the top level folder for .vstpreset files for this manufacturer's product
 * @param pluginName CString to specify the plug-in name, which will be the sub folder (beneath mfrName) in which the .vstpreset files are located
 * @param isSystem Set \c true if you want to obtain the system-wide path, otherwise the path will be in the user's home folder */
extern void VST3PresetsPath(WDL_String& path, const char* mfrName, const char* pluginName, bool isSystem = true);

/** Get the path to the folder where the App's settings.ini file is stored
 * @param path WDL_String reference where the path will be put on success or empty string on failure
 * @param pluginName CString to specify the plug-in name (BUNDLE_NAME from config.h can be used here) */
extern void INIPath(WDL_String& path, const char * pluginName);