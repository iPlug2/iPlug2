
#ifndef __IPLUGPOLYSYNTHDSP__
#define __IPLUGPOLYSYNTHDSP__

const double ENV_VALUE_LOW = 0.000001; // -120dB
const double ENV_VALUE_HIGH = 0.999;
const double MIN_ENV_TIME_MS = 0.5;
const double MAX_ENV_TIME_MS = 60000.;

inline double fastClip(double x, const double low, const double high)
{
  double x1 = fabs(x-low);
  double x2 = fabs(x-high);
  x = x1+low+high-x2;
  
  return x * 0.5;
}

inline double wrap(double x, const double low = 0., const double high = 1.)
{
  while (x >= high) x -= high;
  while (x < low)  x += high - low;
  
  return x;
}

inline double lerp(const double phase, const double* buffer, const unsigned long int mask)
{
  const int intPart = (int) phase;
  const double fracPart = phase-intPart;
  
  const double a = buffer[intPart & mask];
  const double b = buffer[(intPart+1) & mask];
  
  return a + (b - a) * fracPart;
}

inline double calcIncrFromTimeLinear(const double timeMS, const double sr)
{
  if (timeMS <= 0.) return 0.;
  else return (1./sr) / (timeMS/1000.);
}

struct WTOscState
{
	double mPhase;			// float phase (goes between 0. and 1.)
	double mPhaseIncr;		// freq * mPhaseStep
	
	WTOscState()
	{
		mPhase = 0.;
    mPhaseIncr = 0.;
	}
	
} WDL_FIXALIGN;

class WTOsc 
{	
protected:
	const double* mLUT;           // pointer to waveform lookup table, const because the oscilator doesn't change the table data
	unsigned long int mLUTSize;		// wavetable size
	unsigned long int mLUTSizeM;  // wavetable Mask (size -1)
	double mLUTSizeF;             // float version
	double mPhaseStep;            // (1./sr);
	double mSampleRate;
	double mFreqCPS;
public: 	
			
	WTOsc(const double* LUT, const unsigned long int LUTSize) 
	{
		setLUT(LUT, LUTSize);
		setSampleRate(44100.);
	} 
	
	~WTOsc() {}
	
	void setLUT(const double* LUT, const unsigned long int LUTSize)
	{
		mLUTSize = LUTSize;
		mLUTSizeM = LUTSize-1;
		mLUTSizeF = (double) LUTSize;
		mLUT = LUT;
	}
	
	void setSampleRate(const double sr)
	{
		if (mSampleRate != sr)
		{
			mSampleRate = sr;
			mPhaseStep = 1./sr;
		}
	}
	
	inline double process(WTOscState* pState)
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
  kAttack, 
  kDecay, 
  kSustain, 
  kRelease,
};

struct ADSREnvLState
{
  double mEnvValue;          // current value of the envelope
  int	mStage;                // idle, attack, decay, sustain, release
  double mLevel;             // envelope depth
  double mPrev;
  double mReleaseLevel;
  
  ADSREnvLState()
  {
    mEnvValue = 0.;
    mStage = kIdle;
    mLevel = 0.;
    mReleaseLevel = 0.;
    mPrev = 0.;
  }
  
} WDL_FIXALIGN;

class ADSREnvL
{
protected:
  double mAttackIncr, mDecayIncr, mReleaseIncr;
  double mSustainLevel, mReleaseLevel;
  double mPrev;
  double mSampleRate;
  
public:
  ADSREnvL()
  {
    setStageTime(kAttack, 1.);
    setStageTime(kDecay, 100.);
    setStageTime(kRelease, 20.);
    
    mSustainLevel = 1.;
    mReleaseLevel = 0.;
    mPrev = 0.;
    mSampleRate = 44100.;
    mLevel = 1.;
  }
  
  void setStageTime(const UInt16 stage, const double timeMS)
  {
    const double incr = calcIncrFromTimeLinear(fastClip(timeMS, MIN_ENV_TIME_MS, MAX_ENV_TIME_MS), mSampleRate);
    
    switch(stage)
    {					
      case kAttack:
        mAttackIncr = incr;
        break;
      case kDecay:
        mDecayIncr = incr;
        break;
      case kRelease:
        mReleaseIncr = incr;
        break;
      default:
        //error
        break;
    }
  }
  
  void setSustainLevel(const double sustainLevel)
  {
    mSustainLevel = sustainLevel;
  }
  
  inline double process(ADSREnvLState* pS)
  {
    double result = 0.;
    
    switch(pS->mStage)
    {
      case kIdle:
        result = pS->mEnvValue;
        break;
      case kAttack:
        pS->mEnvValue += mAttackIncr;		
        if (pS->mEnvValue > ENV_VALUE_HIGH || mAttackIncr == 0.)
        {
          pS->mStage = kDecay;
          pS->mEnvValue = 1.0;
        }
        result = pS->mEnvValue;
        break;
      case kDecay:
        pS->mEnvValue -= mDecayIncr;
        result = (pS->mEnvValue * (1.-mSustainLevel)) + mSustainLevel;
        if (pS->mEnvValue < ENV_VALUE_LOW)
        {
          pS->mStage = kSustain;
          pS->mEnvValue = 1.;
          result = mSustainLevel;
        }
        break;
      case kSustain:
        result = mSustainLevel;
        break;
      case kRelease:
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

class CParamSmooth
{
public:
  CParamSmooth() { a = 0.99; b = 1. - a; z = 0.; };
  ~CParamSmooth() {};
  inline double Process(double in) { z = (in * b) + (z * a); return z; }
private:
  double a, b, z;
};

struct VoiceState
{
  WTOscState mOsc_ctx;
  ADSREnvLState mEnv_ctx;
  bool mLastBusy;
  int mKey;
  
  VoiceState()
  {
    mKey = -1;
    mLastBusy = false;
  }
  
  bool GetBusy()
  {
    if (mEnv_ctx.mStage == EnvBase::kIdle) 
      return false;
    else 
      return true;
  }
};


#endif //__IPLUGPOLYSYNTHDSP__