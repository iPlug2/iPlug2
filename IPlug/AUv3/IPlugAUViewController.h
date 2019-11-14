 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

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
- (void) audioUnitInitialized;
- (AUAudioUnit*) getAudioUnit;
@end

#endif /* _IPLUGAUVIEWCONTROLLER_ */
