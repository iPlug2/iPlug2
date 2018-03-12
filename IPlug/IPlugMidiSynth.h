#pragma once

/**
 * @file
 * @copydoc MIDISynth
 */

#include <vector>
#include <stdint.h>

#include "ptrlist.h"

#include "IPlugConstants.h"
#include "IPlugMidi.h"

#define DEBUG_VOICE_COUNT 0

/** A monophonic/polyphonic synthesiser base class which can be supplied with a custom voice.
 *  Supports different kinds of after touch, pitch bend, velocity and after touch curves, unison (currently monophonic mode only)
 *  NOTE: This is not currently particularly efficient, and needs a bit more work to be more generalisable */
class MidiSynth
{
public:
  struct KeyPressInfo
  {
    int mKey;
    double mVelNorm;

    KeyPressInfo(int key, double velNorm)
    : mKey(key)
    , mVelNorm(velNorm)
    {
    }

    friend bool operator==(const KeyPressInfo& lhs, const KeyPressInfo& rhs);
  };

  enum EATMode
  {
    kATModeChannel = 0,
    kATModePoly,
    kNumATModes
  };

  enum EPolyMode
  {
    kPolyModePoly = 0,
    kPolyModeLegato,
    kPolyModeMono,
    kNumPolyModes
  };

#pragma mark - Voice class
  class Voice
  {
  public:
    virtual bool GetBusy() const = 0;

    /** @return true if voice is free or amp envs are in release stage */
    virtual bool GetReleased() const = 0;

    /** Trigger is called when a new voice should start, or if the voice limit has been hit and an existing voice needs to re-trigger
     * @param level Normalised starting level for this voice, derived from the velocity of the keypress, or in the case of a re-trigger the existing level \todo check
     * @param isRetrigger If this is \c true it means the voices being re-triggered, and you should accommodate for this in your algorithm */
    virtual void Trigger(double level, bool isRetrigger) { DBGMSG("Voice Triggered\n"); }

    /** Release envelopes on note off message */
    virtual void Release() { DBGMSG("Voice Released\n"); }

    /** Kill a playing voice. Hard kill should kill voice immediately (potentially causing glitch)
     *  Soft kill should kill voice as quickly as possible with a fade out to avoid glitch */
    virtual void Kill(bool isSoft) { DBGMSG("Voice Hard Killed\n"); }

    /** Process a single sample of audio data for the voice
     * @param inputs Pointer to input channel arrays (sometimes synthesisers have audio inputs)
     * @param outputs Pointer to output channel arrays
     * @param sampleIdx The index of the sample to process */
    virtual void ProcessSample(sample** inputs, sample** outputs, int sampleIdx, double pitchBend)
    {
      outputs[0][sampleIdx] = 0.;
    }

    /** If you have members that need to update when the sample rate changes you can do that by overriding this method
     * @param sampleRate The new sample rate */
    virtual void SetSampleRate(double sampleRate) {};

  private:
    void RemovedFromKey()
    {
      mKey = -1;
      mAftertouch = 0.;
    }

  public:
    int64_t mStartTime = -1;
    bool mLastBusy = false;
    int mKey = -1;
    double mBasePitch = 0.;
    double mAftertouch = 0.;
    int mStackIdx = -1;
    
    friend class MidiSynth;
  };

public:
#pragma mark - Engine class
  MidiSynth(EPolyMode polyMode = kPolyModePoly);
  ~MidiSynth();

  void Reset()
  {
    mSampleTime = 0;
  }

  void SetSampleRateAndBlockSize(double sampleRate, int blockSize);

  void SetPitchBendRange(double rangeInSemitones)
  {
    mPitchBendRange = rangeInSemitones;
  }
  
  void SetPolyMode(EPolyMode mode)
  {
    mPolyMode = mode; //TODO: implement click safe solution
  }
  
  void SetATMode(EATMode mode)
  {
    mATMode = mode; //TODO: implement click safe solution
  }

  inline Voice* GetVoice(int voiceIdx)
  {
    return mVS.Get(voiceIdx);
  }

  int NVoices() const
  {
    return mVS.GetSize();
  }

  void AddVoice(Voice* pVoice)
  {
    mVS.Add(pVoice);
  }

  void AddMidiMsgToQueue(const IMidiMsg& msg)
  {
    mMidiQueue.Add(msg);
  }

