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

public:
  ADSREnvelope(const char* name = "", std::function<void()> resetFunc = nullptr, bool sustainEnabled = true)
  : mName(name)
  , mResetFunc(resetFunc)
  , mSustainEnabled(sustainEnabled)
  {
    SetSampleRate(44100.);
  }

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

  bool GetBusy() const
  {
    return mStage != kIdle;
  }

  bool GetReleased() const
  {
    return mStage != kIdle;
  }

  T GetPrevOutput() const
  {
    return mPrevOutput;
  }
  
  inline void Start(T level, T timeScalar = 1.)
  {
    mStage = kAttack;
    mEnvValue = 0.;
    mLevel = level;
    mScalar = 1./timeScalar;
    mReleased = false;
  }

  inline void Release()
  {
    mStage = kRelease;
    mReleaseLevel = mPrevResult;
    mEnvValue = 1.;
    mReleased = true;
  }

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

  void SetSampleRate(T sr)
  {
    mSampleRate = sr;
    mEarlyReleaseIncr = CalcIncrFromTimeLinear(EARLY_RELEASE_TIME, sr);
    mRetriggerReleaseIncr = CalcIncrFromTimeLinear(RETRIGGER_RELEASE_TIME, sr);
  }

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
