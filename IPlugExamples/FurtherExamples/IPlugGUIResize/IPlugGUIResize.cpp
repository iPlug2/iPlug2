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
};

IPlugGUIResize::IPlugGUIResize(IPlugInstanceInfo instanceInfo)
  :	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{
  TRACE;

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
    case 0: // could change the positioning of the controls by storing a size index when the button is clicked
      pGraphics->AttachPanelBackground(&COLOR_RED);
      pGraphics->AttachControl(new IGUIResizeButton(this, IRECT(10, 10, 80, 30), "mini", 152, 152 ));
      pGraphics->AttachControl(new IGUIResizeButton(this, IRECT(10, 35, 80, 55), "medium", 300, 300 ));
      pGraphics->AttachControl(new IGUIResizeButton(this, IRECT(10, 60, 80, 80), "big", 500, 500 ));
      break;
    case 1:
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

  for (int s = 0; s < nFrames; ++s, ++in1, ++in2, ++out1, ++out2)
  {
    *out1 = *in1;
    *out2 = *in2;
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

//  switch (paramIdx)
//  {
//    case kGain:
//      mGain = GetParam(kGain)->Value() / 100.;;
//      break;
//
//    default:
//      break;
//  }
}

void IPlugGUIResize::OnWindowResize()
{
  CreateControls(GetGUI(), 0);
}