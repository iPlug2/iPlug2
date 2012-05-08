// IPlugIOSAudio.mm based on libpd ios audio wrapper, see license below


/**
 * This software is copyrighted by Reality Jockey Ltd. and Peter Brinkmann.
 * The following terms (the "Standard Improved BSD License") apply to
 * all files associated with the software unless explicitly disclaimed
 * in individual files:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 * products derived from this software without specific prior
 * written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "IPlugIOSAudio.h"

@interface IPlugIOSAudio()

// private
- (void)initializeAudioSession:(int)ticks withAudioCategory:(UInt32)audioCategory;
- (void)initializeAudioUnit;

@end


@implementation IPlugIOSAudio

@synthesize audioUnit;
@synthesize sampleRate;
@synthesize numInputChannels;
@synthesize numOutputChannels;
@synthesize microphoneVolume;
@synthesize floatBufferL;
@synthesize floatBufferR;
@synthesize floatBufferLength;

/** The render callback used by the audio unit. This is where all of the action is regarding the AU. */
// This function must be listed first (and thus defined) because a pointer to this function is used
// to define the AU later in the code during setup.
// http://developer.apple.com/iphone/library/documentation/AudioUnit/Reference/AUComponentServicesReference/Reference/reference.html#//apple_ref/doc/c_ref/AURenderCallback
OSStatus renderCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags,
                        const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber,
                        UInt32 inNumberFrames, AudioBufferList *ioData)
{

  IPlugIOSAudio *controller = (IPlugIOSAudio *) inRefCon;

  //NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  // Get the remote io audio unit to render its input into the buffers
  // 1 == inBusNumber for mic input
  AudioUnitRender(controller.audioUnit, ioActionFlags, inTimeStamp, 1, inNumberFrames, ioData);

  short *shortBufferL = (short *) ioData->mBuffers[0].mData;
  FLT *floatBufferL = controller.floatBufferL;

#ifdef IOS_STEREO
  FLT *floatBufferR = controller.floatBufferR;
  short *shortBufferR = (short *) ioData->mBuffers[1].mData;
#endif

  int floatBufferLength = controller.floatBufferLength;
  float a = 0.000030517578125f * controller.microphoneVolume; // == 1/32768 * microphone volume

#ifdef IOS_AUDIO_IN
  for (int i = 0; i < floatBufferLength; i++)
  {
    floatBufferL[i] = ((FLT) shortBufferL[i]) * a;
#ifdef IOS_STEREO
    floatBufferR[i] = ((FLT) shortBufferR[i]) * a;
#endif
  }
#endif

  FLT *floatBufferData[1];
  floatBufferData[0] = floatBufferL;

#ifdef IOS_STEREO
  floatBufferData[1] = floatBufferR;
#else
  floatBufferData[1] = floatBufferL;
#endif

#ifdef IOS_AUDIO_SINGLE_PRECISION
  controller->mPluginInstance->LockMutexAndProcessSingleReplacing(floatBufferData, floatBufferData, floatBufferLength);
#else
  controller->mPluginInstance->LockMutexAndProcessDoubleReplacing(floatBufferData, floatBufferData, floatBufferLength);
#endif

  for (int i = 0; i < floatBufferLength; i++)
  {
    float fL = floatBufferL[i];

    if (fL < FLT(-1.0)) shortBufferL[i] = -32767;
    else if (fL > FLT(1.0)) shortBufferL[i] = 32767;
    else shortBufferL[i] = (short) (fL * FLT(32767.0));

#ifdef IOS_STEREO
    float fR = floatBufferR[i];
    if (fR < FLT(-1.0)) shortBufferR[i] = -32767;
    else if (fR > FLT(1.0)) shortBufferR[i] = 32767;
    else shortBufferR[i] = (short) (fR * FLT(32767.0));
#endif
  }

//  [pool drain]; // drain the pool and release any retained objects

  return 0; // no errors
}

// the interrupt listener for the audio session
void audioSessionInterruptListener(void *inClientData, UInt32 inInterruption)
{
  IPlugIOSAudio *controller = (IPlugIOSAudio *) inClientData;
  switch (inInterruption)
  {
    case kAudioSessionBeginInterruption:
    {
      // when the interruption begins, suspend audio playback
      //NSLog(@"AudioSession === kAudioSessionBeginInterruption");
      [controller pause];
      break;
    }
    case kAudioSessionEndInterruption:
    {
      // when the interruption ends, resume audio playback
      //NSLog(@"AudioSession === kAudioSessionEndInterruption");
      [controller play];
      break;
    }
    default:
    {
      break;
    }
  }
}

