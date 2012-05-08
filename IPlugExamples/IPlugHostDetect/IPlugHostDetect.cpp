#include "IPlugHostDetect.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "resource.h"
#include "IBitmapMonoText.h"

const int kNumPrograms = 1;

enum EParams
{
  kNumParams = 0
};

enum ELayout
{
  kWidth = GUI_WIDTH,
  kHeight = GUI_HEIGHT,
};

IPlugHostDetect::IPlugHostDetect(IPlugInstanceInfo instanceInfo)
  :	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{
  TRACE;

  IGraphics* pGraphics = MakeGraphics(this, kWidth, kHeight);
  pGraphics->AttachPanelBackground(&COLOR_GRAY);

  IRECT tmpRect(10, 10, 200, 50);
  IText textProps(40, &COLOR_ORANGE, "Arial", IText::kStyleNormal, IText::kAlignCenter, 0, IText::kQualityDefault);
  mHostNameControl = new ITextControl(this, tmpRect, &textProps, "");
  pGraphics->AttachControl(mHostNameControl);

  tmpRect = IRECT(tmpRect.L, tmpRect.T+40, tmpRect.R, tmpRect.B+40);
  mHostVersionControl = new ITextControl(this, tmpRect, &textProps, "");
  pGraphics->AttachControl(mHostVersionControl);

  AttachGraphics(pGraphics);
  //MakePreset("preset 1", ... );
  MakeDefaultPreset((char *) "-", kNumPrograms);
}

IPlugHostDetect::~IPlugHostDetect() {}

void IPlugHostDetect::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
}

void IPlugHostDetect::Reset()
{
  TRACE;
  IMutexLock lock(this);

  //double sr = GetSampleRate();
}

void IPlugHostDetect::OnParamChange(int paramIdx)
{
  IMutexLock lock(this);
}

void IPlugHostDetect::OnHostIdentified()
{
  char hostText[128];
  GetHostNameStr(GetHost(), hostText);
  mHostNameControl->SetTextFromPlug(hostText);
  GetHostVersionStr(hostText);
  mHostVersionControl->SetTextFromPlug(hostText);
}