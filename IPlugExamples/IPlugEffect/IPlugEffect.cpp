#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "config.h"

class MyControl : public IControl
{
private:
  double mPhase = 0.;
public:
  MyControl(IPlugBaseGraphics& plug, IRECT rect)
  : IControl(plug, rect)
  {
  }
  
  void Draw(IGraphics& g) override
  {
    //g.DrawRect(COLOR_BLUE, mRECT.GetPadded(-50));
    g.FillRect(COLOR_BLUE, mRECT.GetScaled((float) mPhase));
    //g.FillRect(COLOR_GREEN, mRECT.GetScaled(1.-mPhase));
  }
  
  bool IsDirty() override
  {
    mPhase += 0.01;
    
    if (mPhase > 1.)
      mPhase-=1.;
    
    return true;
  }
};


IPlugEffect::IPlugEffect(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{
  TRACE; 

  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");
  GetParam(kGain)->SetShape(2.);

  //create user interface
  IGraphics* pGraphics = MakeGraphics(*this, kWidth, kHeight, 30);
  pGraphics->AttachPanelBackground(COLOR_RED);
  
  // lamda function for custom actions on stock IControls
  pGraphics->AttachControl(new IVSwitchControl(*this, IRECT(kGainX, kGainY, kGainX + 200, kGainY + 200), kNoParameter, [](IControl* pControl)
  {
    //pGraphics->ShowMessageBox("do you like lambdas?", "", MB_YESNO); // blocks
    pControl->Hide(true);
  }));
  
//  pGraphics->AttachControl(new IVKnobControl(*this, IRECT(kGainX, kGainY, kGainX + 100, kGainY + 100), kGain, COLOR_BLACK));

  AttachGraphics(pGraphics);
//  pGraphics->EnableLiveEdit(true);
  PrintDebugInfo();

  //MakePreset("preset 1", ... );
  MakeDefaultPreset("-", kNumPrograms);
}

IPlugEffect::~IPlugEffect() {}

void IPlugEffect::ProcessBlock(double** inputs, double** outputs, int nFrames)
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