- (id)initWithIPlugInstance:(IPlug*)pluginInstance andSampleRate:(float)newSampleRate andTicksPerBuffer:(int)ticks
   andNumberOfInputChannels:(int)inputChannels andNumberOfOutputChannels:(int)outputChannels
{

  // this version of init uses PlayAndRecord audio session
  // set the audio category to PlayAndRecord so that we can have low-latency IO
  UInt32 audioCategory = kAudioSessionCategory_PlayAndRecord;

  return [self initWithIPlugInstance:pluginInstance andSampleRate:newSampleRate andTicksPerBuffer:ticks
            andNumberOfInputChannels:inputChannels andNumberOfOutputChannels:outputChannels
             andAudioSessionCategory:audioCategory];
}

- (id)initWithIPlugInstance:(IPlug*)pluginInstance andSampleRate:(float)newSampleRate andTicksPerBuffer:(int)ticks
   andNumberOfInputChannels:(int)inputChannels andNumberOfOutputChannels:(int)outputChannels
    andAudioSessionCategory:(UInt32)audioSessionCategory
{
  self = [super init];
  if (self != nil)
  {
    audioUnit = NULL;
    numInputChannels = inputChannels;
    numOutputChannels = outputChannels;
    sampleRate = (Float64) newSampleRate;
    microphoneVolume = 1.0f;

    int numberOfChannels = (numInputChannels < numOutputChannels) ? numOutputChannels : numInputChannels;

    floatBufferLength = ticks * numberOfChannels;
    //floatBufferLength = IOS_BLOCK_SIZE * ticks * numberOfChannels;
    floatBufferL = (FLT *) malloc(floatBufferLength * sizeof(FLT));;
    floatBufferR = (FLT *) malloc(floatBufferLength * sizeof(FLT));;

    [self initializeAudioSession:ticks withAudioCategory:audioSessionCategory];
    [self initializeAudioUnit];

    mPluginInstance = pluginInstance;
    mPluginInstance->SetBlockSize(IOS_BLOCK_SIZE);
    mPluginInstance->SetSampleRate(sampleRate);
    mPluginInstance->Reset();
    //mPluginInstance->RestorePreset(0);

  }
  return self;
}

- (void)dealloc
{
  [self pause];
  // TODO: delete audioUnit?
  free(floatBufferL);
  free(floatBufferR);

  floatBufferL = nil;
  floatBufferR = nil;

  [super dealloc];
}

/** Begin audio/scene playback.*/
- (void)play
{
  AudioOutputUnitStart(audioUnit);
  //NSLog(@"AudioSession === starting audio unit.");
}

/** Pause audio/scene playback. */
- (void)pause
{
  AudioOutputUnitStop(audioUnit);
  //NSLog(@"AudioSession === stopping audio unit.");
}

// private
- (void)initializeAudioSession:(int)ticks withAudioCategory:(UInt32) audioCategory
{
  /*** Create AudioSession interface to Core Audio === ***/

  // initialise the audio session
  AudioSessionInitialize(NULL, NULL, audioSessionInterruptListener, self);

  AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(audioCategory), &audioCategory);

  // set the sample rate of the session
  AudioSessionSetProperty(kAudioSessionProperty_PreferredHardwareSampleRate, sizeof(sampleRate), &sampleRate);
  NSLog(@"AudioSession === setting PreferredHardwareSampleRate to %.0fHz.", sampleRate);

  // set buffer size //TODO: blocksize
  Float32 bufferSize = 64;
  //Float32 bufferSize = (Float32) [PdBase getBlockSize] * ticks; // requested buffer size
  Float32 bufferDuration = (bufferSize + 0.5) / sampleRate; // buffer duration in seconds - add 0.5 due to differences in armv6 and armv7 fp
  AudioSessionSetProperty(kAudioSessionProperty_PreferredHardwareIOBufferDuration,
  sizeof(bufferDuration), &bufferDuration);
  NSLog(@"AudioSession === setting PreferredHardwareIOBufferDuration to %3.2fms.", bufferDuration*1000.0);

  // NOTE: note that round-off errors make it hard to determine whether the requested buffersize
  // was granted. we just assume that it was and carry on.

  AudioSessionSetActive(true);
  NSLog(@"AudioSession === starting Audio Session.");

  // print value of properties to check that everything was set properly
  Float64 audioSessionProperty64 = 0;
  Float32 audioSessionProperty32 = 0;
  UInt32 audioSessionPropertySize64 = sizeof(audioSessionProperty64);
  UInt32 audioSessionPropertySize32 = sizeof(audioSessionProperty32);
  AudioSessionGetProperty(kAudioSessionProperty_CurrentHardwareSampleRate,
  &audioSessionPropertySize64, &audioSessionProperty64);
  NSLog(@"AudioSession === CurrentHardwareSampleRate: %.0fHz", audioSessionProperty64);
  sampleRate = audioSessionProperty64;

  AudioSessionGetProperty(kAudioSessionProperty_CurrentHardwareIOBufferDuration,
  &audioSessionPropertySize32, &audioSessionProperty32);
  int blockSize = lrint(audioSessionProperty32 * audioSessionProperty64);
  NSLog(@"AudioSession === CurrentHardwareIOBufferDuration: %3.2fms", audioSessionProperty32*1000.0f);
  NSLog(@"AudioSession === block size: %i", blockSize);
}

