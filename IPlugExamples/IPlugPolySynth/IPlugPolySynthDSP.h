
#ifndef __IPLUGPOLYSYNTHDSP__
#define __IPLUGPOLYSYNTHDSP__

const double ENV_VALUE_LOW = 0.000001; // -120dB
const double ENV_VALUE_HIGH = 0.999;
const double MIN_ENV_TIME_MS = 0.5;
const double MAX_ENV_TIME_MS = 60000.;

inline double midi2CPS(double pitch)
{
  return 440. * pow(2., (pitch - 69.) / 12.);
}

inline double fastClip(double x, double low, double high)
{
  double x1 = fabs(x-low);
  double x2 = fabs(x-high);
  x = x1+low+high-x2;

  return x * 0.5;
}

inline double wrap(double x, double low = 0., double high = 1.)
{
  while (x >= high) x -= high;
  while (x < low)  x += high - low;

  return x;
}

inline double lerp(double phase, const double* buffer, unsigned long int mask)
{
  const int intPart = (int) phase;
  const double fracPart = phase-intPart;

  const double a = buffer[intPart & mask];
  const double b = buffer[(intPart+1) & mask];

  return a + (b - a) * fracPart;
}

inline double calcIncrFromTimeLinear(double timeMS, double sr)
{
  if (timeMS <= 0.) return 0.;
  else return (1./sr) / (timeMS/1000.);
}

struct CWTOscState
{
  double mPhase;        // float phase (goes between 0. and 1.)
  double mPhaseIncr;    // freq * mPhaseStep

  CWTOscState()
  {
    mPhase = 0.;
    mPhaseIncr = 0.;
  }

} WDL_FIXALIGN;

class CWTOsc
{
protected:
  const double* mLUT;           // pointer to waveform lookup table, const because the oscilator doesn't change the table data
  unsigned long int mLUTSize;   // wavetable size
  unsigned long int mLUTSizeM;  // wavetable Mask (size -1)
  double mLUTSizeF;             // float version

public:

  CWTOsc(const double* LUT, unsigned long int LUTSize)
  {
    setLUT(LUT, LUTSize);
  }

  ~CWTOsc() {}

  void setLUT(const double* LUT, unsigned long int LUTSize)
  {
    mLUTSize = LUTSize;
    mLUTSizeM = LUTSize-1;
    mLUTSizeF = (double) LUTSize;
    mLUT = LUT;
  }

  inline double process(CWTOscState* pState)
  {
    pState->mPhase = wrap(pState->mPhase, 0., 1.);
    const double output = lerp(pState->mPhase * mLUTSizeF, mLUT, mLUTSizeM);
    pState->mPhase += pState->mPhaseIncr;

    return output;
  }

} WDL_FIXALIGN;

enum EADSREnvStage
{
  kIdle = 0,
  kStageAttack,
  kStageDecay,
  kStageSustain,
  kStageRelease,
};

struct CADSREnvLState
{
  double mEnvValue;          // current value of the envelope
  int mStage;                // idle, attack, decay, sustain, release
  double mLevel;             // envelope depth
  double mPrev;
  double mReleaseLevel;

  CADSREnvLState()
  {
    mEnvValue = 0.;
    mStage = kIdle;
    mLevel = 0.;
    mReleaseLevel = 0.;
    mPrev = 0.;
  }

} WDL_FIXALIGN;

class CADSREnvL
{
protected:
  double mAttackIncr, mDecayIncr, mReleaseIncr;
  double mSustainLevel, mReleaseLevel;
  double mPrev;
  double mSampleRate;

public:
  CADSREnvL()
  {
    mSustainLevel = 1.;
    mReleaseLevel = 0.;
    mPrev = 0.;
    mSampleRate = 44100.;

    setStageTime(kStageAttack, 1.);
    setStageTime(kStageDecay, 100.);
    setStageTime(kStageRelease, 20.);
  }

  void setStageTime(int stage, double timeMS)
  {
    const double incr = calcIncrFromTimeLinear(fastClip(timeMS, MIN_ENV_TIME_MS, MAX_ENV_TIME_MS), mSampleRate);

    switch(stage)
    {
      case kStageAttack:
        mAttackIncr = incr;
        break;
      case kStageDecay:
        mDecayIncr = incr;
        break;
      case kStageRelease:
        mReleaseIncr = incr;
        break;
      default:
        //error
        break;
    }
  }

  void setSustainLevel(double sustainLevel)
  {
    mSustainLevel = sustainLevel;
  }

  virtual void setSampleRate(double sr)
  {
    mSampleRate = sr;
  }

  inline double process(CADSREnvLState* pS)
  {
    double result = 0.;

    switch(pS->mStage)
    {
      case kIdle:
        result = pS->mEnvValue;
        break;
      case kStageAttack:
        pS->mEnvValue += mAttackIncr;
        if (pS->mEnvValue > ENV_VALUE_HIGH || mAttackIncr == 0.)
        {
          pS->mStage = kStageDecay;
          pS->mEnvValue = 1.0;
        }
        result = pS->mEnvValue;
        break;
      case kStageDecay:
        pS->mEnvValue -= mDecayIncr;
        result = (pS->mEnvValue * (1.-mSustainLevel)) + mSustainLevel;
        if (pS->mEnvValue < ENV_VALUE_LOW)
        {
          pS->mStage = kStageSustain;
          pS->mEnvValue = 1.;
          result = mSustainLevel;
        }
        break;
      case kStageSustain:
        result = mSustainLevel;
        break;
      case kStageRelease:
        pS->mEnvValue -= mReleaseIncr;
        if(pS->mEnvValue < ENV_VALUE_LOW || mReleaseIncr == 0.)
        {
          pS->mStage = kIdle;
          pS->mEnvValue = 0.0;
        }
        result = pS->mEnvValue * pS->mReleaseLevel;
        break;
      default:
        result = pS->mEnvValue;
        break;
    }

    pS->mPrev = result;

    return result * pS->mLevel;
  }

} WDL_FIXALIGN ;

// http://www.musicdsp.org/archive.php?classid=3#257

//class CParamSmooth
//{
//public:
//  CParamSmooth() { a = 0.99; b = 1. - a; z = 0.; };
//  ~CParamSmooth() {};
//  inline double Process(double in) { z = (in * b) + (z * a); return z; }
//private:
//  double a, b, z;
//};

struct CVoiceState
{
  CWTOscState mOsc_ctx;
  CADSREnvLState mEnv_ctx;
  //bool mLastBusy;
  int mKey;

  CVoiceState()
  {
    mKey = -1;
    //mLastBusy = false;
  }

  bool GetBusy()
  {
    if (mEnv_ctx.mStage == kIdle)
      return false;
    else
      return true;
  }
};


#endif //__IPLUGPOLYSYNTHDSP__