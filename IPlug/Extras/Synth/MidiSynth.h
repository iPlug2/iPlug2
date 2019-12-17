/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
 */

#pragma once

/**
 * @file
 * @copydoc MidiSynth
 */

#include <array>
#include <vector>
#include <stdint.h>

#include "ptrlist.h"

#include "IPlugConstants.h"
#include "IPlugMidi.h"
#include "IPlugLogger.h"

#include "IPlugQueue.h"
#include "SynthVoice.h"
#include "VoiceAllocator.h"

#define DEBUG_VOICE_COUNT 0

BEGIN_IPLUG_NAMESPACE

/** A monophonic/polyphonic synthesiser base class which can be supplied with a custom voice.
 *  Supports different kinds of after touch, pitch bend, velocity and after touch curves, unison (currently monophonic mode only) */
class MidiSynth
{
public:
  /** This defines the size in samples of a single block of processing that will be done by the synth. */
  static constexpr int kDefaultBlockSize = 32;

#pragma mark - MidiSynth class

  MidiSynth(VoiceAllocator::EPolyMode mode, int blockSize = kDefaultBlockSize);
  ~MidiSynth();

  MidiSynth(const MidiSynth&) = delete;
  MidiSynth& operator=(const MidiSynth&) = delete;
    
  void Reset()
  {
    mSampleTime = 0;
    mVoiceAllocator.Clear();
  }

  void SetSampleRateAndBlockSize(double sampleRate, int blockSize);

  /** If you are using this class in a non-traditional mode of polyphony (e.g.to stack loads of voices) you might want to manually SetVoicesActive()
   * usually this would happen when you trigger notes
   * @param active should the class report that voices are active */
  void SetVoicesActive(bool active)
  {
    mVoicesAreActive = active;
  }

  void SetPolyMode(VoiceAllocator::EPolyMode mode)
  {
    mVoiceAllocator.mPolyMode = mode;
  }

  void SetATMode(VoiceAllocator::EATMode mode)
  {
    mVoiceAllocator.mATMode = mode;
  }

  /** Set this function to something other than the default
   * if you need to implement a tuning table for microtonal support
   * @param fn A function taking an integer key value and returning a double-precision
   *  pitch value, where 0.5 = 220Hz, 1.0 = 440 Hz, 2.0 = 880 Hz ("1v / octave"). */
  void SetKeyToPitchFn(const std::function<float(int)>& fn)
  {
    mVoiceAllocator.SetKeyToPitchFunction(fn);
  }

  void SetNoteOffset(double offset)
  {
    mVoiceAllocator.SetPitchOffset(static_cast<float>(offset));
  }

  void SetNoteGlideTime(double t)
  {
    mVoiceAllocator.SetNoteGlideTime(t);
  }

  void SetControlGlideTime(double t)
  {
    mVoiceAllocator.SetControlGlideTime(t);
  }

  SynthVoice* GetVoice(int voiceIdx)
  {
    return mVoiceAllocator.GetVoice(voiceIdx);
  }
  
  void ForEachVoice(std::function<void(SynthVoice& voice)> func)
  {
    for (auto v = 0; v < NVoices(); v++)
      func(*GetVoice(v));
  }
  
  size_t NVoices() const
  {
    return mVoiceAllocator.GetNVoices();
  }

  /** adds a SynthVoice to this MidiSynth, taking ownership of the object. */
  void AddVoice(SynthVoice* pVoice, uint8_t zone)
  {
    mVoiceAllocator.AddVoice(pVoice, zone);
  }

  void AddMidiMsgToQueue(const IMidiMsg& msg)
  {
    mMidiQueue.Add(msg);
  }

  /** Processes a block of audio samples
   * @param inputs Pointer to input Arrays
   * @param outputs Pointer to output Arrays
   * @param nInputs The number of input channels that contain valid data
   * @param nOutputs input channels that contain valid data
   * @param nFrames The number of sample frames to process
   * @return \c true if the synth is silent */
  bool ProcessBlock(sample** inputs, sample** outputs, int nInputs, int nOutputs, int nFrames);

private:

  // maintain the state for one MIDI channel including RPN receipt state and pitch bend range.
  struct ChannelState
  {
    uint8_t paramMSB;
    uint8_t paramLSB;
    uint8_t valueMSB;
    uint8_t valueLSB;
    uint8_t pitchBendRange; // in semitones
    float pitchBend;
    float pressure;
    float timbre;
  };

  // MPE helper functions
  const int kMPELowerZoneMasterChannel = 0;
  const int kMPEUpperZoneMasterChannel = 15;
  inline bool isMasterChannel(int c) { return ((c == 0)||(c == 15)); }
  bool isInLowerZone(int c) { return ((c > 0)&&(c < mMPELowerZoneChannels)); }
  bool isInUpperZone(int c) { return ((c < 15)&&(c > 15 - mMPEUpperZoneChannels)); }
  int MasterChannelFor(int memberChan) { return isInUpperZone(memberChan) ? kMPEUpperZoneMasterChannel : kMPELowerZoneMasterChannel; }
  int MasterZoneFor(int memberChan) { return isInUpperZone(memberChan) ? 1 : 0; }

  // handy functions for writing loops on lower and upper Zone member channels
  int lowerZoneStart() { return 1; }
  int lowerZoneEnd() { return mMPELowerZoneChannels - 1; }
  int upperZoneStart() {  return 15 - mMPEUpperZoneChannels; }
  int upperZoneEnd() { return 15; }

  void InitMPE();
  void SetMPEZones(int channel, int nChans);
  void SetChannelPitchBendRange(int channel, int range);

  VoiceInputEvent MidiMessageToEventBasic(const IMidiMsg& msg);
  VoiceInputEvent MidiMessageToEventMPE(const IMidiMsg& msg);
  VoiceInputEvent MidiMessageToEvent(const IMidiMsg& msg);
  void HandleRPN(IMidiMsg msg);

  // basic MIDI data

  VoiceAllocator mVoiceAllocator;
  uint16_t mUnisonVoices{1};
  IMidiQueue mMidiQueue;
  float mVelocityLUT[128];
  float mAfterTouchLUT[128];
  ChannelState mChannelStates[16]{};
  int mBlockSize;
  int64_t mSampleTime{0};
  double mSampleRate = DEFAULT_SAMPLE_RATE;
  bool mVoicesAreActive = false;

  // the synth will startup in basic MIDI mode. When an MPE Zone setup message is received, MPE mode is entered.
  // To leave MPE mode, use RPNs to set all MPE zone channel counts to 0 as per the MPE spec.
  bool mMPEMode{false};

  // MPE state - channels in zone including master channels.
  int mMPELowerZoneChannels{0};
  int mMPEUpperZoneChannels{0};
};

END_IPLUG_NAMESPACE

