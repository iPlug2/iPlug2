/*
 ==============================================================================
 
 This file is part of the iPlug 2 library
 
 Oli Larkin et al. 2018 - https://www.olilarkin.co.uk
 
 iPlug 2 is an open source library subject to commercial or open-source
 licensing.
 
 The code included in this file is provided under the terms of the WDL license
 - https://www.cockos.com/wdl/
 
 ==============================================================================
 */

#pragma once
#include "string.h"
#include "wdlstring.h"

/** @param path WDL_String reference where the path will be put on success or empty string on failure */
extern void HostPath(WDL_String& path, const char* bundleID = 0);

/** @param path WDL_String reference where the path will be put on success or empty string on failure */
extern void PluginPath(WDL_String& path, const char* bundleID = 0);

/** @param path WDL_String reference where the path will be put on success or empty string on failure */
extern void BundleResourcePath(WDL_String& path, const char* bundleID = 0);

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
 * @param mfrName CString to specify the manfucturer name, which will be the top level folder for .vstpreset files for this manufacturer's product
 * @param pluginName CString to specify the plug-in name, which will be the sub folder (beneath mfrName) in which the .vstpreset files are located
 * @param isSystem Set \c true if you want to obtain the system-wide path, otherwise the path will be in the user's home folder */
extern void VST3PresetsPath(WDL_String& path, const char* mfrName, const char* pluginName, bool isSystem = true);
