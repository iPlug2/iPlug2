 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#ifndef _IPLUGAUVIEWCONTROLLER_
#define _IPLUGAUVIEWCONTROLLER_

/** @file AUv3 ViewController for plug-ins with a UI */

#import <CoreAudioKit/AUViewController.h>

@class IPlugAUAudioUnit;

@interface IPlugAUViewController : AUViewController <AUAudioUnitFactory>

@property (nonatomic, retain) IPlugAUAudioUnit* audioUnit;
- (void) setAudioUnit:(IPlugAUAudioUnit*) audioUnit;
- (void) audioUnitInitialized;
- (AUAudioUnit*) getAudioUnit;
@end

#endif /* _IPLUGAUVIEWCONTROLLER_ */
