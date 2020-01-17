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

@implementation IPlugAUViewController

- (id)init {
#if PLUG_HAS_UI
  self = [super initWithNibName:@"IPlugAUViewController" bundle:[NSBundle bundleForClass:NSClassFromString(@"IPlugAUViewController")]];
#else
  self = [super init];
#endif
  
  return self;
}

- (AUAudioUnit*) createAudioUnitWithComponentDescription:(AudioComponentDescription) desc error:(NSError **)error
{
  TRACE
  self.audioUnit = [[IPlugAUAudioUnit alloc] initWithComponentDescription:desc error:error];

  return self.audioUnit;
}

- (AUAudioUnit*) getAudioUnit
{
  return _audioUnit;
}

- (void) audioUnitInitialized
{
  //No-op
}

- (void)setAudioUnit:(IPlugAUAudioUnit*) audioUnit
{
  TRACE
  _audioUnit = audioUnit;
  [self audioUnitInitialized];
}

#if PLUG_HAS_UI
- (void) doOpenUI
{
  PLATFORM_VIEW* view = [_audioUnit openWindow:self.view];

#ifdef OS_IOS
  if(view == nil)
    self.view = [[GenericUI alloc] initWithAUPlugin:self.audioUnit];
#endif
  
  int viewWidth = (int) [_audioUnit width];
  int viewHeight = (int) [_audioUnit height];
  self.preferredContentSize = CGSizeMake (viewWidth, viewHeight);
}

- (void) viewDidLoad
{
  TRACE
  [super viewDidLoad];
  
#ifdef OS_MAC
  [self doOpenUI];
#endif
}

- (void) viewDidLayoutSubviews
{
  TRACE
}

#ifdef OS_IOS
- (void) viewWillAppear:(BOOL)animated
{
  TRACE
  [super viewWillAppear:animated];
  
  if(_audioUnit)
    [self doOpenUI];
}

- (void) viewDidDisappear:(BOOL)animated
{
  TRACE
  [super viewDidDisappear:animated];
  
  if(_audioUnit)
    [_audioUnit closeWindow];
}
#else // MAC_OS
- (void) viewWillAppear:(BOOL)animated
{
}
#endif

#else // PLUG_HAS_UI
- (void)beginRequestWithExtensionContext:(nonnull NSExtensionContext *)context {
}
#endif

@end

