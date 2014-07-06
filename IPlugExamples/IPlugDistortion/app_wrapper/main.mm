#import <Cocoa/Cocoa.h>
#include "swell.h"

int main(int argc, char *argv[])
{
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

