#include "config.h"
#import <Cocoa/Cocoa.h>
#import <AVFoundation/AVFoundation.h>

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

void CheckAU()
{
//  AudioComponentDescription desc;
//  
//  desc.componentType = 'aufx';
//  desc.componentSubType = PLUG_UNIQUE_ID;
//  desc.componentManufacturer = PLUG_MFR_ID;
//  desc.componentFlags = 0;
//  desc.componentFlagsMask = 0;
//  
//  AudioComponentInstantiationOptions options = kAudioComponentInstantiation_LoadOutOfProcess;
//  
//  __block AVAudioUnit* pAU;
//  
//  [AVAudioUnit instantiateWithComponentDescription:desc
//                                           options:options
//                                 completionHandler:^(__kindof AVAudioUnit * _Nullable audioUnit,
//                                                     NSError * _Nullable error) {
//                                   pAU = audioUnit;
//                                 }];
}