  // return \c true if the synth is silent

  /** Processes a block of audio samples
   * @param inputs Pointer to input Arrays
   * @param outputs Pointer to output Arrays
   * @param nFrames The number of sample frames to process
   * @return \c true if the synth is silent */
  bool ProcessBlock(double** inputs, double** outputs, int nFrames);

protected:
  /** Override this method if you need to implement a tuning table for microtonal support
   * @param key The input MIDI pitch of the key pressed
   * @return The adjusted MIDI pitch */
  virtual double GetAdjustedPitch(int key)
  {
    return key;
  }
  
private:
  void NoteOnOffMono(const IMidiMsg& msg);
  
  void NoteOnOffPoly(const IMidiMsg& msg);
  
  inline void TriggerMonoNote(KeyPressInfo note);
  
  inline void StopVoicesForKey(int note)
  {
    // now stop voices associated with this key
    for (int v = 0; v < NVoices(); v++)
    {
      if (GetVoice(v)->mKey == note)
      {
        if (GetVoice(v)->GetBusy())
        {
          StopVoice(*GetVoice(v));
        }
      }
    }
  }
  
  inline void StopVoice(Voice& voice)
  {
    voice.Release();
    voice.RemovedFromKey();
  }
  
  inline void ReleaseAllVoices()
  {
    for (int v = 0; v < NVoices(); v++)
    {
      if (GetVoice(v)->GetBusy())
      {
        Voice* pVoice = GetVoice(v);
        pVoice->Release();
        pVoice->RemovedFromKey();
      }
    }
  }
  
  inline void SoftKillAllVoices()
  {
    for (int v = 0; v < NVoices(); v++)
    {
      Voice* pVoice = GetVoice(v);
      pVoice->Kill(true);
      pVoice->RemovedFromKey();
    }
  }
  
  inline void HardKillAllVoices()
  {
    for (int v = 0; v < NVoices(); v++)
    {
      Voice* pVoice = GetVoice(v);
      pVoice->Kill(false);
      pVoice->RemovedFromKey();
    }
  }
  
  inline int CheckKey(int key)
  {
    for(int v = 0; v < NVoices(); v++)
    {
      if(GetVoice(v)->mKey == key)
        return v;
    }
    
    return -1;
  }
  
  inline bool VoicesAreBusy()
  {
    for(int v = 0; v < NVoices(); v++)
    {
      if(GetVoice(v)->GetBusy())
        return true;
    }
    
    return false;
  }
  
  inline int FindFreeVoice()
  {
    for(int v = 0; v < NVoices(); v++)
    {
      if(!GetVoice(v)->GetBusy())
        return v;
    }
    
    int64_t mostRecentTime = mSampleTime;
    int longestPlayingVoice = -1;
    
    for(int v = 0; v < NVoices(); v++)
    {
      if (GetVoice(v)->mStartTime < mostRecentTime)
      {
        longestPlayingVoice = v;
        mostRecentTime = GetVoice(v)->mStartTime;
      }
    }
    
    return longestPlayingVoice;
  }
  
private:
  WDL_PtrList<Voice> mVS;

  int64_t mSampleTime = 0;
  double mSampleRate = DEFAULT_SAMPLE_RATE;
  double mPitchBendRange = 12.;
  double mPitchBend = 0.;
  double mModWheel = 0.; //TODO: not used
  double mPrevVelNorm = 0.; //TODO: not used

  bool mSustainPedalDown = false;
  bool mVoicesAreActive = false;
  uint16_t mUnisonVoices = 1;

  EPolyMode mPolyMode = kPolyModePoly; // mono note priority / polyphony
  EATMode mATMode = kATModeChannel;
  std::vector<KeyPressInfo> mHeldKeys; // The currently physically held keys on the keyboard
  std::vector<KeyPressInfo> mSustainedNotes; // Any notes that are sustained, including those that are physically held
  IMidiQueue mMidiQueue;

public: // these are public for state saving
  int mVelocityLUT[128];
  int mAfterTouchLUT[128];

} WDL_FIXALIGN;

inline bool operator==(const MidiSynth::KeyPressInfo& lhs, const MidiSynth::KeyPressInfo& rhs) { return lhs.mKey == rhs.mKey; }

