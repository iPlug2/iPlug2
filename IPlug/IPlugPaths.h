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

/** @param path WDL_String reference where the path will be put on success or empty string on failure */
extern void HostPath(WDL_String& path, const char* bundleID = 0);

/** @param path WDL_String reference where the path will be put on success or empty string on failure */
extern void PluginPath(WDL_String& path, const char* bundleID = 0);

/** @param path WDL_String reference where the path will be put on success or empty string on failure */
extern void BundleResourcePath(WDL_String& path, const char* bundleID);

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
