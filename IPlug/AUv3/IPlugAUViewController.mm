 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#import <CoreAudioKit/AUViewController.h>
#import "IPlugAUAudioUnit.h"
#import "IPlugAUViewController.h"
#include "IPlugPlatform.h"
#include "IPlugLogger.h"

#ifdef OS_IOS
#import "GenericUI.h"
#endif

#if !__has_feature(objc_arc)
#error This file must be compiled with Arc. Use -fobjc-arc flag
#endif

@interface IPlugAUViewController (AUAudioUnitFactory)
@end

#ifdef OS_IOS
#pragma mark - iOS
@implementation IPlugAUViewController

- (id) init
{
  self = [super initWithNibName:@"IPlugAUViewController"
                         bundle:[NSBundle bundleForClass:NSClassFromString(@"IPlugAUViewController")]];

  return self;
}

- (AUAudioUnit*) createAudioUnitWithComponentDescription:(AudioComponentDescription) desc error:(NSError **)error
{
  self.audioUnit = [[IPlugAUAudioUnit alloc] initWithComponentDescription:desc error:error];

  return self.audioUnit;
}

- (AUAudioUnit*) getAudioUnit
{
  return self.audioUnit;
}

- (void) audioUnitInitialized
{
  //No-op
}

- (void) setAudioUnit:(IPlugAUAudioUnit*) audioUnit
{
  _audioUnit = audioUnit;
  [self audioUnitInitialized];
}

- (void) viewDidLoad
{
  [super viewDidLoad];
}

- (void) viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  
  if(self.audioUnit)
  {
    UIView* view = [_audioUnit openWindow:self.view];

    if(view == nil)
      self.view = [[GenericUI alloc] initWithAUPlugin:self.audioUnit];
      
    int viewWidth = (int) [self.audioUnit width];
    int viewHeight = (int) [self.audioUnit height];
    self.preferredContentSize = CGSizeMake (viewWidth, viewHeight);
  }
}

- (void) viewDidDisappear:(BOOL)animated
{
  [super viewDidDisappear:animated];
  
  if(self.audioUnit)
  {
    [self.audioUnit closeWindow];
  }
}
@end

#else // macOS
#pragma mark - macOS
@implementation IPlugAUViewController

- (id) init
{
  self = [super initWithNibName:@"IPlugAUViewController"
                         bundle:[NSBundle bundleForClass:NSClassFromString(@"IPlugAUViewController")]];

  return self;
}

- (AUAudioUnit*) createAudioUnitWithComponentDescription:(AudioComponentDescription) desc error:(NSError **)error
{
  self.audioUnit = [[IPlugAUAudioUnit alloc] initWithComponentDescription:desc error:error];

  return self.audioUnit;
}

- (AUAudioUnit*) getAudioUnit
{
  return self.audioUnit;
}

- (void) audioUnitInitialized
{
  dispatch_async(dispatch_get_main_queue(), ^{
    int viewWidth = (int) [self.audioUnit width];
    int viewHeight = (int) [self.audioUnit height];
    self.preferredContentSize = CGSizeMake (viewWidth, viewHeight);
  });
}

- (void) setAudioUnit:(IPlugAUAudioUnit*) audioUnit
{
  _audioUnit = audioUnit;
  [self audioUnitInitialized];
}

- (void) viewWillAppear
{
  [_audioUnit openWindow:self.view];
}
@end

#endif

