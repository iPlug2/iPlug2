#include "IPlugWebView.h"
#include "IPlug_include_in_plug_src.h"

IPlugWebView::IPlugWebView(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");
  
  mEditorInitFunc = [&](){
    LoadFileFromBundle("web/index.html");
  };
}

void IPlugWebView::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  const int nChans = NOutChansConnected();

  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s] * gain;
    }
  }
}

bool IPlugWebView::OnMessage(int messageTag, int controlTag, int dataSize, const void* pData)
{
  if(messageTag == kMsgTagButton)
  {
    DBGMSG("Button Clicked\n");
  }
  
  return false;
}
