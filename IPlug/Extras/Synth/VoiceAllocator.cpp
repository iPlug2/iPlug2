/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
 */

#include "VoiceAllocator.h"

#include <algorithm>
#include <numeric>
#include <iostream>

using namespace iplug;

std::ostream& operator<< (std::ostream& out, const VoiceInputEvent& r)
{
  out << "[z" << (int)r.mAddress.mZone << " c" << (int)r.mAddress.mChannel << " k" << (int)r.mAddress.mKey << " f" << (int)r.mAddress.mFlags << "]"  ;
  out << "[action:" << r.mAction << " ctl:" << r.mControllerNumber << " val:" << r.mValue << " off:" << r.mSampleOffset << "]";
  return out;
}

VoiceAllocator::VoiceAllocator()
{
  // setup default key->pitch fn
  mKeyToPitchFn = [](int k){return (k - 69.f)/12.f;};

  mSustainedNotes.reserve(128);
  mHeldKeys.reserve(128);
}

VoiceAllocator::~VoiceAllocator()
{
}

void VoiceAllocator::Clear()
{
  mHeldKeys.clear();
  mSustainedNotes.clear();
  HardKillAllVoices();
}

void VoiceAllocator::ClearVoiceInputs(SynthVoice* pVoice)
{
  for(int i=0; i<kNumVoiceControlRamps; ++i)
  {
    pVoice->mInputs[i].Clear();
  }
}

void VoiceAllocator::AddVoice(SynthVoice* pVoice, uint8_t zone)
{
  if(mVoicePtrs.size() + 1 < UCHAR_MAX)
  {
    mVoicePtrs.push_back(pVoice);
    ClearVoiceInputs(pVoice);
    pVoice->mKey = -1;
    pVoice->mZone = zone;

    // make a glides structures for the control ramps of the new voice
    mVoiceGlides.emplace_back(ControlRampProcessor::Create(pVoice->mInputs));
  }
  else
  {
    throw std::runtime_error{"VoiceAllocator: max voices exceeded!"};
  }
}

VoiceAllocator::VoiceBitsArray VoiceAllocator::VoicesMatchingAddress(VoiceAddress addr)
{
  const int n = static_cast<int>(mVoicePtrs.size());
  VoiceBitsArray v;

  // set all bits to true
  for(int i=0; i<n; ++i)
  {
    v[i] = true;
  }

  // for each criterion present in address, clear any voice bits not matching

  // zone
  if(addr.mZone != kAllZones)
  {
    for(int i=0; i<n; ++i)
    {
      v[i] = v[i] & (mVoicePtrs[i]->mZone == addr.mZone);
    }
  }

  // setting the flag kVoicesAll returns all voices matching the zone of the address.
  if(addr.mFlags & kVoicesAll) return v;

  // channel
  if(addr.mChannel != kAllChannels)
  {
    for(int i=0; i<n; ++i)
    {
      v[i] = v[i] & (mVoicePtrs[i]->mChannel == addr.mChannel);
    }
  }

  // Key
  if(addr.mKey != kAllKeys)
  {
    for(int i=0; i<n; ++i)
    {
      v[i] = v[i] & (mVoicePtrs[i]->mKey == addr.mKey);
    }
  }

  // busy flag
  if(addr.mFlags & kVoicesBusy)
  {
    for(int i=0; i<n; ++i)
    {
      v[i] = v[i] & mVoicePtrs[i]->GetBusy();
    }
  }

  // most recent
  if(addr.mFlags & kVoicesMostRecent)
  {
    int64_t maxT = -1;
    int maxIdx = -1;
    for(int i=0; i<n; ++i)
    {
      if(v[i])
      {
        int64_t vt = mVoicePtrs[i]->mLastTriggeredTime;
        if(vt > maxT)
        {
          maxT = vt;
          maxIdx = i;
        }
      }
    }

    // set all bits to false
    //v.reset();
    for(int i=0; i<n; ++i)
    {
      v[i] = 0;
    }

    if(maxIdx >= 0)
    {
      v[maxIdx] = true;
    }
  }
  return v;
}

