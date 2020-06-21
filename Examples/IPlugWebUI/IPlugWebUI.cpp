#include "IPlugWebUI.h"
#include "IPlug_include_in_plug_src.h"

IPlugWebUI::IPlugWebUI(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitGain("Gain", 0., -70, 0.);

  // Hard-coded paths must be modified!
#ifdef OS_WIN
  SetWebViewPaths("C:\\Users\\oli\\Dev\\iPlug2\\Examples\\IPlugWebUI\\WebView2Loader.dll", "C:\\Users\\oli\\Dev\\iPlug2\\Examples\\IPlugWebUI\\");
#endif

  mEditorInitFunc = [&]() {
#ifdef OS_WIN
    LoadFile("C:\\Users\\oli\\Dev\\iPlug2\\Examples\\IPlugWebUI\\resources\\web\\index.html", nullptr);
#else
    LoadFile("index.html", GetBundleID());
#endif
    
    EnableScroll(false);
  };
  
  MakePreset("One", 0.);
  MakePreset("Two", -30.);
  MakePreset("Three", 40.);
}

void IPlugWebUI::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->DBToAmp();
  
  sample maxVal = 0.;
  
  mOscillator.ProcessBlock(inputs[0], nFrames); // comment for audio in

  for (int s = 0; s < nFrames; s++)
  {
    outputs[0][s] = inputs[0][s] * mGainSmoother.Process(gain);
    outputs[1][s] = outputs[0][s]; // copy left
    
    maxVal += std::fabs(outputs[0][s]);
  }
  
  mLastPeak = static_cast<float>(maxVal / (sample) nFrames);
}

void IPlugWebUI::OnReset()
{
  auto sr = GetSampleRate();
  mOscillator.SetSampleRate(sr);
  mGainSmoother.SetSmoothTime(20., sr);
}

bool IPlugWebUI::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  if(msgTag == kMsgTagButton1)
    Resize(512, 335);
  else if(msgTag == kMsgTagButton2)
    Resize(1024, 335);
  else if(msgTag == kMsgTagButton3)
    Resize(1024, 768);

  return false;
}

void IPlugWebUI::OnIdle()
{
  if(mLastPeak > 0.01)
    SendControlValueFromDelegate(kCtrlTagMeter, mLastPeak);
}

void IPlugWebUI::OnParamChange(int paramIdx)
{
  DBGMSG("gain %f\n", GetParam(paramIdx)->Value());
}
