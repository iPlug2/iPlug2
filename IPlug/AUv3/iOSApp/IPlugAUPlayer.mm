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

bool isInstrument()
{
#if PLUG_TYPE == 1
  return YES;
#else
  return NO;
#endif
}

@implementation IPlugAUPlayer
{
  AVAudioEngine* engine;
  AVAudioUnit* avAudioUnit;
  UInt32 componentType;
}

- (instancetype) initWithComponentType: (UInt32) unitComponentType
{
  self = [super init];
  
  if (self)
  {
    engine = [[AVAudioEngine alloc] init];
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
  
  [engine attachNode:avAudioUnit];

  self.currentAudioUnit = avAudioUnit.AUAudioUnit;
  
  [self setupSession];
    
#ifdef _DEBUG
  [self printEngineInfo];
  [self printSessionInfo];
#endif
  
  [self makeEngineConnections];
  [self addNotifications];
  
  AVAudioSession* session = [AVAudioSession sharedInstance];

  if (![session setActive:TRUE error: &error])
  {
    NSLog(@"Error setting session active: %@", [error localizedDescription]);
  }
  
  if (![engine startAndReturnError: &error])
  {
    NSLog(@"engine failed to start: %@", error);
  }

  completionBlock();
}

- (void) dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver: self];
}

- (void) restartAudioEngine
{
  [engine stop];

  NSError *error = nil;
  
  if (![engine startAndReturnError:&error])
  {
    NSLog(@"Error re-starting audio engine: %@", error);
  }
  else
  {
    [self printSessionInfo];
  }
}

- (void) setupSession
{
  AVAudioSession* session = [AVAudioSession sharedInstance];
  NSError* error = nil;

  AVAudioSessionCategoryOptions options = AVAudioSessionCategoryOptionDefaultToSpeaker | AVAudioSessionCategoryOptionAllowBluetooth;
  [session setCategory: isInstrument() ? AVAudioSessionCategoryPlayback
                                       : AVAudioSessionCategoryPlayAndRecord
                  withOptions:options error: &error];
  
  if (error)
  {
    NSLog(@"Error setting category: %@", error);
  }
  
  [session setPreferredSampleRate:iplug::DEFAULT_SAMPLE_RATE error: &error];
  
  if (error)
  {
    NSLog(@"Error setting samplerate: %@", error);
  }
  
  [session setPreferredIOBufferDuration:128.0/iplug::DEFAULT_SAMPLE_RATE error: &error];
  
  if (error)
  {
    NSLog(@"Error setting io buffer duration: %@", error);
  }
}

- (void) makeEngineConnections
{
  if (!isInstrument())
  {
    AVAudioNode* inputNode = [engine inputNode];
    AVAudioFormat* inputNodeFormat = [inputNode inputFormatForBus:0];
    
    @autoreleasepool {
      @try {
        [engine connect:inputNode to:avAudioUnit format: inputNodeFormat];
      }
      @catch (NSException *exception) {
        NSLog(@"NSException when trying to connect input node: %@, Reason: %@", exception.name, exception.reason);
      }
    }
  }
  
  auto numOutputBuses = [avAudioUnit numberOfOutputs];
  AVAudioMixerNode* mainMixer = [engine mainMixerNode];
  AVAudioFormat* pluginOutputFormat = [avAudioUnit outputFormatForBus:0];
  AVAudioNode* outputNode = [engine outputNode];

  if (numOutputBuses > 1)
  {
    // Assume all output buses are the same format
    for (int busIdx=0; busIdx<numOutputBuses; busIdx++)
    {
      [engine connect:avAudioUnit to:mainMixer fromBus: busIdx toBus:[mainMixer nextAvailableInputBus] format: pluginOutputFormat];
    }
  }
  else
  {
    [engine connect:avAudioUnit to:outputNode format: pluginOutputFormat];
  }
}

- (void) printEngineInfo
{
  if (!isInstrument())
  {
    AVAudioFormat* inputNodeFormat = [[engine inputNode] inputFormatForBus:0];
    AVAudioFormat* pluginInputFormat = [avAudioUnit inputFormatForBus:0];
    NSLog(@"Input Node SR: %i", int(inputNodeFormat.sampleRate));
    NSLog(@"Input Node Chans: %i", inputNodeFormat.channelCount);
    NSLog(@"Plugin Input SR: %i", int(pluginInputFormat.sampleRate));
    NSLog(@"Plugin Input Chans: %i", pluginInputFormat.channelCount);
  }
  
  AVAudioFormat* pluginOutputFormat = [avAudioUnit outputFormatForBus:0];
  AVAudioFormat* outputNodeFormat = [[engine outputNode] outputFormatForBus:0];
  
  NSLog(@"Plugin Output SR: %i", int(pluginOutputFormat.sampleRate));
  NSLog(@"Plugin Output Chans: %i", pluginOutputFormat.channelCount);
  NSLog(@"Output Node SR: %i", int(outputNodeFormat.sampleRate));
  NSLog(@"Output Node Chans: %i", outputNodeFormat.channelCount);
}

- (void) printSessionInfo
{
  AVAudioSession* session = [AVAudioSession sharedInstance];
  NSLog(@"Session SR: %i", int(session.sampleRate));
  NSLog(@"Session IO Buffer: %i", int((session.IOBufferDuration * session.sampleRate)+0.5));
  if (!isInstrument()) NSLog(@"Session Input Chans: %i", int(session.inputNumberOfChannels));
  NSLog(@"Session Output Chans: %i", int(session.outputNumberOfChannels));
  if (!isInstrument()) NSLog(@"Session Input Latency: %f ms", session.inputLatency * 1000.0f);
  NSLog(@"Session Output Latency: %f ms", session.outputLatency * 1000.0f);
  AVAudioSessionRouteDescription *currentRoute = [session currentRoute];
  for (AVAudioSessionPortDescription* input in currentRoute.inputs)
  {
    NSLog(@"Input Port Name: %@", input.portName);
  }
  
  for (AVAudioSessionPortDescription* output in currentRoute.outputs)
  {
    NSLog(@"Output Port Name: %@", output.portName);
  }
}

- (void) addNotifications
{
  NSNotificationCenter* notifCtr = [NSNotificationCenter defaultCenter];

  [notifCtr addObserver: self selector: @selector (onEngineConfigurationChange:) name:AVAudioEngineConfigurationChangeNotification object: engine];
}

#pragma mark Notifications
- (void) onEngineConfigurationChange: (NSNotification*) notification
{
  [self restartAudioEngine];
}

@end