void VoiceAllocator::SendControlToVoiceInputs(VoiceBitsArray v, int ctlIdx, float val, int glideSamples)
{
  // send control change to all matched voices through glide generators
  for(int i=0; i<mVoicePtrs.size(); ++i)
  {
    if(v[i])
    {
      mVoiceGlides[i]->at(ctlIdx).SetTarget(val, 0, glideSamples, mBlockSize);
    }
  }
}

void VoiceAllocator::SendControlToVoicesDirect(VoiceBitsArray v, int ctlIdx, float val)
{
  // send generic control change directly to voice
  for(int i=0; i<mVoicePtrs.size(); ++i)
  {
    if(v[i])
    {
      mVoicePtrs[i]->SetControl(ctlIdx, val);
    }
  }
}

void VoiceAllocator::SendProgramChangeToVoices(VoiceBitsArray v, int pgm)
{
  for(int i=0; i<mVoicePtrs.size(); ++i)
  {
    if(v[i])
    {
      mVoicePtrs[i]->SetProgramNumber(pgm);
    }
  }
}

void VoiceAllocator::ProcessEvents(int blockSize, int64_t sampleTime)
{
  while(mInputQueue.ElementsAvailable())
  {
    VoiceInputEvent event;
    mInputQueue.Pop(event);
    VoiceAllocator::VoiceBitsArray voices = VoicesMatchingAddress(event.mAddress);

    switch(event.mAction)
    {
      case kNoteOnAction:
      {
        NoteOn(event, sampleTime);
        break;
      }
      case kNoteOffAction:
      {
        if(event.mAddress.mFlags == kVoicesAll)
        {
          SoftKillAllVoices();
        }
        else
        {
          NoteOff(event, sampleTime);
        }
        break;
      }
      case kPitchBendAction:
      {
        SendControlToVoiceInputs(voices, kVoiceControlPitchBend, event.mValue, mControlGlideSamples);
        break;
      }
      case kPressureAction:
      {
        SendControlToVoiceInputs(voices, kVoiceControlPressure, event.mValue, mControlGlideSamples);
        break;
      }
      case kTimbreAction:
      {
        SendControlToVoiceInputs(voices, kVoiceControlTimbre, event.mValue, mControlGlideSamples);
        break;
      }
      case kSustainAction:
      {
        mSustainPedalDown = (bool) (event.mValue >= 0.5);
        if (!mSustainPedalDown) // sustain pedal released
        {
          // if notes are sustaining, check that they're not still held and if not then stop voice
          if (!mSustainedNotes.empty())
          {
            for (auto susNotesItr = mSustainedNotes.begin(); susNotesItr != mSustainedNotes.end();)
            {
              uint8_t key = *susNotesItr;
              bool held = std::find(mHeldKeys.begin(), mHeldKeys.end(), key) != mHeldKeys.end();
              if (!held)
              {
                StopVoices(VoicesMatchingAddress({event.mAddress.mZone, kAllChannels, key, 0}), event.mSampleOffset);
                susNotesItr = mSustainedNotes.erase(susNotesItr);
              }
              else
                susNotesItr++;
            }
          }
        }
        break;
      }
      case kControllerAction:
      {
        // called for any continuous controller other than the special #74 specified in MPE
        SendControlToVoicesDirect(voices, event.mControllerNumber, event.mValue);
        break;
      }
      case kProgramChangeAction:
      {
        SendProgramChangeToVoices(voices, event.mControllerNumber);
        break;
      }
      case kNullAction:
      default:
      {
        break;
      }
    }
  }

  // update any glides in progress, writing voice control outputs
  for(auto& glides : mVoiceGlides)
  {
    for(int i=0; i<kNumVoiceControlRamps; ++i)
    {
      glides->at(i).Process(blockSize);
    }
  }
}

void VoiceAllocator::CalcGlideTimesInSamples()
{
  mNoteGlideSamples = static_cast<int>(mNoteGlideTime * mSampleRate);
  mControlGlideSamples = static_cast<int>(mControlGlideTime * mSampleRate);
}

