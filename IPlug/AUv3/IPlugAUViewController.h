#ifndef _IPLUGAUVIEWCONTROLLER_
#define _IPLUGAUVIEWCONTROLLER_

/** @file AUv3 ViewController */

#import <CoreAudioKit/AUViewController.h>
#include "config.h"

@class IPlugAUAudioUnit;

#if PLUG_HAS_UI
@interface IPlugAUViewController : AUViewController <AUAudioUnitFactory>
#else
@interface IPlugAUViewController : NSObject <AUAudioUnitFactory>
#endif

@property (nonatomic, retain) IPlugAUAudioUnit *audioUnit;
- (void)setAudioUnit:(IPlugAUAudioUnit*) audioUnit;
@end

#endif /* _IPLUGAUVIEWCONTROLLER_ */
