#ifndef __IPLUGMOUSETESTDSP_H__
#define __IPLUGMOUSETESTDSP_H__

inline double midi2CPS(double pitch)
{
  return 440. * pow(2., (pitch - 69.) / 12.);
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

#endif // __IPLUGMOUSETESTDSP_H__