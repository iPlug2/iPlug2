#include "IPlugGUIResize.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "resource.h"

const int kNumPrograms = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

enum ELayout
{
  kWidth = GUI_WIDTH,
  kHeight = GUI_HEIGHT,

  kGainX = 100,
  kGainY = 100,
  kKnobFrames = 60
};

IPlugGUIResize::IPlugGUIResize(IPlugInstanceInfo instanceInfo)
:	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo), mGain(1.)
{
  TRACE;

  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kGain)->InitDouble("Gain", 50., 0., 100.0, 0.01, "%");
  GetParam(kGain)->SetShape(2.);


  IGraphics* pGraphics = MakeGraphics(this, kWidth, kHeight);
  CreateControls(pGraphics, 0);
  AttachGraphics(pGraphics);

  //MakePreset("preset 1", ... );
  MakeDefaultPreset((char *) "-", kNumPrograms);
}

IPlugGUIResize::~IPlugGUIResize() {}

void IPlugGUIResize::CreateControls(IGraphics* pGraphics, int size)
{
  switch(size)
  {
    case 0:
      pGraphics->AttachPanelBackground(&COLOR_RED);
      pGraphics->AttachControl(new IGUIResizeButton(this, IRECT(kGainX, kGainY, kGainX + 48, kGainY + 48 + 20)));
      break;
    case 1:
      pGraphics->AttachPanelBackground(&COLOR_RED);
      pGraphics->AttachControl(new IGUIResizeButton(this, IRECT(kGainX+100, kGainY, kGainX + 48, kGainY + 48 + 20)));
    default:
      break;
  }
}

void IPlugGUIResize::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  // Mutex is already locked for us.

  double* in1 = inputs[0];
  double* in2 = inputs[1];
  double* out1 = outputs[0];
  double* out2 = outputs[1];

  //double samplesPerBeat = GetSamplesPerBeat();
  //double samplePos = (double) GetSamplePos();
  //double tempo = GetTempo();

  for (int s = 0; s < nFrames; ++s, ++in1, ++in2, ++out1, ++out2)
  {
    *out1 = *in1 * mGain;
    *out2 = *in2 * mGain;
  }
}

void IPlugGUIResize::Reset()
{
  TRACE;
  IMutexLock lock(this);

  //double sr = GetSampleRate();
}

void IPlugGUIResize::OnParamChange(int paramIdx)
{
  IMutexLock lock(this);

  switch (paramIdx)
  {
    case kGain:
      mGain = GetParam(kGain)->Value() / 100.;;
      break;

    default:
      break;
  }
}

void IPlugGUIResize::OnWindowResize()
{
  CreateControls(GetGUI(), 0);
}