#include "IPlugPaths.h"
#include "IPlugConstants.h"
#import <UIKit/UIKit.h>

void HostPath(WDL_String& path, const char* bundleID)
{
}

void PluginPath(WDL_String& path, const char* bundleID)
{
}

void DesktopPath(WDL_String& path)
{
}

void UserHomePath(WDL_String& path)
{
}

void VST3PresetsPath(WDL_String& path, const char* mfrName, const char* pluginName, bool isSystem)
{
}

void AppSupportPath(WDL_String& path, bool isSystem)
{
}

void SandboxSafeAppSupportPath(WDL_String& path)
{
  NSArray *pPaths = NSSearchPathForDirectoriesInDomains(NSMusicDirectory, NSUserDomainMask, YES);
  NSString *pUserMusicDirectory = [pPaths objectAtIndex:0];
  path.Set([pUserMusicDirectory UTF8String]);
}
