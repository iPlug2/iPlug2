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

#if defined(OS_VISIONOS) && defined(VISIONOS_TRANSPARENT_VC)
- (UIContainerBackgroundStyle) preferredContainerBackgroundStyle
{
  return UIContainerBackgroundStyleHidden;
}
#endif

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
  [super viewDidLayout];

  if (self.audioUnit)
  {
    [(IPLUG_AUAUDIOUNIT*) self.audioUnit hostResized: self.view.frame.size];
  }
}

- (void) viewWillAppear
{
  [super viewWillAppear];

  NSView* pPlugView = [(IPLUG_AUAUDIOUNIT*) self.audioUnit openWindow:self.view];

  if (pPlugView)
  {
    // Use auto-layout constraints to pin the plugin view to the top-left of the container
    pPlugView.translatesAutoresizingMaskIntoConstraints = NO;

    [NSLayoutConstraint activateConstraints:@[
      [pPlugView.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],
      [pPlugView.topAnchor constraintEqualToAnchor:self.view.topAnchor],
      [pPlugView.widthAnchor constraintEqualToConstant:pPlugView.frame.size.width],
      [pPlugView.heightAnchor constraintEqualToConstant:pPlugView.frame.size.height]
    ]];
  }
}

- (void) viewDidDisappear
{
  [(IPLUG_AUAUDIOUNIT*) self.audioUnit closeWindow];
}

- (instancetype) initWithNibName:(NSNibName)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
  self = [super initWithNibName:nibNameOrNil bundle:[NSBundle bundleForClass:self.class]];
  
  return self;
}

- (void) viewDidLoad
{
  [super viewDidLoad];

  // Disable auto-layout constraint translation for the view controller's view
  // so we can rely on frame-based positioning from the host
  self.view.translatesAutoresizingMaskIntoConstraints = YES;

  // Set autoresizing to keep the view pinned to top-left when host container resizes
  // flexibleMaxX = extra space on right, flexibleMinY = extra space below (stay at top)
  self.view.autoresizingMask = NSViewMaxXMargin | NSViewMinYMargin;

  // Set preferredContentSize here for OOP hosts
  if (self.audioUnit)
  {
    int width = (int) [(IPLUG_AUAUDIOUNIT*) self.audioUnit width];
    int height = (int) [(IPLUG_AUAUDIOUNIT*) self.audioUnit height];
    self.preferredContentSize = CGSizeMake(width, height);
  }
}


#endif

- (AUAudioUnit*) getAudioUnit
{
  return self.audioUnit;
}

- (void) audioUnitInitialized
{
  // Set preferredContentSize synchronously for in-process hosts (REAPER)
  if ([NSThread isMainThread])
  {
    int width = (int) [(IPLUG_AUAUDIOUNIT*) self.audioUnit width];
    int height = (int) [(IPLUG_AUAUDIOUNIT*) self.audioUnit height];
    self.preferredContentSize = CGSizeMake(width, height);
  }
}

@end
