 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#import <AVFoundation/AVFoundation.h>

@interface IPlugAUPlayer : NSObject

@property (assign) AUAudioUnit* audioUnit;
@property (nonatomic, assign) AudioComponentDescription componentDescription;

+ (instancetype) sharedInstance;
- (instancetype) init;

- (void) loadAudioUnit:(void (^) (void)) completionBlock;

- (void) unmuteOutput;

@end
