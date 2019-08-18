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

#if defined OS_IOS
#import <Foundation/Foundation.h>
#endif

#ifdef IGRAPHICS_METAL
extern std::map<std::string, void*> gTextureMap;
#endif

BEGIN_IPLUG_NAMESPACE

#ifdef OS_MAC
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

void SandboxSafeAppSupportPath(WDL_String& path, const char* appGroupID)
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
    
    NSBundle* pBundle = [NSBundle bundleWithIdentifier:[NSString stringWithCString:bundleID encoding:NSUTF8StringEncoding]];
    NSString* pFile = [[[NSString stringWithCString:fileName encoding:NSUTF8StringEncoding] lastPathComponent] stringByDeletingPathExtension];

    if (isCorrectType && pBundle && pFile)
    {
      NSString* pPath = [pBundle pathForResource:pFile ofType:[NSString stringWithCString:searchExt encoding:NSUTF8StringEncoding]];

      if (!pPath)
      {
          pFile = [[NSString stringWithCString:fileName encoding:NSUTF8StringEncoding] stringByDeletingPathExtension];
          pPath = [pBundle pathForResource:pFile ofType:[NSString stringWithCString:searchExt encoding:NSUTF8StringEncoding]];
      }
        
      if (pPath)
      {
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
      WDL_String musicFolder;
      SandboxSafeAppSupportPath(musicFolder);

      if(subfolder)
      {
        NSString* pPluginName = [NSString stringWithCString: subfolder encoding:NSUTF8StringEncoding];
        NSString* pMusicLocation = [NSString stringWithCString: musicFolder.Get() encoding:NSUTF8StringEncoding];
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

EResourceLocation LocateResource(const char* name, const char* type, WDL_String& result, const char* bundleID, void*)
{
  if(CStringHasContents(name))
  {
    // first check this bundle
    if(GetResourcePathFromBundle(name, type, result, bundleID))
      return EResourceLocation::kAbsolutePath;
    
    // then check ~/Music/PLUG_NAME, which is a shared folder that can be accessed from app sandbox
    if(GetResourcePathFromSharedLocation(name, type, result, "VirtualCZ")) //TODO: Hardcoded! This will change
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

#elif defined OS_IOS
#pragma mark - IOS

void HostPath(WDL_String& path, const char* bundleID)
{
}

void PluginPath(WDL_String& path, PluginIDType bundleID)
{
}

void BundleResourcePath(WDL_String& path, PluginIDType bundleID)
{
  NSBundle* pBundle = [NSBundle mainBundle];
  
  if([[pBundle bundleIdentifier] containsString:@"AUv3"])
    pBundle = [NSBundle bundleWithIdentifier:[NSString stringWithCString:bundleID encoding:NSUTF8StringEncoding]];
  
  path.Set([[pBundle resourcePath] UTF8String]);
}

void AppSupportPath(WDL_String& path, bool isSystem)
{
}

void SandboxSafeAppSupportPath(WDL_String& path, const char* appGroupID)
{
  NSFileManager* mgr = [NSFileManager defaultManager];
  NSURL* url = [mgr containerURLForSecurityApplicationGroupIdentifier:[NSString stringWithUTF8String:appGroupID]];
  path.Set([[url path] UTF8String]);
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

bool IsAuv3AppExtension()
{
  return ([[[NSBundle mainBundle] bundleIdentifier] containsString:@"AUv3"]);
}

bool GetResourcePathFromBundle(const char* fileName, const char* searchExt, WDL_String& fullPath, const char* bundleID)
{
  @autoreleasepool
  {
    const char* ext = fileName+strlen(fileName)-1;
    while (ext >= fileName && *ext != '.') --ext;
    ++ext;
    
    bool isCorrectType = !strcasecmp(ext, searchExt);
    
    bool isAppExtension = false;
    
    NSBundle* pBundle = [NSBundle mainBundle];
    
    if([[pBundle bundleIdentifier] containsString:@"AUv3"])
      isAppExtension = true;
    
    if(isAppExtension)
      pBundle = [NSBundle bundleWithIdentifier:[NSString stringWithCString:bundleID encoding:NSUTF8StringEncoding]];
    
    NSString* pFile = [[NSString stringWithCString:fileName encoding:NSUTF8StringEncoding] stringByDeletingPathExtension];
    NSString* pExt = [NSString stringWithCString:searchExt encoding:NSUTF8StringEncoding];
    
    if (isCorrectType && pBundle && pFile)
    {
      NSString* pRootPath;
      
      if(isAppExtension)
        pRootPath = [[[pBundle bundlePath] stringByDeletingLastPathComponent] stringByDeletingLastPathComponent];
      else
        pRootPath = [pBundle bundlePath];
      
      NSString* pPath = [[[[pRootPath stringByAppendingString:@"/"] stringByAppendingString:pFile] stringByAppendingString: @"."] stringByAppendingString:pExt];
      
      if (pPath)
      {
        fullPath.Set([pPath cStringUsingEncoding:NSUTF8StringEncoding]);
        return true;
      }
    }
    
    fullPath.Set("");
    return false;
  }
}

EResourceLocation LocateResource(const char* name, const char* type, WDL_String& result, const char* bundleID, void*)
{
  if(CStringHasContents(name))
  {
#ifdef IGRAPHICS_METAL
    auto itr = gTextureMap.find(name);
    
    if (itr != gTextureMap.end())
    {
      result.Set(name);
      return EResourceLocation::kPreloadedTexture;
    }
#endif
    
    if(GetResourcePathFromBundle(name, type, result, bundleID))
      return EResourceLocation::kAbsolutePath;
  }
  
  return EResourceLocation::kNotFound;
}

#endif

END_IPLUG_NAMESPACE
