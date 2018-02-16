#include "IPlugMultiChannel.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "resource.h"
#include "IAutoGUI.h"

const int kNumPrograms = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

IPlugMultiChannel::IPlugMultiChannel(IPlugInstanceInfo instanceInfo)
  :	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo), mGain(1.)
{
  TRACE;

  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kGain)->InitDouble("Gain", 50., 0., 100.0, 0.01, "%");
  GetParam(kGain)->SetShape(2.);
  
  IGraphics* pGraphics = MakeGraphics(this, GUI_WIDTH, GUI_HEIGHT);
  IText textProps(12, &COLOR_BLACK, "Verdana", IText::kStyleNormal, IText::kAlignNear, 0, IText::kQualityNonAntiAliased);
	GenerateKnobGUI(pGraphics, this, &textProps, &COLOR_WHITE, &COLOR_BLACK, 60, 70);
  AttachGraphics(pGraphics);
  
  MakeDefaultPreset((char *) "-", kNumPrograms);
}

IPlugMultiChannel::~IPlugMultiChannel() {}

void IPlugMultiChannel::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  // Mutex is already locked for us.

//  bool in1ic = IsInChannelConnected(0);
//  bool in2ic = IsInChannelConnected(1);
//  bool in3ic = IsInChannelConnected(2);
//  bool in4ic = IsInChannelConnected(3);
//  
//  DBGMSG("%i %i %i %i, ------------------------- \n", in1ic, in2ic, in3ic, in4ic);
  
  double* in1 = inputs[0];
  double* in2 = inputs[1];
  double* in3 = inputs[2];
  double* in4 = inputs[3];

  double* out1 = outputs[0];
  double* out2 = outputs[1];
  double* out3 = outputs[2];
  double* out4 = outputs[3];

  for (int s=0; s<nFrames; s++)
  {
    out1[s] = in1[s] * mGain;
    out2[s] = in2[s] * mGain;
    out3[s] = in3[s] * mGain;
    out4[s] = in4[s] * mGain;
  }
}

void IPlugMultiChannel::Reset()
{
  TRACE;
  IMutexLock lock(this);
}

void IPlugMultiChannel::OnParamChange(int paramIdx)
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
