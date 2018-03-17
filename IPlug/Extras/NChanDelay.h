#pragma once

// A static delayline used to delay bypassed signals to match mLatency in AAX/VST3/AU
template<typename sampType>
class NChanDelayLine
{
private:
  WDL_TypedBuf<sampType> mBuffer;
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
    memset(mBuffer.Get(), 0, mNumInChans * mDTSamples * sizeof(sampType));
  }
  
  void ProcessBlock(sampType** inputs, sampType** outputs, int nFrames)
  {
    sampType* buffer = mBuffer.Get();
    
    for (int s = 0 ; s < nFrames; ++s)
    {
      signed long readAddress = mWriteAddress - mDTSamples;
      readAddress %= mDTSamples;
      
      for (int chan = 0; chan < mNumInChans; chan++)
      {
        if (chan < mNumOutChans)
        {
          unsigned long offset = chan * mDTSamples;
          outputs[chan][s] = buffer[offset + readAddress];
          buffer[offset + mWriteAddress] = inputs[chan][s];
        }
      }
      
      mWriteAddress++;
      mWriteAddress %= mDTSamples;
    }
  }
  
} WDL_FIXALIGN;

