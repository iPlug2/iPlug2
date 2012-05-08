// IPlugIOSAudio.h based on libpd ios audio wrapper
// If testing using the check the IPad/IPhone simulator, check the osx core audio sr matches the sr you are using


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

#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>
#import <Foundation/Foundation.h>

#define IOS_BLOCK_SIZE 64
#define IOS_STEREO
#define IOS_AUDIO_IN
#define IOS_AUDIO_SINGLE_PRECISION // comment out for 64bit processing

#ifdef IOS_STEREO
#define IOS_INCHANS 2
#define IOS_OUTCHANS 2
#else
#define IOS_INCHANS 1
#define IOS_OUTCHANS 1
#endif

#ifdef IOS_AUDIO_SINGLE_PRECISION
typedef float FLT;
#else
typedef Float64 FLT;
#endif

//TODO: can this be generic, i.e. IPlugBase.h?
#include "../IPlugMultiTargets.h"

@interface IPlugIOSAudio : NSObject
{
  AudioUnit audioUnit;
  int numInputChannels;
  int numOutputChannels;
  Float64 sampleRate;
  float microphoneVolume;
  FLT *floatBufferL;
  FLT *floatBufferR; // could ifdef this
  int floatBufferLength;
  IPlug *mPluginInstance;
  unsigned short midiOutChan;
}

@property (nonatomic, readonly) AudioUnit audioUnit;
@property (nonatomic, readonly) int numInputChannels;
@property (nonatomic, readonly) int numOutputChannels;
@property (nonatomic, readonly) Float64 sampleRate;
@property (nonatomic) float microphoneVolume;
@property (nonatomic, readonly) FLT *floatBufferL;
@property (nonatomic, readonly) FLT *floatBufferR;
@property (nonatomic, readonly) int floatBufferLength;


/** Initialising with a sample rate, ticks per buffer, number of input and output channels
 *
 * This init method assumes the AudioSession Category will be PlayAndRecord
 * note: PlayAndRecord doesn't work properly on some older iDevices with armv6 processors
 * (iPhone EDGE, iPhone 3G, iTouch 1g, iTouch 2g)
 * Use alternate init method in conjunction with an alternative AudioSession Category
 * for use with armv6 devices
 */
- (id)initWithIPlugInstance:(IPlug*)pluginInstance andSampleRate:(float)newSampleRate andTicksPerBuffer:(int)ticks
andNumberOfInputChannels:(int)inputChannels andNumberOfOutputChannels:(int)outputChannels;

/** Initialising with a sample rate, ticks per buffer, number of input and output channels
 * and AudioSession Category
 *
 * This alternate init method allow the specification of an AudioSession Category
 * note: PlayAndRecord AudioSession Category currently doesn't work properly on some older
 * iDevices with armv6 processors
 * (iPhone EDGE, iPhone 3G, iTouch 1g, iTouch 2g)
 */
- (id)initWithIPlugInstance:(IPlug*)pluginInstance andSampleRate:(float)newSampleRate andTicksPerBuffer:(int)ticks
andNumberOfInputChannels:(int)inputChannels andNumberOfOutputChannels:(int)outputChannels
andAudioSessionCategory:(UInt32)audioSessionCategory;

/** Begin audio/scene playback. To avoid clicks, you have to create a subclass and override this
 * function and add ramping or any other custom behaviour. Make sure to call the superclass method
 * as well.
 */
- (void)play;

/** Pause audio/scene playback.To avoid clicks, you have to create a subclass and override this
 * function and add ramping or any other custom behaviour. Make sure to call the superclass method
 * as well.
 */
- (void)pause;

@end
