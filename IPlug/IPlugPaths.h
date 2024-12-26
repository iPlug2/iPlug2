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

#include "IPlugUtilities.h"

BEGIN_IPLUG_NAMESPACE

#if defined OS_MAC || defined OS_IOS
using PluginIDType = const char *;
#elif defined OS_WIN
using PluginIDType = HMODULE;
#else
using PluginIDType = void *;
#endif

/** Get the path to the host binary 
* @param path WDL_String reference where the path will be put on success or empty string on failure */
extern void HostPath(WDL_String& path, const char* bundleID = 0);

/** Get the path to the plug-in binary 
 * @param path WDL_String reference where the path will be put on success or empty string on failure
 * @param pExtra This should either be a const char* to bundleID (macOS) or an HMODULE handle (windows) */
extern void PluginPath(WDL_String& path, PluginIDType pExtra);

/** Get the path to the plug-in bundle resource path. On macOS and iOS if this is called in an AUv3 app extension it will return the bundle of the parent app
 * iOS bundles are flat, so the path is just to the .app where as macOS bundles contain a resources subfolder
 * On Windows this is only useful for VST3 plug-ins which have a "bundle" with a resource path since v3.6
 * @param path WDL_String reference where the path will be put on success or empty string on failure
 * @param pExtra This should either be a const char* to bundleID (macOS/iOS) or an HMODULE handle (windows) */
extern void BundleResourcePath(WDL_String& path, PluginIDType pExtra = 0);

/** @param path WDL_String reference where the path will be put on success or empty string on failure */
extern void DesktopPath(WDL_String& path);

/** @param path WDL_String reference where the path will be put on success or empty string on failure */
extern void UserHomePath(WDL_String& path);

/** @param path WDL_String reference where the path will be put on success or empty string on failure
 * @param isSystem Set \c true if you want to obtain the system-wide path, otherwise the path will be in the user's home folder */
extern void AppSupportPath(WDL_String& path, bool isSystem = false);

/** @param path WDL_String reference where the path will be put on success or empty string on failure
 * @param mfrName CString to specify the manufacturer name, which will be the top level folder for .vstpreset files for this manufacturer's product
 * @param pluginName CString to specify the plug-in name, which will be the sub folder (beneath mfrName) in which the .vstpreset files are located
 * @param isSystem Set \c true if you want to obtain the system-wide path, otherwise the path will be in the user's home folder */
extern void VST3PresetsPath(WDL_String& path, const char* mfrName, const char* pluginName, bool isSystem = true);

/** Get the path to the folder where the App's settings.ini file is stored
 * @param path WDL_String reference where the path will be put on success or empty string on failure
 * @param pluginName CString to specify the plug-in name (BUNDLE_NAME from config.h can be used here) */
extern void INIPath(WDL_String& path, const char* pluginName);

/** Get the path to the folder where the Plug-in's ICoreWebView2 userdata folder should be (Windows WebView only)*/
extern void WebViewCachePath(WDL_String& path);

/** Find the absolute path of a resource based on it's file name (e.g. “background.png”) and type (e.g. “png”), or in the case of windows,
 * confirm the existence of a particular resource in the binary. If it fails to find the resource with the binary it will test the fileNameOrResID argument
 * as an absolute path, to see if the file exists in that place.
 * On macOS resources are usually included inside the bundle resources folder.
 * On Windows resources are usually baked into the binary via the resource compiler. In this case the fileName argument is the resource id to look for.
 * The .rc file must include these ids, otherwise you may hit a runtime assertion when you come to load the file.
 * In some cases you may want to provide an absolute path to a file in a shared resources folder
 * here (for example if you want to reduce the disk footprint of multiple bundles, such as when you have multiple plug-in formats installed).
 *
 * @param fileNameOrResID The filename or resourceID including extension. If no resource is found this argument is tested as an absolute path.
 * @param type The resource type (file extension) in lower or upper case, e.g. ttf or TTF for a truetype font
 * @param result WDL_String which will either contain the full path to the resource on disk, or the full Windows resourceID on success
 * @return \c true on success */
extern EResourceLocation LocateResource(const char* fileNameOrResID, const char* type, WDL_String& result, const char* bundleID, void* pHInstance, const char* sharedResourcesSubPath);

/** Load a resource from the binary (windows only).
 * @param type The resource type in lower or upper case, e.g. ttf or TTF for a truetype font
 * @return const void pointer to the data if successfull on windows. Returns nullptr if unsuccessfull or on platforms other than windows */
extern const void* LoadWinResource(const char* resID, const char* type, int& sizeInBytes, void* pHInstance);

#if defined OS_MAC || defined OS_IOS

/** @return \c true if the app is sandboxed (and therefore file access etc is restricted) */
extern bool AppIsSandboxed();

/** @param path WDL_String reference where the path will be put on success or empty string on failure */
extern void AppGroupContainerPath(WDL_String& path, const char* appGroupID);

/** @param path WDL_String reference where the path will be put on success or empty string on failure */
extern void SharedMusicPath(WDL_String& path);

/** @return \c true if XPC AUv2 host (Typically Logic/Garageband on Apple Silicon)  */
extern bool IsXPCAuHost();

/** @return \c true if in an out-of-process AUv3  */
extern bool IsOOPAuv3AppExtension();

#endif

END_IPLUG_NAMESPACE