int VoiceAllocator::FindFreeVoiceIndex(int startIndex) const
{
  size_t voices = mVoicePtrs.size();
  for(int i=0; i<voices; ++i)
  {
    int j = (startIndex + i)%voices;
    SynthVoice* pv = mVoicePtrs[j];
    if(!pv->GetBusy())
    {
      return j;
    }
  }
  return -1;
}

int VoiceAllocator::FindVoiceIndexToSteal(int64_t sampleTime) const
{
  size_t voices = mVoicePtrs.size();
  int64_t earliestTime = sampleTime;
  int longestPlayingVoiceIdx = 0;
  for(int i=0; i<voices; ++i)
  {
    SynthVoice* pv = mVoicePtrs[i];
    if(pv->mLastTriggeredTime < earliestTime)
    {
      earliestTime = pv->mLastTriggeredTime;
      longestPlayingVoiceIdx = i;
    }
  }
  return longestPlayingVoiceIdx;
}

// start a single voice and set its current channel and key.
void VoiceAllocator::StartVoice(int voiceIdx, int channel, int key, float pitch, float velocity, int sampleOffset, int64_t sampleTime, bool retrig)
{
  if(!retrig)
  {
    // add immediate sample-accurate change for trigger
    mVoiceGlides[voiceIdx]->at(kVoiceControlGate).SetTarget(velocity, sampleOffset, 1, mBlockSize);
  }

  // add glide for pitch
  mVoiceGlides[voiceIdx]->at(kVoiceControlPitch).SetTarget(pitch, sampleOffset, mNoteGlideSamples, mBlockSize);

  // set things directly in voice
  SynthVoice* pVoice = mVoicePtrs[voiceIdx];
  pVoice->mLastTriggeredTime = sampleTime;
  pVoice->mChannel = channel;
  pVoice->mKey = key;
  pVoice->mGain = 1.;

  // call voice's Trigger method
  pVoice->Trigger(velocity, retrig);
}

// start all of the voice indexes marked in the VoieBitsArray and set the current channel and key of each.
void VoiceAllocator::StartVoices(VoiceBitsArray vbits, int channel, int key, float pitch, float velocity, int sampleOffset, int64_t sampleTime, bool retrig)
{
  for(int i=0; i<mVoicePtrs.size(); ++i)
  {
    if(vbits[i])
    {
      StartVoice(i, channel, key, pitch, velocity, sampleOffset, sampleTime, retrig);
    }
  }
}

void VoiceAllocator::StopVoice(int voiceIdx, int sampleOffset)
{
  mVoiceGlides[voiceIdx]->at(kVoiceControlGate).SetTarget(0.0, sampleOffset, 1, mBlockSize);
  mVoicePtrs[voiceIdx]->mKey = -1;
  mVoicePtrs[voiceIdx]->Release();
}

// stop all voices marked in the VoiceBitsArray.
void VoiceAllocator::StopVoices(VoiceBitsArray vbits, int sampleOffset)
{
  for(int i=0; i<mVoicePtrs.size(); ++i)
  {
    if(vbits[i])
    {
      StopVoice(i, sampleOffset);
    }
  }
}

void VoiceAllocator::SoftKillAllVoices()
{
  mHeldKeys.clear();
  mSustainedNotes.clear();
  mSustainPedalDown = false;

  size_t voices = mVoicePtrs.size();
  for (int v = 0; v < voices; v++)
  {
    StopVoice(v, 0);
  }
}

void VoiceAllocator::HardKillAllVoices()
{
  SoftKillAllVoices();
  for (int v = 0; v < mVoicePtrs.size(); v++)
  {
    mVoicePtrs[v]->mGain = 0.;
  }
}

