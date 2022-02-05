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

#if !__has_feature(objc_arc)
#error This file must be compiled with Arc. Use -fobjc-arc flag
#endif

@interface IPLUG_AUVIEWCONTROLLER (AUAudioUnitFactory)
@end

@implementation IPLUG_AUVIEWCONTROLLER

- (AUAudioUnit*) createAudioUnitWithComponentDescription:(AudioComponentDescription) desc error:(NSError **)error
{
  self.audioUnit = [[IPLUG_AUAUDIOUNIT alloc] initWithComponentDescription:desc error:error];

  [self audioUnitInitialized];

  return self.audioUnit;
}

#ifdef OS_IOS
- (void) viewDidLayoutSubviews
{
  if (self.audioUnit)
  {
    [(IPLUG_AUAUDIOUNIT*) self.audioUnit hostResized: self.view.frame.size];
  }
}

- (void) viewWillAppear:(BOOL) animated
{
  [super viewWillAppear:animated];
  
  if (self.audioUnit)
  {
    [(IPLUG_AUAUDIOUNIT*) self.audioUnit openWindow:self.view];
  }
}

- (void) viewDidDisappear:(BOOL) animated
{
  [super viewDidDisappear:animated];
  
  if (self.audioUnit)
  {
    [(IPLUG_AUAUDIOUNIT*) self.audioUnit closeWindow];
  }
}
#else
- (void) viewDidLayout
{
  if (self.audioUnit)
  {
    [(IPLUG_AUAUDIOUNIT*) self.audioUnit hostResized: self.view.frame.size];
  }
}

- (void) viewWillAppear
{
  [(IPLUG_AUAUDIOUNIT*) self.audioUnit openWindow:self.view];
}

- (void) viewDidDisappear
{
  [(IPLUG_AUAUDIOUNIT*) self.audioUnit closeWindow];
}

- (void) loadView
{
  int width = (int) [(IPLUG_AUAUDIOUNIT*) self.audioUnit width];
  int height = (int) [(IPLUG_AUAUDIOUNIT*) self.audioUnit height];
  self.view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, width, height)];
}

#endif

- (AUAudioUnit*) getAudioUnit
{
  return self.audioUnit;
}

- (void) audioUnitInitialized
{
  dispatch_async(dispatch_get_main_queue(), ^{
    if (self.audioUnit)
    {
      int viewWidth = (int) [(IPLUG_AUAUDIOUNIT*) self.audioUnit width];
      int viewHeight = (int) [(IPLUG_AUAUDIOUNIT*) self.audioUnit height];
      self.preferredContentSize = CGSizeMake (viewWidth, viewHeight);
    }
  });
}

@end
