#pragma once

#import <AVFoundation/AVFoundation.h>

@interface IPlugAUPlayer : NSObject

@property (assign) AUAudioUnit* currentAudioUnit;

- (instancetype) initWithComponentType:(UInt32) unitComponentType;

- (void) loadAudioUnitWithComponentDescription:(AudioComponentDescription) desc completion:(void (^) (void)) completionBlock;

- (void) start;

@end