void VoiceAllocator::NoteOn(VoiceInputEvent e, int64_t sampleTime)
{
  int channel = e.mAddress.mChannel;
  int key = e.mAddress.mKey;
  int offset = e.mSampleOffset;
  float velocity = e.mValue;
  float pitch = mKeyToPitchFn(key + static_cast<int>(mPitchOffset));

  switch(mPolyMode)
  {
    case kPolyModeMono:
    {
      // TODO retrig / legato
      bool retrig = false;

      // trigger all voices in zone
      StartVoices(VoicesMatchingAddress({e.mAddress.mZone, kAllChannels, kAllKeys, 0}), channel, key, pitch, velocity, offset, sampleTime, retrig);

      // in mono modes only ever 1 sustained note
      mSustainedNotes.clear();
      break;
    }
    case kPolyModePoly:
    {
      int i = FindFreeVoiceIndex(mVoiceRotateIndex);
      if(i < 0)
      {
        i = FindVoiceIndexToSteal(sampleTime);
      }
      if(mRotateVoices)
      {
        mVoiceRotateIndex = i + 1;
      }
      if(i >= 0)
      {
        bool retrig = false;
        StartVoice(i, channel, key, pitch, velocity, offset, sampleTime, retrig);
      }
      break;
    }

    default:
      break;
  }

  // add to held keys
  if(std::find(mHeldKeys.begin(), mHeldKeys.end(), key) == mHeldKeys.end())
  {
    mHeldKeys.push_back(key);
    mMinHeldVelocity = std::min(velocity, mMinHeldVelocity);
  }

  // add to sustained notes
  if(std::find(mSustainedNotes.begin(), mSustainedNotes.end(), key) == mSustainedNotes.end())
  {
    mSustainedNotes.push_back(key);
  }
}

void VoiceAllocator::NoteOff(VoiceInputEvent e, int64_t sampleTime)
{
  int channel = e.mAddress.mChannel;
  int key = e.mAddress.mKey;
  int offset = e.mSampleOffset;

  // remove from held keys
  mHeldKeys.erase(std::remove(mHeldKeys.begin(), mHeldKeys.end(), key), mHeldKeys.end());
  if(mHeldKeys.empty())
  {
    mMinHeldVelocity = 1.0f;
  }

  if(mPolyMode == kPolyModeMono)
  {
    bool doPlayQueuedKey = false;
    int queuedKey = 0;

    // if there are still held keys...
    if(!mHeldKeys.empty())
    {
      queuedKey = mHeldKeys.back();
      if (queuedKey != mVoicePtrs[0]->mKey)
      {
        doPlayQueuedKey = true;
        if(mSustainPedalDown)
        {
          // in mono modes only ever 1 sustained note
          mSustainedNotes.clear();
          mSustainedNotes.push_back(queuedKey);
        }
      }
    }
    else if(mSustainPedalDown)
    {
      if(!mSustainedNotes.empty())
      {
        queuedKey = mSustainedNotes.back();
        if (queuedKey != mVoicePtrs[0]->mKey)
        {
          doPlayQueuedKey = true;
        }
      }
    }
    else
    {
      // there are no held keys, so no voices in the zone should be playing.
      StopVoices(VoicesMatchingAddress({e.mAddress.mZone, kAllChannels, kAllKeys, 0}), offset);
    }

    if(doPlayQueuedKey)
    {
      // trigger the queued key for all voices in the zone at the minimum held velocity.
      // alternatively the release velocity of the note off could be used here.
      float pitch = mKeyToPitchFn(queuedKey + static_cast<int>(mPitchOffset));
      bool retrig = false;

      StartVoices(VoicesMatchingAddress({e.mAddress.mZone, kAllChannels, kAllKeys, 0}), channel, queuedKey, pitch, mMinHeldVelocity, offset, sampleTime, retrig);
    }
  }
  else // poly
  {
    if (!mSustainPedalDown)
    {
      StopVoices(VoicesMatchingAddress(e.mAddress), e.mSampleOffset);
      mSustainedNotes.erase(std::remove(mSustainedNotes.begin(), mSustainedNotes.end(), key), mSustainedNotes.end());
    }
  }
}

void VoiceAllocator::ProcessVoices(sample** inputs, sample** outputs, int nInputs, int nOutputs, int startIndex, int blockSize)
{
  for(auto pVoice : mVoicePtrs)
  {
    // TODO distribute voices across cores
    if(pVoice->GetBusy())
    {
      pVoice->ProcessSamplesAccumulating(inputs, outputs, nInputs, nOutputs, startIndex, blockSize);
    }
  }
}
