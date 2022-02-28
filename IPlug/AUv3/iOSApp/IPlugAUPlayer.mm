 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#import "IPlugAUPlayer.h"
#include "IPlugConstants.h"
#include "config.h"

#if !__has_feature(objc_arc)
#error This file must be compiled with Arc. Use -fobjc-arc flag
#endif

@implementation IPlugAUPlayer
{
  AVAudioEngine* audioEngine;
  AVAudioUnit* avAudioUnit;
  UInt32 componentType;
}

- (instancetype) initWithComponentType: (UInt32) unitComponentType
{
  self = [super init];
  
  if (self)
  {
    audioEngine = [[AVAudioEngine alloc] init];
    componentType = unitComponentType;
  }

  return self;
}

- (void) loadAudioUnitWithComponentDescription:(AudioComponentDescription)desc
                                   completion:(void (^) (void))completionBlock
{
  [AVAudioUnit instantiateWithComponentDescription:desc options:0
                                 completionHandler:^(AVAudioUnit* __nullable audioUnit, NSError* __nullable error) {
                                   [self onAudioUnitInstantiated:audioUnit error:error completion:completionBlock];
                                 }];
}

- (void) onAudioUnitInstantiated:(AVAudioUnit* __nullable) audioUnit error:(NSError* __nullable) error completion:(void (^) (void))completionBlock
{
  if (audioUnit == nil)
    return;
  
  avAudioUnit = audioUnit;
  
  self.currentAudioUnit = avAudioUnit.AUAudioUnit;
  
  AVAudioSession* session = [AVAudioSession sharedInstance];
  
#if PLUG_TYPE == 1
  [session setCategory: AVAudioSessionCategoryPlayback error:&error];
#else
  [session setCategory: AVAudioSessionCategoryPlayAndRecord error:&error];
#endif
  
  [session setPreferredSampleRate:iplug::DEFAULT_SAMPLE_RATE error:nil];
  [session setPreferredIOBufferDuration:128.0/iplug::DEFAULT_SAMPLE_RATE error:nil];
  AVAudioMixerNode* mainMixer = [audioEngine mainMixerNode];
  mainMixer.outputVolume = 1;
  [audioEngine attachNode:avAudioUnit];
  
#if PLUG_TYPE != 1
  AVAudioFormat* micInputFormat = [[audioEngine inputNode] inputFormatForBus:0];
  AVAudioFormat* pluginInputFormat = [avAudioUnit inputFormatForBus:0];
#endif
  
  AVAudioFormat* pluginOutputFormat = [avAudioUnit outputFormatForBus:0];

  NSLog(@"Session SR: %i", int(session.sampleRate));
  NSLog(@"Session IO Buffer: %i", int((session.IOBufferDuration * session.sampleRate)+0.5));
  
#if PLUG_TYPE != 1
  NSLog(@"Mic Input SR: %i", int(micInputFormat.sampleRate));
  NSLog(@"Mic Input Chans: %i", micInputFormat.channelCount);
  NSLog(@"Plugin Input SR: %i", int(pluginInputFormat.sampleRate));
  NSLog(@"Plugin Input Chans: %i", pluginInputFormat.channelCount);
#endif
  
#if PLUG_TYPE != 1
  if (pluginInputFormat != nil)
    [audioEngine connect:audioEngine.inputNode to:avAudioUnit format: micInputFormat];
#endif
  
  auto numOutputBuses = [avAudioUnit numberOfOutputs];
  
  if (numOutputBuses > 1)
  {
    // Assume all output buses are the same format
    for (int busIdx=0; busIdx<numOutputBuses; busIdx++)
    {
      [audioEngine connect:avAudioUnit to:mainMixer fromBus: busIdx toBus:[mainMixer nextAvailableInputBus] format:pluginOutputFormat];
    }
  }
  else
  {
    [audioEngine connect:avAudioUnit to:audioEngine.outputNode format: pluginOutputFormat];
  }
  
  BOOL success = [[AVAudioSession sharedInstance] setActive:TRUE error:nil];
  
  if (success == NO)
    NSLog(@"Error setting category: %@", [error localizedDescription]);
  
  if (![audioEngine startAndReturnError:&error])
    NSLog(@"engine failed to start: %@", error);
  
  completionBlock();
}

@end
