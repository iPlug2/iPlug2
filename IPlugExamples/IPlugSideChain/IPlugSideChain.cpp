#include "IPlugSideChain.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "resource.h"

const int kNumPrograms = 1;

const double METER_ATTACK = 0.6, METER_DECAY = 0.05;

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

  kMeterL_X = 10,
  kMeterL_Y = 10,
  kMeterL_W = 8,
  kMeterL_H = 50,

  kMeterR_X = 20,
  kMeterR_Y = 10,
  kMeterR_W = 8,
  kMeterR_H = 50,

  kMeterLS_X = 30,
  kMeterLS_Y = 10,
  kMeterLS_W = 8,
  kMeterLS_H = 50,

  kMeterRS_X = 40,
  kMeterRS_Y = 10,
  kMeterRS_W = 8,
  kMeterRS_H = 50,

  kKnobFrames = 60
};

IPlugSideChain::IPlugSideChain(IPlugInstanceInfo instanceInfo)
  :	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo), mGain(1.)
  , mPrevL(0.0)
  , mPrevR(0.0)
  , mPrevLS(0.0)
  , mPrevRS(0.0)
{
  TRACE;

  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kGain)->InitDouble("Gain", 50., 0., 100.0, 0.01, "%");
  GetParam(kGain)->SetShape(2.);

  IGraphics* pGraphics = MakeGraphics(this, kWidth, kHeight);
  pGraphics->AttachPanelBackground(&COLOR_RED);
  IBitmap knob = pGraphics->LoadIBitmap(KNOB_ID, KNOB_FN, kKnobFrames);
  IText text = IText(14);
  pGraphics->AttachControl(new IKnobMultiControlText(this, IRECT(kGainX, kGainY, kGainX + 48, kGainY + 48 + 20), kGain, &knob, &text));

  mMeterIdx_L = pGraphics->AttachControl(new IPeakMeterVert(this, MakeIRect(kMeterL)));
  mMeterIdx_R = pGraphics->AttachControl(new IPeakMeterVert(this, MakeIRect(kMeterR)));
  mMeterIdx_LS = pGraphics->AttachControl(new IPeakMeterVert(this, MakeIRect(kMeterLS)));
  mMeterIdx_RS = pGraphics->AttachControl(new IPeakMeterVert(this, MakeIRect(kMeterRS)));

  if (GetAPI() == kAPIVST2) // for VST2 we name individual outputs
  {
    SetInputLabel(0, "main input L");
    SetInputLabel(1, "main input R");
    SetInputLabel(2, "sc input L");
    SetInputLabel(3, "sc input R");
    SetOutputLabel(0, "output L");
    SetOutputLabel(1, "output R");
  }
  else // for AU and VST3 we name buses
  {
    SetInputBusLabel(0, "main input");
    SetInputBusLabel(1, "sc input");
    SetOutputBusLabel(0, "output");
  }

  AttachGraphics(pGraphics);
  //MakePreset("preset 1", ... );
  MakeDefaultPreset((char *) "-", kNumPrograms);
}

IPlugSideChain::~IPlugSideChain() {}

void IPlugSideChain::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  // Mutex is already locked for us.

  bool in1ic = IsInChannelConnected(0);
  bool in2ic = IsInChannelConnected(1);
  bool in3ic = IsInChannelConnected(2);
  bool in4ic = IsInChannelConnected(3);

  printf("%i %i %i %i, ------------------------- \n", in1ic, in2ic, in3ic, in4ic);

#ifdef RTAS_API
  double* in1 = inputs[0];
  double* in2 = inputs[1];
  double* scin1 = inputs[2];

  double* out1 = outputs[0];
  double* out2 = outputs[1];

  double peakL = 0.0, peakR = 0.0, peakLS = 0.0;

  for (int s = 0; s < nFrames; ++s, ++in1, ++in2, ++scin1, ++out1, ++out2)
  {
    *out1 = *in1 * mGain;
    *out2 = *in2 * mGain;

    peakL = IPMAX(peakL, fabs(*in1));
    peakR = IPMAX(peakR, fabs(*in2));
    peakLS = IPMAX(peakLS, fabs(*scin1));
  }

  double xL = (peakL < mPrevL ? METER_DECAY : METER_ATTACK);
  double xR = (peakR < mPrevR ? METER_DECAY : METER_ATTACK);
  double xLS = (peakLS < mPrevLS ? METER_DECAY : METER_ATTACK);

  peakL = peakL * xL + mPrevL * (1.0 - xL);
  peakR = peakR * xR + mPrevR * (1.0 - xR);
  peakLS = peakLS * xLS + mPrevLS * (1.0 - xLS);

  mPrevL = peakL;
  mPrevR = peakR;
  mPrevLS = peakLS;

  if (GetGUI())
  {
    GetGUI()->SetControlFromPlug(mMeterIdx_L, peakL);
    GetGUI()->SetControlFromPlug(mMeterIdx_R, peakR);
    GetGUI()->SetControlFromPlug(mMeterIdx_LS, peakLS);
  }

#else
  double* in1 = inputs[0];
  double* in2 = inputs[1];
  double* scin1 = inputs[2];
  double* scin2 = inputs[3];

  double* out1 = outputs[0];
  double* out2 = outputs[1];

  double peakL = 0.0, peakR = 0.0, peakLS = 0.0, peakRS = 0.0;

//Stupid hack because logic connects the sidechain bus to the main bus when no sidechain is connected
//see coreaudio mailing list
#ifdef AU_API
  if (GetHost() == kHostLogic)
  {
    if(!memcmp(in1, scin1, nFrames * sizeof(double)))
    {
      memset(scin1, 0, nFrames * sizeof(double));
      memset(scin2, 0, nFrames * sizeof(double));
    }
  }
#endif

  for (int s = 0; s < nFrames; ++s, ++in1, ++in2, ++scin1, ++scin2, ++out1, ++out2)
  {
    *out1 = *in1 * mGain;
    *out2 = *in2 * mGain;

    peakL = IPMAX(peakL, fabs(*in1));
    peakR = IPMAX(peakR, fabs(*in2));
    peakLS = IPMAX(peakLS, fabs(*scin1));
    peakRS = IPMAX(peakRS, fabs(*scin2));
  }

  double xL = (peakL < mPrevL ? METER_DECAY : METER_ATTACK);
  double xR = (peakR < mPrevR ? METER_DECAY : METER_ATTACK);
  double xLS = (peakLS < mPrevLS ? METER_DECAY : METER_ATTACK);
  double xRS = (peakRS < mPrevRS ? METER_DECAY : METER_ATTACK);

  peakL = peakL * xL + mPrevL * (1.0 - xL);
  peakR = peakR * xR + mPrevR * (1.0 - xR);
  peakLS = peakLS * xLS + mPrevLS * (1.0 - xLS);
  peakRS = peakRS * xRS + mPrevRS * (1.0 - xRS);

  mPrevL = peakL;
  mPrevR = peakR;
  mPrevLS = peakLS;
  mPrevRS = peakRS;

  if (GetGUI())
  {
    GetGUI()->SetControlFromPlug(mMeterIdx_L, peakL);
    GetGUI()->SetControlFromPlug(mMeterIdx_R, peakR);
    GetGUI()->SetControlFromPlug(mMeterIdx_LS, peakLS);
    GetGUI()->SetControlFromPlug(mMeterIdx_RS, peakRS);
  }
#endif
}

void IPlugSideChain::Reset()
{
  TRACE;
  IMutexLock lock(this);

  //double sr = GetSampleRate();
}

void IPlugSideChain::OnParamChange(int paramIdx)
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