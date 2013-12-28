#ifndef _NCHANDELAY_
#define _NCHANDELAY_

// A static delayline used to delay bypassed signals to match mLatency in RTAS/AAX/VST3/AU
class NChanDelayLine
{
private:
  WDL_TypedBuf<double> mBuffer;
  unsigned long mWriteAddress;
  unsigned int mNumInChans, mNumOutChans;
  unsigned long mDTSamples;
  
public:
  NChanDelayLine(int maxInputChans = 2, int maxOutputChans = 2)
  : mNumInChans(maxInputChans)
  , mNumOutChans(maxOutputChans) 
  , mWriteAddress(0)
  , mDTSamples(0) {}
  
  ~NChanDelayLine() {}
  
  void SetDelayTime(int delayTimeSamples)
  {
    mDTSamples = delayTimeSamples;
    mBuffer.Resize(mNumInChans * delayTimeSamples);
    mWriteAddress = 0;
    ClearBuffer();
  }
  
  void ClearBuffer()
  {
    memset(mBuffer.Get(), 0, mNumInChans * mDTSamples * sizeof(double));
  }
  
  void ProcessBlock(double** inputs, double** outputs, int nFrames)
  {
    double* buffer = mBuffer.Get();
    
    for (int s = 0 ; s < nFrames; ++s)
    {
      signed long readAddress = mWriteAddress - mDTSamples;
      readAddress %= mDTSamples;
      
      for (int chan = 0; chan < mNumInChans; chan++) 
      {
        if (chan < mNumOutChans)
        {
          int offset = chan * mDTSamples;
          outputs[chan][s] = buffer[offset + readAddress];
          buffer[offset + mWriteAddress] = inputs[chan][s];
        }
      }
      
      mWriteAddress++;
      mWriteAddress %= mDTSamples;
    }
  }
  
} WDL_FIXALIGN;

#endif //_NCHANDELAY_