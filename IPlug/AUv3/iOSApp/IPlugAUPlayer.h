 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#import <AVFoundation/AVFoundation.h>

@interface IPlugAUPlayer : NSObject

@property (assign) AUAudioUnit* currentAudioUnit;

- (instancetype) initWithComponentType:(UInt32) unitComponentType;

- (void) loadAudioUnitWithComponentDescription:(AudioComponentDescription) desc completion:(void (^) (void)) completionBlock;
@end
