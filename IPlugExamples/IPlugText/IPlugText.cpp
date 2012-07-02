#include "IPlugText.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "resource.h"
#include "IBitmapMonoText.h"

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

IPlugText::IPlugText(IPlugInstanceInfo instanceInfo)
  :	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo), mGain(1.)
{
  TRACE;

  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kGain)->InitDouble("Gain", 50., 0., 100.0, 0.01, "%");
  GetParam(kGain)->SetShape(2.);

  IGraphics* pGraphics = MakeGraphics(this, kWidth, kHeight);
  pGraphics->AttachPanelBackground(&COLOR_GRAY);

  IRECT tmpRect(10, 10, 200, 30);
  IText textProps(12, &COLOR_BLACK, "Arial", IText::kStyleNormal, IText::kAlignCenter, 0, IText::kQualityDefault);
  pGraphics->AttachControl(new ITextControl(this, tmpRect, &textProps, "hello iplug!"));

  IRECT tmpRect2(30, 30, 200, 60);
  IText textProps2(18, &COLOR_WHITE, "Tahoma", IText::kStyleNormal, IText::kAlignCenter, 0, IText::kQualityDefault);
  pGraphics->AttachControl(new ITextControl(this, tmpRect2, &textProps2, "hello iplug!"));

  IRECT tmpRect3(80, 50, 200, 80.);
  IText textProps3(24, &COLOR_RED, "Arial", IText::kStyleItalic, IText::kAlignFar, 0, IText::kQualityDefault);
  pGraphics->AttachControl(new ITextControl(this, tmpRect3, &textProps3, "hello iplug!"));

  IRECT tmpRect4(120, 60, 300, 120);
  IText textProps4(40, &COLOR_ORANGE, "Arial", IText::kStyleNormal, IText::kAlignCenter, 0, IText::kQualityDefault);
  pGraphics->AttachControl(new ITextControl(this, tmpRect4, &textProps4, "hello iplug!"));

  IRECT tmpRect5(10, 100, 400, 170);
  IText textProps5(50, &COLOR_BLUE, "Courier", IText::kStyleNormal, IText::kAlignCenter, 0, IText::kQualityDefault);
  
  pGraphics->MeasureIText(&textProps5, "hello iplug!", &tmpRect5); // get the bounds of the text and stick them in tmpRect5
  pGraphics->AttachControl( new IPanelControl(this, tmpRect5, &COLOR_WHITE));
  
  tmpRect5 = IRECT(10, 100, 400, 170);
  pGraphics->AttachControl(new ITextControl(this, tmpRect5, &textProps5, "hello iplug!"));
  
  DBGMSG("text bounds = %i, %i, %i, %i\n", tmpRect5.L, tmpRect5.T, tmpRect5.R, tmpRect5.B);

  IBitmap blackText = pGraphics->LoadIBitmap(TEXT_BLACK_ID, TEXT_BLACK_FN, 95, true);
  IBitmap whiteText = pGraphics->LoadIBitmap(TEXT_WHITE_ID, TEXT_WHITE_FN, 95, true);

  IRECT tmpRect6(10, 250, 400, 170);
  pGraphics->AttachControl(new IBitmapTextControl(this, tmpRect6, &blackText, "i'm bitmap monospace text"));

  IRECT tmpRect7(10, 280, 400, 170);
  pGraphics->AttachControl(new IBitmapTextControl(this, tmpRect7, &whiteText, "i'm also bitmap monospace text"));

  AttachGraphics(pGraphics);
  //MakePreset("preset 1", ... );
  MakeDefaultPreset((char *) "-", kNumPrograms);
}

IPlugText::~IPlugText() {}

void IPlugText::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
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

void IPlugText::Reset()
{
  TRACE;
  IMutexLock lock(this);

  //double sr = GetSampleRate();
}

void IPlugText::OnParamChange(int paramIdx)
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