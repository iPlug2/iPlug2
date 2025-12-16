/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#ifdef OS_MAC

#include "IPlugAPP2_Host.h"
#include "IPlugSWELL.h"
#include "config.h"

#import <Cocoa/Cocoa.h>

using namespace iplug;

HWND gHWND = nullptr;
HINSTANCE gHINSTANCE = nullptr;

/**
 * Check if the app is running in a sandbox container
 */
static bool AppIsSandboxed()
{
  NSString* homePath = NSHomeDirectory();
  return [homePath containsString:@"/Library/Containers/"];
}

/**
 * Application delegate for Cocoa event handling
 */
@interface IPlugAPP2Delegate : NSObject <NSApplicationDelegate>
@property (nonatomic, assign) IPlugAPP2Host* host;
@end

@implementation IPlugAPP2Delegate

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
  // Create and initialize host
  self.host = IPlugAPP2Host::Create();
  if (!self.host)
  {
    NSAlert* alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Error"];
    [alert setInformativeText:@"Failed to create application host"];
    [alert runModal];
    [NSApp terminate:nil];
    return;
  }

  if (!self.host->Init())
  {
    NSAlert* alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Error"];
    [alert setInformativeText:@"Failed to initialize application"];
    [alert runModal];
    [NSApp terminate:nil];
    return;
  }

  // TODO: Create main window and attach plugin UI
}

- (void)applicationWillTerminate:(NSNotification*)notification
{
  if (self.host)
  {
    self.host->StopAudio();
    self.host->CloseWindow();
  }
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender
{
  return YES;
}

- (IBAction)showPreferences:(id)sender
{
  if (self.host)
  {
    self.host->ShowSettingsDialog(gHWND);
  }
}

- (IBAction)showAbout:(id)sender
{
  if (self.host && self.host->GetPlug())
  {
    if (!self.host->GetPlug()->OnHostRequestingAboutBox())
    {
      // Show default about box
      NSString* info = [NSString stringWithFormat:@"%s\nBuilt on %s",
                        PLUG_COPYRIGHT_STR, __DATE__];
      NSAlert* alert = [[NSAlert alloc] init];
      [alert setMessageText:@PLUG_NAME];
      [alert setInformativeText:info];
      [alert runModal];
    }
  }
}

@end

/**
 * macOS application entry point
 */
int main(int argc, char* argv[])
{
  @autoreleasepool
  {
    // Initialize SWELL
    SWELL_InitAutoRelease();

    // Create application
    [NSApplication sharedApplication];

    // Set up delegate
    IPlugAPP2Delegate* delegate = [[IPlugAPP2Delegate alloc] init];
    [NSApp setDelegate:delegate];

    // Set activation policy
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    // Load main menu from resources or create programmatically
    // TODO: Create menu

    // Run
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp run];
  }

  return 0;
}

#endif // OS_MAC
