/*
==============================================================================

This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

See LICENSE.txt for  more info.

==============================================================================
*/

#import "SettingsViewController.h"
#import <AVFoundation/AVFoundation.h>
#import <CoreAudioKit/CoreAudioKit.h>
#import <UIKit/UIKit.h>

@interface SettingsViewController ()
@property (weak, nonatomic) IBOutlet UIButton *audioInputDeviceButton;
@property (weak, nonatomic) IBOutlet UISwitch *micMuteSwitch;
@property (weak, nonatomic) IBOutlet UIButton *audioOutputDeviceButton;
@end

@implementation SettingsViewController

- (void) viewDidLoad
{
  [super viewDidLoad];
  
  [self addNotifications];
  [self populateAudioInputDeviceMenu];
  [self updateMicMuteState];
}

- (void) dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver: self];
}

- (void) addNotifications
{
  NSNotificationCenter* notifCtr = [NSNotificationCenter defaultCenter];
  AVAudioSession *session = [AVAudioSession sharedInstance];

  [notifCtr addObserver: self selector: @selector (onSessionRouteChanges:) name:AVAudioSessionRouteChangeNotification object: session];
}

- (void) updateMicMuteState
{
  if (@available(iOS 17.0, *)) {
    AVAudioApplication* audioApplication = [AVAudioApplication sharedInstance];
    [self.micMuteSwitch setOn: [audioApplication isInputMuted]];
  } else {
    // TODO: Fallback on earlier versions
  }
}

- (void) populateAudioInputDeviceMenu
{
  AVAudioSession *session = [AVAudioSession sharedInstance];
//  NSError *error = nil;
//  BOOL success = [session setActive:YES error:&error];
//  if (!success) {
//    NSLog(@"Error activating AVAudioSession: %@", error.localizedDescription);
//    return;
//  }
  
  NSArray<AVAudioSessionPortDescription *> *availableInputs = session.availableInputs;
  
  NSMutableArray<UIAction *> *menuActions = [[NSMutableArray alloc] init];
  
  for (AVAudioSessionPortDescription *input in availableInputs) 
  {
    UIAction *action = [UIAction actionWithTitle:input.portName
                                            image:nil
                                       identifier:nil
                                          handler:^(__kindof UIAction * _Nonnull action) {
      NSError *error = nil;
      [session setPreferredInput:input error:&error];
      if (error) {
        NSLog(@"Error setting preferred audio input: %@", error.localizedDescription);
      }
      [self.audioInputDeviceButton setTitle:input.portName forState:UIControlStateNormal];
    }];
    [menuActions addObject:action];
  }
  
  UIMenu *inputDeviceMenu = [UIMenu menuWithTitle:@"" children:menuActions];
  
  self.audioInputDeviceButton.menu = inputDeviceMenu;
  self.audioInputDeviceButton.showsMenuAsPrimaryAction = YES;
}


- (IBAction)bluetoothButtonPressed:(id)sender 
{
  CABTMIDICentralViewController* vc = [[CABTMIDICentralViewController alloc] init];
  UINavigationController* nc = [[UINavigationController alloc] initWithRootViewController:vc];
  nc.modalPresentationStyle = UIModalPresentationPopover;

  UIPopoverPresentationController* ppc = nc.popoverPresentationController;
  ppc.permittedArrowDirections = UIPopoverArrowDirectionAny;
  if (@available(iOS 16.0, *)) {
    ppc.sourceItem = sender;
  } else {
    // TODO: Fallback on earlier versions
  }

  [self presentViewController:nc animated:YES completion:nil];
}

- (IBAction)audioOutputDeviceButtonPressed:(id)sender 
{
}

- (IBAction)micMuteSwitchChanged:(id)sender 
{
  if (@available(iOS 17.0, *)) {
    NSError *error = nil;
    AVAudioApplication* audioApplication = [AVAudioApplication sharedInstance];
    [audioApplication setInputMuted:[self.micMuteSwitch isOn] error:&error];
  } else {
    // TODO: Fallback on earlier versions
  }
}

#pragma mark Notifications
- (void) onSessionRouteChanges: (NSNotification*) notification
{
  [self populateAudioInputDeviceMenu];
}

@end
