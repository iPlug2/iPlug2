#include "IPlugResampler.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "IAutoGUI.h"

#include "resource.h"

#include "../../WDL/resample.cpp"

const int kNumPrograms = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

IPlugResampler::IPlugResampler(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo), mGain(1.)
{
  TRACE;
  
  //Fill the mAudioToResample array with 100 cycles of a 441hz sine wave
  double recip = 1. / 44100.;
  double phase = 0.;
  
  for (int i = 0; i< 44100; i++)
  {
    mAudioToResample[i] = sin( phase * 6.283185307179586);
    phase += recip * 441.;
    if (phase > 1.) phase -= 1.;
  }
  
  mSampleIndx = 0;
  // WDL_Resampler::SetMode arguments are bool interp, int filtercnt, bool sinc, int sinc_size, int sinc_interpsize
  // sinc mode will get better results, but will use more cpu
  // todo: explain arguments
  mResampler.SetMode(true, 1, false, 0, 0);
  mResampler.SetFilterParms();
  // set it output driven
  mResampler.SetFeedMode(false);
  // set input and output samplerates
  mResampler.SetRates(44100., GetSampleRate());

  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kGain)->InitDouble("Gain", 0.0, -70.0, 12.0, 0.1, "dB");

  IGraphics* pGraphics = MakeGraphics(this, GUI_WIDTH, GUI_HEIGHT);
  IText textProps(12, &COLOR_BLACK, "Verdana", IText::kStyleNormal, IText::kAlignNear, 0, IText::kQualityNonAntiAliased);
	GenerateKnobGUI(pGraphics, this, &textProps, &COLOR_WHITE, &COLOR_BLACK, 60, 70);
  AttachGraphics(pGraphics);
  
  //MakePreset("preset 1", ... );
  MakeDefaultPreset((char *) "-", kNumPrograms);
}

IPlugResampler::~IPlugResampler() {}

void IPlugResampler::Reset()
{
  TRACE;
  IMutexLock lock(this);
  
  mSampleIndx = 0;
  mResampler.Reset();
  // set input and output samplerates
  mResampler.SetRates(44100., GetSampleRate());
}

void IPlugResampler::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  // Mutex is already locked for us.

  double* out1 = outputs[0];
  double* out2 = outputs[1];

  WDL_ResampleSample *resampledAudio=NULL;
  int numSamples = mResampler.ResamplePrepare(nFrames,1,&resampledAudio);
  
  for (int s = 0; s < numSamples; s++) 
  {
    if (mSampleIndx >= 44100) mSampleIndx = 0;
    resampledAudio[s] = mAudioToResample[mSampleIndx++] * mGain;
  }
    
  if (mResampler.ResampleOut(out1, numSamples, nFrames, 1) != nFrames)
  {
    //failed somehow
    memset(out1, 0 , nFrames * sizeof(double));
  }

  memcpy(out2, out1, nFrames * sizeof(double));
}

void IPlugResampler::OnParamChange(int paramIdx)
{
  IMutexLock lock(this);

  switch (paramIdx)
  {
    case kGain:
      mGain = GetParam(kGain)->DBToAmp();
      break;
      
    default:
      break;
  }
}


