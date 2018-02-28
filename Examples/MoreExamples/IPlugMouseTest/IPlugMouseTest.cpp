#include "IPlugMouseTest.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "IPlugMouseTestControls.h"
#include "resource.h"

const int kNumPrograms = 1;

enum EParams
{
  kPitchA = 0,
  kPitchB,
  kNumParams
};

enum ELayout
{
  kWidth = GUI_WIDTH,
  kHeight = GUI_HEIGHT,
};

#define TABLE_SIZE 512

#ifndef M_PI
#define M_PI 3.14159265
#endif

IPlugMouseTest::IPlugMouseTest(IPlugInstanceInfo instanceInfo)
  :	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{
  TRACE;

  mTable = new double[TABLE_SIZE];
  
  for (int i = 0; i < TABLE_SIZE; i++)
  {
    mTable[i] = sin( i/double(TABLE_SIZE) * 2. * M_PI);
    //printf("mTable[%i] %f\n", i, mTable[i]);
  }
  
  mOsc = new CWTOsc(mTable, TABLE_SIZE);

  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kPitchA)->InitDouble("PitchA", 10., 0., 127., 0.01, "st");
  GetParam(kPitchB)->InitDouble("PitchB", 10., 0., 127., 0.01, "st");

  IGraphics* pGraphics = MakeGraphics(this, kWidth, kHeight, 30);
  pGraphics->AttachPanelBackground(&COLOR_RED);

  pGraphics->AttachControl(new IXYPad(this, IRECT(0, 0, kWidth, kHeight), 10, kPitchA, kPitchB));

  pGraphics->HandleMouseOver(true);
  AttachGraphics(pGraphics);

  //MakePreset("preset 1", ... );
  MakeDefaultPreset((char *) "-", kNumPrograms);
}

IPlugMouseTest::~IPlugMouseTest() 
{
  delete mOsc;
  delete [] mTable;
}

void IPlugMouseTest::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  // Mutex is already locked for us.
  double* out1 = outputs[0];
  double* out2 = outputs[1];

  for (int s = 0; s < nFrames; ++s, ++out1, ++out2)
  {
    *out1 = mOsc->process(&mOsc1_ctx) * 0.1;
    *out2 = mOsc->process(&mOsc2_ctx) * 0.1;
  }
}

void IPlugMouseTest::Reset()
{
  TRACE;
  IMutexLock lock(this);

  mSampleRate = GetSampleRate();
  mOsc1_ctx.mPhaseIncr = (1./mSampleRate) * midi2CPS(GetParam(kPitchA)->Value());
  mOsc2_ctx.mPhaseIncr = (1./mSampleRate) * midi2CPS(GetParam(kPitchB)->Value());

}

void IPlugMouseTest::OnParamChange(int paramIdx)
{
  IMutexLock lock(this);

  switch (paramIdx)
  {
    case kPitchA:
      mOsc1_ctx.mPhaseIncr = (1./mSampleRate) * midi2CPS(GetParam(kPitchA)->Value());
      break;
    case kPitchB:
      mOsc2_ctx.mPhaseIncr = (1./mSampleRate) * midi2CPS(GetParam(kPitchB)->Value());
      break;
    default:
      break;
  }
}
