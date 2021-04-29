/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
 */

#pragma once

/**
 * @file
 * @copydoc SynthVoice
 */

#include <array>
#include <vector>
#include <stdint.h>

#include "ptrlist.h"

#include "IPlugConstants.h"
#include "IPlugMidi.h"
#include "IPlugLogger.h"

#include "IPlugQueue.h"
#include "ControlRamp.h"

BEGIN_IPLUG_NAMESPACE

/** A generic synthesizer voice to be controlled by a voice allocator. */
namespace voiceControlNames
{
  /** This enum names the control ramps by which we connect a controller to a synth voice. */
  enum eControlNames
  {
    kVoiceControlGate = 0,
    kVoiceControlPitch,
    kVoiceControlPitchBend,
    kVoiceControlPressure,
    kVoiceControlTimbre,
    kNumVoiceControlRamps
  };
}

using namespace voiceControlNames;

using VoiceInputs = ControlRamp::RampArray<kNumVoiceControlRamps>;

#pragma mark - Voice class

class SynthVoice
{
public:

  virtual ~SynthVoice() {};

  /** @return true if voice is generating any audio. */
  virtual bool GetBusy() const = 0;

  /** Trigger is called by the VoiceAllocator when a new voice should start, or if the voice limit has been hit and an existing voice needs to re-trigger. While the VoiceInputs are sufficient to control a voice from the VoiceAllocator, this method can be used to do additional tasks like resetting oscillators.
   * @param level Normalised starting level for this voice, derived from the velocity of the keypress, or in the case of a re-trigger the existing level \todo check
   * @param isRetrigger If this is \c true it means the voice is being re-triggered, and you should accommodate for this in your algorithm */
  virtual void Trigger(double level, bool isRetrigger) {};

  /** As with Trigger, called to do optional tasks when a voice is released. */
  virtual void Release() {};

  /** Process a block of audio data for the voice
   @param inputs Pointer to input channel arrays. Sometimes synthesisers have audio inputs. Alternatively you can pass in modulation from global LFOs etc here.
   @param outputs Pointer to output channel arrays. You should add to the existing data in these arrays (so that all the voices get summed)
   @param nInputs The number of input channels that contain valid data
   @param nOutputs The number of output channels that contain valid data
   @param startIdx The start index of the block of samples to process
   @param nFrames The number of samples to process in this block */
  virtual void ProcessSamplesAccumulating(sample** inputs, sample** outputs, int nInputs, int nOutputs, int startIdx, int nFrames)
  {
    for (auto c = 0; c < nOutputs; c++)
    {
      for (auto s = startIdx; s < startIdx + nFrames; s++)
      {
        outputs[c][s] += 0.; // if you are following this no-op example, remember you need to accumulate the output of all the different voices
      }
    }
  }

  /** Implement this if you need to do work when the sample rate or block size changes.
   * @param sampleRate The new sample rate
   * @param blockSize The new block size in samples */
  virtual void SetSampleRateAndBlockSize(double sampleRate, int blockSize) {};

  /** Implement this to allow picking a sound program from an integer index, as with MIDI.
   * @param pgm The new program number */
  virtual void SetProgramNumber(int pgm) {};

  /** Implement this to respond to control numbers for which there are not ramps. A synthesizer could use its own ramps internally if needed. 
   * @param controlNumber The MIDI controller number
   * @param value The normalized value
   */
  virtual void SetControl(int controlNumber, float value) {};

protected:
  VoiceInputs mInputs;
  int64_t mLastTriggeredTime{-1};
  uint8_t mVoiceNumber{0};
  uint8_t mZone{0};
  uint8_t mChannel{0};
  uint8_t mKey{0};
  double mBasePitch{0.};
  double mGain{0.}; // used by voice allocator to hard-kill voices.

  friend class MidiSynth;
  friend class VoiceAllocator;
};

END_IPLUG_NAMESPACE
