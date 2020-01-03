/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
 */

#pragma once

/**
 * @file
 * @copydoc VoiceAllocator
 */

#include <array>
#include <vector>
#include <stdint.h>
#include <functional>
#include <bitset>
//#include <iostream>

#include "IPlugLogger.h"
#include "IPlugQueue.h"

#include "SynthVoice.h"

BEGIN_IPLUG_NAMESPACE

using namespace voiceControlNames;

struct VoiceAddress
{
  uint8_t mZone;
  uint8_t mChannel;
  uint8_t mKey;
  uint8_t mFlags;
};

const uint8_t kAllZones = UCHAR_MAX;
const uint8_t kAllChannels = UCHAR_MAX;
const uint8_t kAllKeys = UCHAR_MAX;

// flags
const uint8_t kVoicesBusy = 1 << 0;
const uint8_t kVoicesMostRecent = 1 << 1;
const uint8_t kVoicesAll = 1 << 2;

enum EVoiceAction
{
  kNullAction = 0,
  kNoteOnAction,
  kNoteOffAction,
  kPitchBendAction,
  kPressureAction,
  kTimbreAction,
  kSustainAction,
  kControllerAction,
  kProgramChangeAction
};

/** A VoiceInputEvent describes a change in input to be applied to one more more voices.
 * mAddress specifies which voices should receive the change.
 * mAction is the type of property change.
 * mControllerNumber is the controller number to change if mAction is kController.
 * mValue is the new value associated with the change.
 * mSampleOffset is the number of samples into a processing buffer at which the change should occur.
 */

struct VoiceInputEvent
{
  VoiceAddress mAddress;
  EVoiceAction mAction;
  int mControllerNumber;
  float mValue;
  int mSampleOffset;
};

#pragma mark - VoiceAllocator class

class VoiceAllocator final
{
public:

  enum EATMode
  {
    kATModeChannel = 0,
    kATModePoly,
    kNumATModes
  };

  enum EPolyMode
  {
    kPolyModePoly = 0,
    kPolyModeMono,
    kNumPolyModes
  };

  static constexpr int kVoiceMostRecent = 1 << 7;

  // one voice worth of ramp generators
  using VoiceControlRamps = ControlRampProcessor::ProcessorArray<kNumVoiceControlRamps>;

  VoiceAllocator();
  ~VoiceAllocator();

  VoiceAllocator(const VoiceAllocator&) = delete;
  VoiceAllocator& operator=(const VoiceAllocator&) = delete;

  void Clear();

  void SetSampleRateAndBlockSize(double sampleRate, int blockSize) { mSampleRate = sampleRate; CalcGlideTimesInSamples(); }
  void SetNoteGlideTime(double t) { mNoteGlideTime = t; CalcGlideTimesInSamples(); }
  void SetControlGlideTime(double t) { mControlGlideTime = t; CalcGlideTimesInSamples(); }

  /** Add a synth voice to the allocator. We do not take ownership ot the voice.
   @param pv Pointer to the voice to add.
   @param zone A zone can be specified to make multitimbral synths.
   */
  void AddVoice(SynthVoice* pv, uint8_t zone);

  /** Add a single event to the input queue for the current processing block.
   */
  void AddEvent(VoiceInputEvent e) { mInputQueue.Push(e); }

  /** Process all input events and generate voice outputs.
   */
  void ProcessEvents(int samples, int64_t sampleTime);

  /** Turn all voice gates off, allowing any voice envelopes to finish.
   */
  void SoftKillAllVoices();

  /** Stop all voices from making sound immdiately.
   */
  void HardKillAllVoices();

  void SetKeyToPitchFunction(const std::function<float(int)>& fn) {mKeyToPitchFn = fn;}

  /** Send the event to the voices matching its address.
   */
  void SendEventToVoices(VoiceInputEvent event);

  void ProcessVoices(sample** inputs, sample** outputs, int nInputs, int nOutputs, int startIndex, int blockSize);

  size_t GetNVoices() const {return mVoicePtrs.size();}
  SynthVoice* GetVoice(int voiceIndex) const {return mVoicePtrs[voiceIndex];}
  void SetPitchOffset(float offset) { mPitchOffset = offset; }

private:
  using VoiceBitsArray = std::bitset<UCHAR_MAX>;

  VoiceBitsArray VoicesMatchingAddress(VoiceAddress va);

  void SendControlToVoiceInputs(VoiceBitsArray v, int ctlIdx, float val, int glideSamples);
  void SendControlToVoicesDirect(VoiceBitsArray v, int ctlIdx, float val);
  void SendProgramChangeToVoices(VoiceBitsArray v, int pgm);

  void StartVoice(int voiceIdx, int channel, int key, float pitch, float velocity, int sampleOffset, int64_t sampleTime, bool retrig);
  void StartVoices(VoiceBitsArray voices, int channel, int key, float pitch, float velocity, int sampleOffset, int64_t sampleTime, bool retrig);

  void StopVoice(int voiceIdx, int sampleOffset);
  void StopVoices(VoiceBitsArray voices, int sampleOffset);

  void CalcGlideTimesInSamples();
  void ClearVoiceInputs(SynthVoice* pVoice);
  int FindFreeVoiceIndex(int startIndex) const;
  int FindVoiceIndexToSteal(int64_t sampleTime) const;

  void NoteOn(VoiceInputEvent e, int64_t sampleTime);
  void NoteOff(VoiceInputEvent e, int64_t sampleTime);

  IPlugQueue<VoiceInputEvent> mInputQueue{1024};

  std::vector<SynthVoice*> mVoicePtrs;
  std::vector<std::unique_ptr<VoiceControlRamps>> mVoiceGlides;
  std::vector<int> mHeldKeys; // The currently physically held keys on the keyboard
  std::vector<int> mSustainedNotes; // Any notes that are sustained, including those that are physically held

  std::function<float(int)> mKeyToPitchFn;
  double mPitchOffset{0.};

  double mNoteGlideTime{0.};
  double mControlGlideTime{0.01};
  int mNoteGlideSamples{0}; // glide for note-to-note portamento
  int mControlGlideSamples{0}; // glide for controls including pitch bend
  double mSampleRate;
  int mBlockSize;

  bool mRotateVoices{true};
  int mVoiceRotateIndex{0};
  bool mSustainPedalDown{false};
  float mModWheel{0.f};
  float mMinHeldVelocity{1.f};

public:
  EPolyMode mPolyMode {kPolyModePoly};
  EATMode mATMode {kATModeChannel};
};

END_IPLUG_NAMESPACE
