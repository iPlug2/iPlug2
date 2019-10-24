/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
 */

#include "IPlugPlatform.h"

BEGIN_IPLUG_NAMESPACE

template <typename T>
class ADSREnvelope
{
public:
  enum EStage
  {
    kReleasedToEndEarly = -3,
    kReleasedToRetrigger = -2,
    kIdle = -1,
    kAttack,
    kDecay,
    kSustain,
    kRelease
  };

  static constexpr T EARLY_RELEASE_TIME = 20.; // ms
  static constexpr T RETRIGGER_RELEASE_TIME = 3.; // ms
  static constexpr T MIN_ENV_TIME_MS = 0.022675736961451; // 1 sample @44100
  static constexpr T MAX_ENV_TIME_MS = 60000.;
  static constexpr T ENV_VALUE_LOW = 0.000001; // -120dB
  static constexpr T ENV_VALUE_HIGH = 0.999;
  
private:
#if DEBUG_ENV
  bool mEnableDBGMSG = false;
#endif
  
  const char* mName;
  T mEarlyReleaseIncr = 0.;
  T mRetriggerReleaseIncr = 0.;
  T mAttackIncr = 0.;
  T mDecayIncr = 0.;
  T mReleaseIncr = 0.;
  T mSampleRate;
  T mEnvValue = 0.;          // current normalized value of the envelope
  int mStage = kIdle;        // the current stage
  T mLevel = 0.;             // envelope depth from velocity
  T mReleaseLevel = 0.;      // the level when the env is released
  T mNewStartLevel = 0.;     // envelope depth from velocity when retriggering
  T mPrevResult = 0.;        // last value BEFORE velocity scaling
  T mPrevOutput = 0.;        // last value AFTER velocity scaling
  T mScalar = 1.;            // for key-follow scaling
  bool mReleased = true;
  bool mSustainEnabled = true; // when false env is AD only
  
  std::function<void()> mResetFunc = nullptr; // reset func
  std::function<void()> mEndReleaseFunc = nullptr; // end release func

public:
  /** Constructs an ADSREnvelope object 
  * @param name CString to identify the envelope in debug mode, when DEBUG_ENV=1 is set as a global preprocessor macro
  * @param resetFunc A function to call when the envelope gets retriggered, called when the fade out ramp is at zero, useful for example to reset an oscillator's phase
  * @param sustainEnabled if true the envelope is an ADSR envelope. If false, it's is an AD envelope (suitable for drums). */
  ADSREnvelope(const char* name = "", std::function<void()> resetFunc = nullptr, bool sustainEnabled = true)
  : mName(name)
  , mResetFunc(resetFunc)
  , mSustainEnabled(sustainEnabled)
  {
    SetSampleRate(44100.);
  }

  /** Sets the time for a particular envelope stage 
  * @param stage The stage to set the time for /see EStage
  * @param timeMS The time in milliseconds for that stage */
  void SetStageTime(int stage, T timeMS)
  {
    switch(stage)
    {
      case kAttack:
        mAttackIncr = CalcIncrFromTimeLinear(Clip(timeMS, MIN_ENV_TIME_MS, MAX_ENV_TIME_MS), mSampleRate);
        break;
      case kDecay:
        mDecayIncr = CalcIncrFromTimeExp(Clip(timeMS, MIN_ENV_TIME_MS, MAX_ENV_TIME_MS), mSampleRate);
        break;
      case kRelease:
        mReleaseIncr = CalcIncrFromTimeExp(Clip(timeMS, MIN_ENV_TIME_MS, MAX_ENV_TIME_MS), mSampleRate);
        break;
      default:
        //error
        break;
    }
  }

  /** @return /c true if the envelope is not idle */
  bool GetBusy() const
  {
    return mStage != kIdle;
  }

  /** @return the previously output value */
  T GetPrevOutput() const
  {
    return mPrevOutput;
  }
  
  /** Trigger/Start the envelope 
   * @param level The overall depth of the envelope (usually linked to MIDI velocity)  
   * @param timeScalar Factor to scale the envelope's rates. Use this, for example to adjust the envelope stage rates based on the key pressed */
  inline void Start(T level, T timeScalar = 1.)
  {
    mStage = kAttack;
    mEnvValue = 0.;
    mLevel = level;
    mScalar = 1./timeScalar;
    mReleased = false;
  }

  /** Release the envelope */
  inline void Release()
  {
    mStage = kRelease;
    mReleaseLevel = mPrevResult;
    mEnvValue = 1.;
    mReleased = true;
  }
  
  /** Retrigger the envelope. This method will cause the envelope to move to a "releasedToRetrigger" stage, which is a fast ramp to zero in RETRIGGER_RELEASE_TIME, used when voices are stolen to avoid clicks.
  * @param newStartLevel the overall level when the envelope restarts (usually linked to MIDI velocity)
  * @param timeScalar Factor to scale the envelope's rates. Use this, for example to adjust the envelope stage rates based on the key pressed */
  inline void Retrigger(T newStartLevel, T timeScalar = 1.)
  {
    mEnvValue = 1.;
    mNewStartLevel = newStartLevel;
    mScalar = 1./timeScalar;
    mReleaseLevel = mPrevResult;
    mStage = kReleasedToRetrigger;
    mReleased = false;

    #if DEBUG_ENV
    if (mEnableDBGMSG) DBGMSG("retrigger\n");
    #endif
  }

