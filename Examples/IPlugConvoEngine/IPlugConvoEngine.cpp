#include "IPlugConvoEngine.h"
#include "IPlug_include_in_plug_src.h"

IPlugConvoEngine::IPlugConvoEngine(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kParamDry)->InitDouble("Dry", 0., 0., 1., 0.001);
  GetParam(kParamWet)->InitDouble("Wet", 1., 0., 1., 0.001);
}

#if IPLUG_DSP
void IPlugConvoEngine::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  sample* inputL = inputs[0];
  sample* outputL = outputs[0];
  
  mEngine.Add(inputs, nFrames, 1);

  int nAvailableSamples = std::min(mEngine.Avail(nFrames), nFrames);

  const sample dryGain = GetParam(kParamDry)->Value();
  const sample wetGain = GetParam(kParamWet)->Value();

  // If not enough samples are available yet, then only output the dry signal
  for (auto i = 0; i < nFrames - nAvailableSamples; ++i)
  {
    *outputL++ = dryGain * *inputL++;
  }

  // Output samples from the convolution engine
  if (nAvailableSamples > 0)
  {
    // Apply the dry/wet mix
    WDL_FFT_REAL* pWetSignal = mEngine.Get()[0];
    for (auto i = 0; i < nAvailableSamples; ++i)
    {
      *outputL++ = dryGain * *inputL++ + wetGain * *pWetSignal++;
    }

    // Remove the sample block from the convolution engine's buffer
    mEngine.Advance(nAvailableSamples);
  }
}

void IPlugConvoEngine::OnReset()
{
  if (GetSampleRate() != mSampleRate)
  {
    mSampleRate = GetSampleRate();

    static constexpr int irLength = sizeof(mIR) / sizeof(mIR[0]);
    static constexpr double irSampleRate = 44100.;
    mImpulse.SetNumChannels(1);

#if defined USE_WDL_RESAMPLER
    mResampler.SetMode(false, 0, true); // Sinc, default size
    mResampler.SetFeedMode(true); // Input driven
#elif defined USE_R8BRAIN
    mResampler = std::make_unique<CDSPResampler16IR>(irSampleRate, mSampleRate, mBlockLength);
#endif

    // Resample the impulse response.
    auto len = mImpulse.SetLength(ResampleLength(irLength, irSampleRate, mSampleRate));
    if (len)
    {
      Resample(mIR, irLength, irSampleRate, mImpulse.impulses[0].Get(), len, mSampleRate);
    }
    
    // Tie the impulse response to the convolution engine.
    mEngine.SetImpulse(&mImpulse);
    
    SetLatency(mEngine.GetLatency());
  }
}

template <class I, class O>
void IPlugConvoEngine::Resample(const I* pSrc, int srcLength, double srcRate, O* pDest, int dstLength, double dstRate)
{
  if (dstLength == srcLength)
  {
    // Copy
    for (int i = 0; i < dstLength; ++i) *pDest++ = (O)*pSrc++;
    return;
  }

  // Resample using WDL's resampler.
  #if defined USE_WDL_RESAMPLER
  mResampler.SetRates(srcRate, dstRate);
  double scale = srcRate / dstRate;
  while (dstLength > 0)
  {
    WDL_ResampleSample* p;
    int n = mResampler.ResamplePrepare(mBlockLength, 1, &p), m = n;
    if (n > srcLength) n = srcLength;
    for (int i = 0; i < n; ++i) *p++ = (WDL_ResampleSample)*pSrc++;
    if (n < m) memset(p, 0, (m - n) * sizeof(WDL_ResampleSample));
    srcLength -= n;

    WDL_ResampleSample buf[mBlockLength];
    n = mResampler.ResampleOut(buf, m, m, 1);
    if (n > dstLength) n = dstLength;
    p = buf;
    for (int i = 0; i < n; ++i) *pDest++ = (O)(scale * *p++);
    dstLength -= n;
  }
  mResampler.Reset();

  // Resample using r8brain-free
  #elif defined USE_R8BRAIN
  double scale = srcRate / dstRate;
  while (dstLength > 0)
  {
    double buf[mBlockLength], *p = buf;
    int n = mBlockLength;
    if (n > srcLength) n = srcLength;
    for (int i = 0; i < n; ++i) *p++ = (double)*pSrc++;
    if (n < mBlockLength) memset(p, 0, (mBlockLength - n) * sizeof(double));
    srcLength -= n;

    n = mResampler->process(buf, mBlockLength, p);
    if (n > dstLength) n = dstLength;
    for (int i = 0; i < n; ++i) *pDest++ = (O)(scale * *p++);
    dstLength -= n;
  }
  mResampler->clear();

  // Resample using linear interpolation.
  #else
  double pos = 0.;
  double delta = srcRate / dstRate;
  for (int i = 0; i < dstLength; ++i)
  {
    int idx = int(pos);
    if (idx < srcLength)
    {
      double frac = pos - floor(pos);
      double interp = (1. - frac) * pSrc[idx];
      if (++idx < srcLength) interp += frac * pSrc[idx];
      pos += delta;
      *pDest++ = (O)(delta * interp);
    }
    else
    {
      *pDest++ = 0;
    }
  }
  #endif
}

const float IPlugConvoEngine::mIR[] =
{
  #include "ir.h"
};
#endif

