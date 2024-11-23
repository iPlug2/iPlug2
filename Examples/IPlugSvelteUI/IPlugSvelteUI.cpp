#include "IPlugSvelteUI.h"
#include "IPlug_include_in_plug_src.h"
#include "IPlugPaths.h"

IPlugSvelteUI::IPlugSvelteUI(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitGain("Gain", -70., -70, 0.);
  
//#ifdef DEBUG
  SetCustomUrlScheme("iplug2");
  SetEnableDevTools(true);
//#endif
  
  mEditorInitFunc = [&]() {
    LoadIndexHtml(__FILE__, GetBundleID());
    // LoadURL("http://localhost:5173/");
    EnableScroll(false);
  };
  
  MakePreset("One", -70.);
  MakePreset("Two", -30.);
  MakePreset("Three", 0.);
}

void IPlugSvelteUI::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
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

void IPlugSvelteUI::OnReset()
{
  auto sr = GetSampleRate();
  mOscillator.SetSampleRate(sr);
  mGainSmoother.SetSmoothTime(20., sr);
}

bool IPlugSvelteUI::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  if (msgTag == kMsgTagButton1)
    Resize(512, 335);
  else if(msgTag == kMsgTagButton2)
    Resize(1024, 335);
  else if(msgTag == kMsgTagButton3)
    Resize(1024, 768);
  else if (msgTag == kMsgTagBinaryTest)
  {
    auto uint8Data = reinterpret_cast<const uint8_t*>(pData);
    DBGMSG("Data Size %i bytes\n",  dataSize);
    DBGMSG("Byte values: %i, %i, %i, %i\n", uint8Data[0], uint8Data[1], uint8Data[2], uint8Data[3]);
  }

  return false;
}

void IPlugSvelteUI::OnIdle()
{
  if (mLastPeak > 0.01)
    SendControlValueFromDelegate(kCtrlTagMeter, mLastPeak);
}

void IPlugSvelteUI::OnParamChange(int paramIdx)
{
  DBGMSG("gain %f\n", GetParam(paramIdx)->Value());
}

void IPlugSvelteUI::ProcessMidiMsg(const IMidiMsg& msg)
{
  TRACE;
  
  msg.PrintMsg();
  SendMidiMsg(msg);
}
