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
#else

void VST3PresetsPath(WDL_String& path, const char* mfrName, const char* pluginName, bool isSystem)
{
//  NSArray* pPaths;
//  if (isSystem)
//    pPaths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSLocalDomainMask, YES);
//  else
//    pPaths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
//  
//  NSString* pApplicationSupportDirectory = [pPaths objectAtIndex:0];
//  path.SetFormatted(MAX_MACOS_PATH_LEN, "%s/Audio/Presets/%s/%s/", [pApplicationSupportDirectory UTF8String], mfrName, pluginName);
}

#endif
