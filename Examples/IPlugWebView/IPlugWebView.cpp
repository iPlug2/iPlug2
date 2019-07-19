#include "IPlugWebView.h"
#include "IPlug_include_in_plug_src.h"

IPlugWebView::IPlugWebView(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");
  
  mEditorInitFunc = [&](){
    LoadFileFromBundle("web/index.html");
  };
  
  MakePreset("One", 0.);
  MakePreset("Two", 10.);
  MakePreset("Three", 100.);
}

void IPlugWebView::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  
  sample maxValL = 0.;
  sample maxValR = 0.;
  
  mOscillator.ProcessBlock(inputs[0], nFrames);

  for (int s = 0; s < nFrames; s++)
  {
    outputs[0][s] = inputs[0][s] * gain;
    outputs[1][s] = inputs[1][s] * gain;
    
    maxValL += std::fabs(outputs[0][s]);
    maxValR += std::fabs(outputs[1][s]);
  }
  
  mLastPeakL = maxValL / (sample) nFrames;
  mLastPeakR = maxValR / (sample) nFrames;
}

bool IPlugWebView::OnMessage(int messageTag, int controlTag, int dataSize, const void* pData)
{
  if(messageTag == kMsgTagButton1) {
    Resize(512, 335);
  }
  else if(messageTag == kMsgTagButton2) {
    Resize(1024, 335);
  }
  else if(messageTag == kMsgTagButton3) {
    Resize(1024, 768);
  }
  
  return false;
}

void IPlugWebView::OnIdle()
{
  SendControlValueFromDelegate(0, mLastPeakL);
  SendControlValueFromDelegate(1, mLastPeakR);
}
