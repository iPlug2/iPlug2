 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#import "AppViewController.h"
#import "IPlugAUPlayer.h"
#import "IPlugAUAudioUnit.h"

#include "config.h"

#import "IPlugAUViewController.h"
#import <CoreAudioKit/CoreAudioKit.h>

#if !__has_feature(objc_arc)
#error This file must be compiled with Arc. Use -fobjc-arc flag
#endif

@interface AppViewController ()
{
  IPlugAUPlayer* player;
  IPLUG_AUVIEWCONTROLLER* pluginVC;
  __weak IBOutlet UIView *auView;
}
@end

@implementation AppViewController

- (BOOL) prefersStatusBarHidden
{
  return YES;
}

- (void) viewDidLoad
{
  [super viewDidLoad];

#if PLUG_HAS_UI
  NSString* storyBoardName = [NSString stringWithFormat:@"%s-iOS-MainInterface", PLUG_NAME];
  UIStoryboard* storyboard = [UIStoryboard storyboardWithName:storyBoardName bundle: nil];
  pluginVC = [storyboard instantiateViewControllerWithIdentifier:@"main"];
  [self addChildViewController:pluginVC];
#endif

  self->player = [IPlugAUPlayer sharedInstance];
  
 [ self->player loadAudioUnit:^{
   self->pluginVC.audioUnit = (IPLUG_AUAUDIOUNIT*) self->player.audioUnit;

   [self embedPlugInView];
 }];
 
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(receiveNotification:) name:@"AppMuted" object:nil];
}

- (void) embedPlugInView
{
#if PLUG_HAS_UI
  UIView* view = pluginVC.view;
  view.frame = auView.bounds;
  [auView addSubview: view];
#endif
}

- (void) showMutedDialog: (NSString*) reason
{
  UIAlertController* alert = [UIAlertController alertControllerWithTitle:reason
                                                                 message:@"Audio is muted"
                                                          preferredStyle:UIAlertControllerStyleAlert];

  UIAlertAction* unmuteAction = [UIAlertAction actionWithTitle:@"Unmute" style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
     [self->player unmuteOutput];
  }];

  UIAlertAction* cancelAction = [UIAlertAction actionWithTitle:@"Leave muted" style:UIAlertActionStyleCancel handler:nil];

  [alert addAction:unmuteAction];
  [alert addAction:cancelAction];

  [[NSOperationQueue mainQueue] addOperationWithBlock:^{
   if (self.presentedViewController)
   {
     [[self presentedViewController] dismissViewControllerAnimated:YES completion:nil];
   }
   [self presentViewController:alert animated:YES completion:nil];
  }];
}

- (UIRectEdge) preferredScreenEdgesDeferringSystemGestures
{
  return UIRectEdgeAll;
}

- (void) receiveNotification: (NSNotification*) notification
{
  if ([notification.name isEqualToString:@"AppMuted"])
  {
    [self showMutedDialog:[notification.userInfo valueForKey: @"reason"]];
  }
}
@end
