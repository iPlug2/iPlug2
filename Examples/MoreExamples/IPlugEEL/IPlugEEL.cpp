#include "IPlugEEL.h"
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

  kGainX = 10,
  kGainY = 10,
  kKnobFrames = 60
};

void NSEEL_HOSTSTUB_EnterMutex()
{
  //TODO: implement
}

void NSEEL_HOSTSTUB_LeaveMutex()
{
  //TODO: implement
}

IPlugEEL::IPlugEEL(IPlugInstanceInfo instanceInfo)
  :	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo), mGain(1.)
{
  TRACE;
  
  vm = NSEEL_VM_alloc(); // create virtual machine
  
  mVmOutput = NSEEL_VM_regvar(vm, "x"); // register a variable into vm to get a value out

  memset(codetext, 0, 65536);
  strcpy(codetext, "x=rand(2)-1.;");
  
  codehandle = NSEEL_code_compile(vm, codetext, 0); // compile code

  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kGain)->InitDouble("Gain", 50., 0., 100.0, 0.01, "%");
  GetParam(kGain)->SetShape(2.);

  IGraphics* pGraphics = MakeGraphics(this, kWidth, kHeight);
  pGraphics->AttachPanelBackground(&COLOR_RED);

  IBitmap knob = pGraphics->LoadIBitmap(KNOB_ID, KNOB_FN, kKnobFrames);

  pGraphics->AttachControl(new IKnobMultiControl(this, kGainX, kGainY, kGain, &knob));
  
  IRECT textRect(5, 70, kWidth-5, kHeight-5);
  IText textProps(15, &COLOR_BLACK, "Arial", IText::kStyleNormal, IText::kAlignNear, 0, IText::kQualityDefault);
  
  mTextControl = new AlgDisplay(this, textRect, &textProps, codetext);
  pGraphics->AttachControl(mTextControl);

  AttachGraphics(pGraphics);

  //MakePreset("preset 1", ... );
  MakeDefaultPreset((char *) "-", kNumPrograms);
}

IPlugEEL::~IPlugEEL() 
{
  NSEEL_code_free(codehandle);
  NSEEL_VM_free(vm);
}

void IPlugEEL::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  // Mutex is already locked for us.

  double* in1 = inputs[0];
  double* in2 = inputs[1];
  double* out1 = outputs[0];
  double* out2 = outputs[1];

  for (int s = 0; s < nFrames; ++s, ++in1, ++in2, ++out1, ++out2)
  {
    NSEEL_code_execute(codehandle);
    *out1 = *mVmOutput * mGain;
    *out2 = *out1;
  }
}

void IPlugEEL::Reset()
{
  TRACE;
  IMutexLock lock(this);

  //double sr = GetSampleRate();
}

void IPlugEEL::OnParamChange(int paramIdx)
{
  IMutexLock lock(this);

  switch (paramIdx)
  {
    case kGain:
      mGain = GetParam(kGain)->Value() / 100.;
      break;

    default:
      break;
  }
}