- (void)initializeAudioUnit
{
  // http://developer.apple.com/iphone/library/documentation/Audio/Conceptual/AudioUnitLoadingGuide_iPhoneOS/AccessingAudioUnits/LoadingIndividualAudioUnits.html#//apple_ref/doc/uid/TP40008781-CH103-SW11

  // create an AudioComponentDescription describing a RemoteIO audio unit
  // such a component provides an interface from microphone to speaker
  AudioComponentDescription auDescription;
  auDescription.componentType = kAudioUnitType_Output;
  auDescription.componentSubType = kAudioUnitSubType_RemoteIO;
  auDescription.componentManufacturer = kAudioUnitManufacturer_Apple;
  auDescription.componentFlags = 0;
  auDescription.componentFlagsMask = 0;

  // find an audio component fitting the given description
  AudioComponent foundComponent = AudioComponentFindNext(NULL, &auDescription);

  // create a new audio unit instance
  AudioComponentInstanceNew(foundComponent, &audioUnit);

  // connect the AU to hardware input and output
  OSStatus err = 0; // http://developer.apple.com/iphone/library/documentation/AudioUnit/Reference/AUComponentServicesReference/Reference/reference.html
  UInt32 doSetProperty = 1;
  AudioUnitElement inputBus = 1;
  AudioUnitElement outputBus = 0;
  // connect the AU to the microphone
  AudioUnitSetProperty(audioUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input,
  inputBus, &doSetProperty, sizeof(doSetProperty));
  // connect the AU to the soundout
  AudioUnitSetProperty(audioUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output,
  outputBus, &doSetProperty, sizeof(doSetProperty));

  // set the sample rate on the input and output busses
  AudioUnitSetProperty(audioUnit, kAudioUnitProperty_SampleRate, kAudioUnitScope_Input, outputBus,
  &sampleRate, sizeof(sampleRate));
  AudioUnitSetProperty(audioUnit, kAudioUnitProperty_SampleRate, kAudioUnitScope_Output, inputBus,
  &sampleRate, sizeof(sampleRate));

  AudioStreamBasicDescription toneStreamFormatInput;
  memset (&toneStreamFormatInput, 0, sizeof (toneStreamFormatInput)); // clear all fields
  toneStreamFormatInput.mSampleRate       = sampleRate;
  toneStreamFormatInput.mFormatID         = kAudioFormatLinearPCM;
  toneStreamFormatInput.mFormatFlags      = kAudioFormatFlagsCanonical;
  toneStreamFormatInput.mFramesPerPacket  = 1;
  toneStreamFormatInput.mBytesPerPacket = toneStreamFormatInput.mBytesPerFrame = sizeof(AudioUnitSampleType);
  toneStreamFormatInput.mChannelsPerFrame = numInputChannels;
  toneStreamFormatInput.mBitsPerChannel = 8 * sizeof(AudioUnitSampleType);
  toneStreamFormatInput.mFormatFlags |= kAudioFormatFlagIsNonInterleaved;

  // apply the audio data stream format to bus 0 of the input scope of the Remote I/O AU. This is
  // actually the OUTPUT to the system.
  err = AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, outputBus,
  &toneStreamFormatInput, sizeof(toneStreamFormatInput));

  AudioStreamBasicDescription toneStreamFormatOutput;
  memset (&toneStreamFormatOutput, 0, sizeof(toneStreamFormatOutput));
  toneStreamFormatOutput.mSampleRate       = sampleRate;
  toneStreamFormatOutput.mFormatID         = kAudioFormatLinearPCM;
  toneStreamFormatOutput.mFormatFlags      = kAudioFormatFlagsCanonical;
  toneStreamFormatOutput.mFramesPerPacket  = 1;
  toneStreamFormatOutput.mBytesPerPacket = toneStreamFormatOutput.mBytesPerFrame = sizeof(AudioUnitSampleType);
  toneStreamFormatOutput.mChannelsPerFrame = numOutputChannels;
  toneStreamFormatOutput.mBitsPerChannel = 8 * sizeof(AudioUnitSampleType);
  toneStreamFormatOutput.mFormatFlags |= kAudioFormatFlagIsNonInterleaved;

  AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, inputBus,
  &toneStreamFormatOutput, sizeof(toneStreamFormatOutput));

  // register the render callback. This is the function that the audio unit calls when it needs audio
  // the callback function (renderCallback()) is defined at the top of the page.
  AURenderCallbackStruct renderCallbackStruct;
  renderCallbackStruct.inputProc = renderCallback;
  renderCallbackStruct.inputProcRefCon = self; // this is an optional data pointer
  // pass the AudioController object

  AudioUnitSetProperty(audioUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input,
  outputBus, &renderCallbackStruct, sizeof(renderCallbackStruct));

  // disable buffer allocation on output (necessary?)
  // http://developer.apple.com/iphone/library/documentation/Audio/Conceptual/AudioUnitLoadingGuide_iPhoneOS/AccessingAudioUnits/LoadingIndividualAudioUnits.html#//apple_ref/doc/uid/TP40008781-CH103-SW19
  doSetProperty = 0;
  AudioUnitSetProperty(audioUnit, kAudioUnitProperty_ShouldAllocateBuffer, kAudioUnitScope_Output,
  outputBus, &doSetProperty, sizeof(doSetProperty));

  // finally, initialise the audio unit. It is ready to go.
  AudioUnitInitialize(audioUnit);

  // ensure that all parameters and settings have been successfully applied
  AudioStreamBasicDescription toneStreamFormatInputTest;
  memset(&toneStreamFormatInputTest, 0, sizeof(toneStreamFormatInputTest));
  UInt32 toneStreamFormatSize = sizeof(AudioStreamBasicDescription);
  AudioUnitGetProperty(audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input,
  outputBus, &toneStreamFormatInputTest, &toneStreamFormatSize);
  NSLog(@"=== input stream format:");
  NSLog(@"  mSampleRate: %.0fHz", toneStreamFormatInputTest.mSampleRate);
  NSLog(@"  mFormatID: %i", toneStreamFormatInputTest.mFormatID);
  NSLog(@"  mFormatFlags: %i", toneStreamFormatInputTest.mFormatFlags);
  NSLog(@"  mBytesPerPacket: %i", toneStreamFormatInputTest.mBytesPerPacket);
  NSLog(@"  mFramesPerPacket: %i", toneStreamFormatInputTest.mFramesPerPacket);
  NSLog(@"  mBytesPerFrame: %i", toneStreamFormatInputTest.mBytesPerFrame);
  NSLog(@"  mChannelsPerFrame: %i", toneStreamFormatInputTest.mChannelsPerFrame);
  NSLog(@"  mBitsPerChannel: %i", toneStreamFormatInputTest.mBitsPerChannel);

  AudioStreamBasicDescription toneStreamFormatOutputTest;
  memset (&toneStreamFormatOutputTest, 0, sizeof(toneStreamFormatOutputTest));
  AudioUnitGetProperty(audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output,
  inputBus, &toneStreamFormatOutputTest, &toneStreamFormatSize);

  NSLog(@"=== output stream format:");
  NSLog(@"  mSampleRate: %.0fHz", toneStreamFormatOutputTest.mSampleRate);
  NSLog(@"  mFormatID: %i", toneStreamFormatOutputTest.mFormatID);
  NSLog(@"  mFormatFlags: %i", toneStreamFormatOutputTest.mFormatFlags);
  NSLog(@"  mBytesPerPacket: %i", toneStreamFormatOutputTest.mBytesPerPacket);
  NSLog(@"  mFramesPerPacket: %i", toneStreamFormatOutputTest.mFramesPerPacket);
  NSLog(@"  mBytesPerFrame: %i", toneStreamFormatOutputTest.mBytesPerFrame);
  NSLog(@"  mChannelsPerFrame: %i", toneStreamFormatOutputTest.mChannelsPerFrame);
  NSLog(@"  mBitsPerChannel: %i", toneStreamFormatOutputTest.mBitsPerChannel);

  Float64 auSampleRate = 0.0;
  UInt32 sampleRateSize = sizeof(auSampleRate);
  AudioUnitGetProperty(audioUnit, kAudioUnitProperty_SampleRate, kAudioUnitScope_Input, outputBus, &auSampleRate, &sampleRateSize);
  NSLog(@"=== input sample rate: %.0fHz", auSampleRate);

  AudioUnitGetProperty(audioUnit, kAudioUnitProperty_SampleRate, kAudioUnitScope_Output, inputBus, &auSampleRate, &sampleRateSize);
// NSLog(@"=== output sample rate: %.0fHz", auSampleRate);
}

@end
