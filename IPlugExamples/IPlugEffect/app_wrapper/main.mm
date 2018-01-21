#import <Cocoa/Cocoa.h>
#import <AVFoundation/AVFoundation.h>

#include "wdlstring.h"
#include "swell.h"

int main(int argc, char *argv[])
{
  //if invoked with an argument registerauv3 use plug-in kit to explicitly register auv3 app extension (doesn't happen from debugger)
  if(strcmp(argv[2], "registerauv3"))
  {
    WDL_String appexPath(argv[0]);
    appexPath.SetFormatted(1024, "pluginkit -a %s%s%s.appex", argv[0], "/../../Plugins/", appexPath.get_filepart());
    system(appexPath.Get());
    printf("registered audiounit app extension\n");
  }
  
  return NSApplicationMain(argc,  (const char **) argv);
}

void CenterWindow(HWND hwnd)
{
  if (!hwnd) return;
  
  id turd=(id)hwnd;
  
  if ([turd isKindOfClass:[NSView class]])
  {
    NSWindow *w = [turd window];
    [w center];
  }
  if ([turd isKindOfClass:[NSWindow class]])
  {
    [turd center];
  }
}
