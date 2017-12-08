#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "config.h"

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

  kTextX = 10,
  kTextY = 10,
  kGainX = 100,
  kGainY = 100,
  kKnobFrames = 60
};

IPlugEffect::IPlugEffect(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{
  TRACE; 

  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kGain)->InitDouble("Gain", 50., 0., 100.0, 0.01, "%");
  GetParam(kGain)->SetShape(2.);

  IGraphics* pGraphics = MakeGraphics(*this, kWidth, kHeight, 30);
  pGraphics->AttachPanelBackground(COLOR_RED);

  IBitmap knob = pGraphics->LoadIBitmap(KNOB_FN, kKnobFrames, false, 2. /* this bitmap is 2* = hidpi */);
  
  pGraphics->AttachControl(new IKnobMultiControl(*this, kGainX, kGainY, kGain, knob));

  IText basic;
  char builddatestr[80];
  sprintf(builddatestr, "IPlugEffect %s %s, built on %s at %.5s ", GetArchString(), GetAPIString(), __DATE__, __TIME__);

  pGraphics->AttachControl(new ITextControl(*this, IRECT(kTextX, kTextY, 290, kTextY+10), basic, builddatestr));

  AttachGraphics(pGraphics);
  //pGraphics->ShowControlBounds(true);
  
  //MakePreset("preset 1", ... );
  MakeDefaultPreset("-", kNumPrograms);
}

IPlugEffect::~IPlugEffect() {}

void IPlugEffect::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  mParams_mutex.Enter();
  const double gain = GetParam(kGain)->Value() / 100.;
  mParams_mutex.Leave();
  
  double* in1 = inputs[0];
  double* in2 = inputs[1];
  double* out1 = outputs[0];
  double* out2 = outputs[1];

  for (int s = 0; s < nFrames; ++s, ++in1, ++in2, ++out1, ++out2)
  {
    *out1 = *in1 * gain;
    *out2 = *in2 * gain;
  }
}

void IPlugEffect::Reset()
{
  TRACE;
}

void IPlugEffect::OnParamChange(int paramIdx)
{
  switch (paramIdx)
  {
    case kGain:
    {
      break;
    }
    default:
      break;
  }
}
