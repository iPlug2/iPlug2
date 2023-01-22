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


#include "IPlugPaths.h"
#include <string>
#include <map>

#if defined(OS_IOS) || defined(OS_MAC)
#import <Foundation/Foundation.h>
#include <TargetConditionals.h>
#endif

#ifdef IGRAPHICS_METAL
extern std::map<std::string, void*> gTextureMap;
#endif

BEGIN_IPLUG_NAMESPACE

#ifdef OS_MAC
#pragma mark - macOS

void HostPath(WDL_String& path, const char* bundleID)
{
  @autoreleasepool
  {
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
}

void PluginPath(WDL_String& path, PluginIDType bundleID)
{
  @autoreleasepool
  {
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
}

void BundleResourcePath(WDL_String& path, PluginIDType bundleID)
{
  @autoreleasepool
  {
    NSBundle* pBundle = [NSBundle bundleWithIdentifier:[NSString stringWithCString:bundleID encoding:NSUTF8StringEncoding]];
    NSString* pResPath = [pBundle resourcePath];
    
    path.Set([pResPath UTF8String]);
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

void INIPath(WDL_String& path, const char* pluginName)
{
  AppSupportPath(path, false);
  path.AppendFormatted(MAX_MACOS_PATH_LEN, "/%s", pluginName);
}

void AppSupportPath(WDL_String& path, bool isSystem)
{
  NSArray* pPaths;
  
  if (isSystem)
    pPaths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSSystemDomainMask, YES);
  else
    pPaths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
  
  NSString *pApplicationSupportDirectory = [pPaths objectAtIndex:0];
  path.Set([pApplicationSupportDirectory UTF8String]);
}

void AppGroupContainerPath(WDL_String& path, const char* appGroupID)
{
  NSFileManager* mgr = [NSFileManager defaultManager];
  NSURL* url = [mgr containerURLForSecurityApplicationGroupIdentifier:[NSString stringWithUTF8String:appGroupID]];
  path.Set([[url path] UTF8String]);
}

void SharedMusicPath(WDL_String& path)
{
  NSArray* pPaths = NSSearchPathForDirectoriesInDomains(NSMusicDirectory, NSUserDomainMask, YES);
  NSString* pUserMusicDirectory = [pPaths objectAtIndex:0];
  path.Set([pUserMusicDirectory UTF8String]);
}

bool GetResourcePathFromBundle(const char* fileName, const char* searchExt, WDL_String& fullPath, const char* bundleID)
{
  @autoreleasepool
  {
    const char* ext = fileName+strlen(fileName)-1;
    while (ext >= fileName && *ext != '.') --ext;
    ++ext;
    
    bool isCorrectType = !strcasecmp(ext, searchExt);
    
    bool isAppExtension = IsOOPAuv3AppExtension();
    
    NSBundle* pBundle = [NSBundle bundleWithIdentifier:[NSString stringWithCString:bundleID encoding:NSUTF8StringEncoding]];
    
    NSString* pFile = [[NSString stringWithCString:fileName encoding:NSUTF8StringEncoding] stringByDeletingPathExtension];
    NSString* pExt = [NSString stringWithCString:searchExt encoding:NSUTF8StringEncoding];
    
    if (isCorrectType && pBundle && pFile)
    {
      NSString* pBasePath;
      
      if(isAppExtension)
        pBasePath = [[[[pBundle bundlePath] stringByDeletingLastPathComponent] stringByDeletingLastPathComponent] stringByAppendingString:@"/Resources/"];
      else
        pBasePath = [[pBundle resourcePath] stringByAppendingString:@"/"];
      
      NSString* pPath = [[[pBasePath stringByAppendingString:pFile] stringByAppendingString: @"."] stringByAppendingString:pExt];
      
      if (pPath && [[NSFileManager defaultManager] fileExistsAtPath : pPath] == YES)
      {
        fullPath.Set([pPath cStringUsingEncoding:NSUTF8StringEncoding]);
        return true;
      }
    }
    
    fullPath.Set("");
    return false;
  }
}

bool GetResourcePathFromSharedLocation(const char* fileName, const char* searchExt, WDL_String& fullPath, const char* subfolder)
{
  @autoreleasepool
  {
    const char* ext = fileName+strlen(fileName)-1;
    while (ext >= fileName && *ext != '.') --ext;
    ++ext;

    bool isCorrectType = !strcasecmp(ext, searchExt);

    NSString* pExt = [NSString stringWithCString:searchExt encoding:NSUTF8StringEncoding];
    NSString* pFile = [[[NSString stringWithCString:fileName encoding:NSUTF8StringEncoding] lastPathComponent] stringByDeletingPathExtension];
      
    if (isCorrectType && pFile)
    {
      WDL_String musicPath;
      SharedMusicPath(musicPath);

      if(subfolder)
      {
        NSString* pPluginName = [NSString stringWithCString: subfolder encoding:NSUTF8StringEncoding];
        NSString* pMusicLocation = [NSString stringWithCString: musicPath.Get() encoding:NSUTF8StringEncoding];
        NSString* pPath = [[[[pMusicLocation stringByAppendingPathComponent:pPluginName] stringByAppendingPathComponent:@"Resources"] stringByAppendingPathComponent: pFile] stringByAppendingPathExtension:pExt];

        if([[NSFileManager defaultManager] fileExistsAtPath : pPath] == YES)
        {
          fullPath.Set([pPath cString]);
          return true;
        }
      }
    }

    fullPath.Set("");
    return false;
  }
}

EResourceLocation LocateResource(const char* name, const char* type, WDL_String& result, const char* bundleID, void*, const char* sharedResourcesSubPath)
{
  if(CStringHasContents(name))
  {
#ifndef AUv3_API
    // first check this bundle
    if(GetResourcePathFromBundle(name, type, result, bundleID))
      return EResourceLocation::kAbsolutePath;
#endif
    
    // then check ~/Music/sharedResourcesSubPath, which is a shared folder that can be accessed from app sandbox
    if(GetResourcePathFromSharedLocation(name, type, result, sharedResourcesSubPath))
      return EResourceLocation::kAbsolutePath;
    
    // finally check name, which might be a full path - if the plug-in is trying to load a resource at runtime (e.g. skin-able UI)
    NSString* pPath = [NSString stringWithCString:name encoding:NSUTF8StringEncoding];
    
    if([[NSFileManager defaultManager] fileExistsAtPath : pPath] == YES)
    {
      result.Set([pPath UTF8String]);
      return EResourceLocation::kAbsolutePath;
    }
  }
  return EResourceLocation::kNotFound;
}

bool AppIsSandboxed()
{
  NSString* pHomeDir = NSHomeDirectory();
  
  if ([pHomeDir containsString:@"Library/Containers/"])
    return true;
  else
    return false;
}

bool IsXPCAuHost()
{
  return ([[[NSProcessInfo processInfo] processName] containsString:@"XPC"]);
}

bool IsOOPAuv3AppExtension()
{
  return ([[[NSBundle mainBundle] bundleIdentifier] containsString:@"AUv3"]);
}

#elif defined OS_IOS
#pragma mark - iOS

void HostPath(WDL_String& path, const char* bundleID)
{
}

void PluginPath(WDL_String& path, PluginIDType bundleID)
{
}

void BundleResourcePath(WDL_String& path, PluginIDType bundleID)
{
  NSBundle* pBundle = [NSBundle mainBundle];
    
  if(IsOOPAuv3AppExtension())
    pBundle = [NSBundle bundleWithPath: [[[pBundle bundlePath] stringByDeletingLastPathComponent] stringByDeletingLastPathComponent]];

  path.Set([[pBundle resourcePath] UTF8String]);
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

void AppSupportPath(WDL_String& path, bool isSystem)
{
}

void AppGroupContainerPath(WDL_String& path, const char* appGroupID)
{
  NSFileManager* mgr = [NSFileManager defaultManager];
  NSURL* url = [mgr containerURLForSecurityApplicationGroupIdentifier:[NSString stringWithUTF8String:appGroupID]];
  path.Set([[url path] UTF8String]);
}

bool GetResourcePathFromBundle(const char* fileName, const char* searchExt, WDL_String& fullPath, const char* bundleID)
{
  @autoreleasepool
  {
    const char* ext = fileName+strlen(fileName)-1;
    while (ext >= fileName && *ext != '.') --ext;
    ++ext;
    
    bool isCorrectType = !strcasecmp(ext, searchExt);
    
    bool isAppExtension = IsOOPAuv3AppExtension();
    
    NSBundle* pBundle;
    
    if(isAppExtension)
      pBundle = [NSBundle bundleWithIdentifier:[NSString stringWithCString:bundleID encoding:NSUTF8StringEncoding]];
    else
      pBundle = [NSBundle mainBundle];

    NSString* pFile = [[NSString stringWithCString:fileName encoding:NSUTF8StringEncoding] stringByDeletingPathExtension];
    NSString* pExt = [NSString stringWithCString:searchExt encoding:NSUTF8StringEncoding];
    
    if (isCorrectType && pBundle && pFile)
    {
      NSString* pRootPath;
      
      if (isAppExtension)
      {
        pRootPath = [[[pBundle bundlePath] stringByDeletingLastPathComponent] stringByDeletingLastPathComponent];
        #if TARGET_OS_MACCATALYST
        pRootPath = [pRootPath stringByAppendingString: @"/Resources"];
        #endif
      }
      else
      {
        pRootPath = [pBundle resourcePath];
      }
      
      NSString* pPath = [[[[pRootPath stringByAppendingString:@"/"] stringByAppendingString:pFile] stringByAppendingString: @"."] stringByAppendingString:pExt];
      
      if (pPath && [[NSFileManager defaultManager] fileExistsAtPath : pPath] == YES)
      {
        fullPath.Set([pPath cStringUsingEncoding:NSUTF8StringEncoding]);
        return true;
      }
    }
    
    fullPath.Set("");
    return false;
  }
}

EResourceLocation LocateResource(const char* name, const char* type, WDL_String& result, const char* bundleID, void*, const char*)
{
  if (CStringHasContents(name))
  {
#ifdef IGRAPHICS_METAL
    auto itr = gTextureMap.find(name);
    
    if (itr != gTextureMap.end())
    {
      result.Set(name);
      return EResourceLocation::kPreloadedTexture;
    }
#endif
    
    if (GetResourcePathFromBundle(name, type, result, bundleID))
      return EResourceLocation::kAbsolutePath;
  }
  
  return EResourceLocation::kNotFound;
}

bool AppIsSandboxed()
{
  return true;
}

bool IsXPCAuHost()
{
  // TODO: actually not allways the case if AUv3 is instantiated from framework
  return true;
}

bool IsOOPAuv3AppExtension()
{
  return ([[[NSBundle mainBundle] bundleIdentifier] containsString:@"AUv3"]);
}

#endif

END_IPLUG_NAMESPACE