  /** Kill the envelope 
  * @param hard If true, the envelope will get reset automatically, probably causing an audible glitch. If false, it's a "soft kill", which will fade out in EARLY_RELEASE_TIME */
  inline void Kill(bool hard)
  {
    if(hard)
    {
      if (mStage != kIdle)
      {
        mReleaseLevel = 0.;
        mStage = kIdle;
        mEnvValue = 0.;
      }

      #if DEBUG_ENV
      if (mEnableDBGMSG) DBGMSG("hard kill\n");
      #endif
    }
    else
    {
      if (mStage != kIdle)
      {
        mReleaseLevel = mPrevResult;
        mStage = kReleasedToEndEarly;
        mEnvValue = 1.;
      }

      #if DEBUG_ENV
      if (mEnableDBGMSG) DBGMSG("soft kill\n");
      #endif
    }
  }

  /** Set the sample rate for processing, with updates the early release time and retrigger release time coefficents.
  * NOTE: you also need to think about updating the Attack, Decay and Release times when the sample rate changes 
  * @param sr SampleRate in samples per second */
  void SetSampleRate(T sr)
  {
    mSampleRate = sr;
    mEarlyReleaseIncr = CalcIncrFromTimeLinear(EARLY_RELEASE_TIME, sr);
    mRetriggerReleaseIncr = CalcIncrFromTimeLinear(RETRIGGER_RELEASE_TIME, sr);
  }

  /** Sets a function to call when the envelope gets retriggered, called when the fade out ramp is at zero, useful for example to reset an oscillator's phase
   * WARNING: don't call this on the audio thread, std::function can malloc
   * @param func the reset function, or nullptr for none */
  void SetResetFunc(std::function<void()> func) { mResetFunc = func; }
  
  /** Sets a function to call when the envelope gets released, called when the ramp is at zero
   * WARNING: don't call this on the audio thread, std::function can malloc
   * @param func the release function, or nullptr for none */
  void SetEndReleaseFunc(std::function<void()> func) { mEndReleaseFunc = func; }
  
  /** Process the envelope, returning the value according to the current envelope stage
  * @param sustainLevel Since the sustain level could be changed during processing, it is supplied as an argument, so that it can be smoothed extenally if nessecary, to avoid discontinuities */
  inline T Process(T sustainLevel = 0.)
  {
    T result = 0.;

    switch(mStage)
    {
      case kIdle:
        result = mEnvValue;
        break;
      case kAttack:
        mEnvValue += (mAttackIncr * mScalar);
        if (mEnvValue > ENV_VALUE_HIGH || mAttackIncr == 0.)
        {
          mStage = kDecay;
          mEnvValue = 1.;
        }
        result = mEnvValue;
        break;
      case kDecay:
        mEnvValue -= ((mDecayIncr*mEnvValue) * mScalar);
        result = (mEnvValue * (1.-sustainLevel)) + sustainLevel;
        if (mEnvValue < ENV_VALUE_LOW)
        {
          if(mSustainEnabled)
          {
            mStage = kSustain;
            mEnvValue = 1.;
            result = sustainLevel;
          }
          else
            Release();
        }
        break;
      case kSustain:
        result = sustainLevel;
        break;
      case kRelease:
        mEnvValue -= ((mReleaseIncr*mEnvValue) * mScalar);
        if(mEnvValue < ENV_VALUE_LOW || mReleaseIncr == 0.)
        {
          mStage = kIdle;
          mEnvValue = 0.;
          
          if(mEndReleaseFunc)
            mEndReleaseFunc();
        }
        result = mEnvValue * mReleaseLevel;
        break;
      case kReleasedToRetrigger:
        mEnvValue -= mRetriggerReleaseIncr;
        if(mEnvValue < ENV_VALUE_LOW)
        {
          mStage = kAttack;
          mLevel = mNewStartLevel;
          mEnvValue = 0.;
          mPrevResult = 0.;
          mReleaseLevel = 0.;
          
          if(mResetFunc)
            mResetFunc();
        }
        result = mEnvValue * mReleaseLevel;
        break;
      case kReleasedToEndEarly:
        mEnvValue -= mEarlyReleaseIncr;
        if(mEnvValue < ENV_VALUE_LOW)
        {
          mStage = kIdle;
          mLevel = 0.;
          mEnvValue = 0.;
          mPrevResult = 0.;
          mReleaseLevel = 0.;
          if(mEndReleaseFunc)
            mEndReleaseFunc();
        }
        result = mEnvValue * mReleaseLevel;
        break;
      default:
        result = mEnvValue;
        break;
    }

    mPrevResult = result;
    mPrevOutput = (result * mLevel);
    return mPrevOutput;
  }

private:
  inline T CalcIncrFromTimeLinear(T timeMS, T sr) const
  {
    if (timeMS <= 0.) return 0.;
    else return (1./sr) / (timeMS/1000.);
  }
  
  inline T CalcIncrFromTimeExp(T timeMS, T sr) const
  {
    T r;
    
    if (timeMS <= 0.0) return 0.;
    else
    {
      r = -std::expm1(1000.0 * std::log(0.001) / (sr * timeMS));
      if (!(r < 1.0)) r = 1.0;
      
      return r;
    }
  }
};

END_IPLUG_NAMESPACE
