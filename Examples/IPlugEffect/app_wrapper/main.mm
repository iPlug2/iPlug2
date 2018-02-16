#import <Cocoa/Cocoa.h>
#import <AVFoundation/AVFoundation.h>

#include "wdlstring.h"
#include "swell.h"

void CenterWindow(HWND hwnd)
{
  if (!hwnd) return;
  
  id view =(id) hwnd;
  
  if ([view isKindOfClass:[NSView class]])
  {
    NSWindow* pWindow = [view window];
    [pWindow center];
  }
}

bool IsSandboxed()
{
  NSString* pHomeDir = NSHomeDirectory();
  //  NSString* pBundleIdentifier = [[NSBundle mainBundle] bundleIdentifier];
  
  if ([pHomeDir containsString:@"Library/Containers/"])
  {
    return true;
  }
  return false;
}

int main(int argc, char *argv[])
{
  //if invoked with an argument registerauv3 use plug-in kit to explicitly register auv3 app extension (doesn't happen from debugger)
  if(strcmp(argv[2], "registerauv3"))
  {
    WDL_String appexPath(argv[0]);
    appexPath.SetFormatted(1024, "pluginkit -a %s%s%s.appex", argv[0], "/../../Plugins/", appexPath.get_filepart());
    if(system(appexPath.Get()) > -1)
      NSLog(@"Registered audiounit app extension\n");
    
    if(IsSandboxed())
      NSLog(@"SANDBOXED\n");
  }
  
  return NSApplicationMain(argc,  (const char **) argv);
}

