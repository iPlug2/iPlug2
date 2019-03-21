/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

/**
 * @file
 * @brief IPlugPaths implementation for macOS and iOS
 */

#include "IPlugPlatform.h"
#include "IPlugPaths.h"
#include "IPlugConstants.h"

#ifdef OS_MAC

void HostPath(WDL_String& path, const char* bundleID)
{
  //  CocoaAutoReleasePool pool; //TODO:
  NSBundle* pBundle = [NSBundle bundleWithIdentifier: [NSString stringWithCString:bundleID encoding:NSUTF8StringEncoding]];
  
  if (pBundle)
  {
    NSString* pPath = [pBundle executablePath];
    if (pPath)
    {
      path.Set([pPath UTF8String]);
    }
  }
}

void PluginPath(WDL_String& path, const char* bundleID)
{
//  CocoaAutoReleasePool pool; //TODO:
  NSBundle* pBundle = [NSBundle bundleWithIdentifier: [NSString stringWithCString:bundleID encoding:NSUTF8StringEncoding]];
  
  if (pBundle)
  {
    NSString* pPath = [[pBundle bundlePath] stringByDeletingLastPathComponent];
    
    if (pPath)
    {
      path.Set([pPath UTF8String]);
      path.Append("/");
    }
  }
}

void BundleResourcePath(WDL_String& path, const char* bundleID)
{
//  CocoaAutoReleasePool pool;
  
  NSBundle* pBundle = [NSBundle bundleWithIdentifier:[NSString stringWithCString:bundleID encoding:NSUTF8StringEncoding]];
  NSString* pResPath = [pBundle resourcePath];
  
  path.Set([pResPath UTF8String]);
}

void DesktopPath(WDL_String& path)
{
  NSArray* pPaths = NSSearchPathForDirectoriesInDomains(NSDesktopDirectory, NSUserDomainMask, YES);
  NSString* pDesktopDirectory = [pPaths objectAtIndex:0];
  path.Set([pDesktopDirectory UTF8String]);
}

void UserHomePath(WDL_String& path)
{
  NSString* pHomeDir = NSHomeDirectory();
  path.Set([pHomeDir UTF8String]);
}

void VST3PresetsPath(WDL_String& path, const char* mfrName, const char* pluginName, bool isSystem)
{
  NSArray* pPaths;
  if (isSystem)
    pPaths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSLocalDomainMask, YES);
  else
    pPaths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
  
  NSString* pApplicationSupportDirectory = [pPaths objectAtIndex:0];
  path.SetFormatted(MAX_MACOS_PATH_LEN, "%s/Audio/Presets/%s/%s/", [pApplicationSupportDirectory UTF8String], mfrName, pluginName);
}

void INIPath(WDL_String& path, const char* pluginName)
{
  AppSupportPath(path, false);
  path.AppendFormatted(MAX_MACOS_PATH_LEN, "/%s", pluginName);
}

void AppSupportPath(WDL_String& path, bool isSystem)
{
  NSArray *pPaths;
  
  if (isSystem)
    pPaths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSSystemDomainMask, YES);
  else
    pPaths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
  
  NSString *pApplicationSupportDirectory = [pPaths objectAtIndex:0];
  path.Set([pApplicationSupportDirectory UTF8String]);
}

void SandboxSafeAppSupportPath(WDL_String& path)
{
  NSArray *pPaths = NSSearchPathForDirectoriesInDomains(NSMusicDirectory, NSUserDomainMask, YES);
  NSString *pUserMusicDirectory = [pPaths objectAtIndex:0];
  path.Set([pUserMusicDirectory UTF8String]);
}

#elif defined OS_IOS

void AppSupportPath(WDL_String& path, bool isSystem)
{
}

void SandboxSafeAppSupportPath(WDL_String& path)
{
}

void DesktopPath(WDL_String& path)
{
  
}

void VST3PresetsPath(WDL_String& path, const char* mfrName, const char* pluginName, bool isSystem)
{
  path.Set("");
}

void INIPath(WDL_String& path, const char* pluginName)
{
  path.Set("");
}

#endif
