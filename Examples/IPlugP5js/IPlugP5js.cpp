#include "IPlugP5js.h"
#include "IPlug_include_in_plug_src.h"

IPlugP5js::IPlugP5js(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitGain("Gain", -70., -70, 0.);

//#ifdef OS_WIN
//  WDL_String tmpPath;
//  AppSupportPath(tmpPath);
//  tmpPath.AppendFormatted(MAX_PATH, "\\IPlugWebUI\\WebView\\%s", GetAPIStr());
//  SetWebViewTmpPath(tmpPath.Get());
//#endif

  mEditorInitFunc = [&]() {
    #ifdef OS_WIN
    LoadFile("C:\\Users\\oli\\Dev\\iPlug2\\Examples\\IPlugP5js\\resources\\web\\index.html", nullptr);
    #else
    LoadFile("index.html", GetBundleID(), true);
    #endif
    EnableScroll(false);
  };
  
  MakeDefaultPreset();
}

void IPlugP5js::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  mSender.ProcessBlock(inputs, nFrames, 0);
}

void IPlugP5js::ProcessMidiMsg(const IMidiMsg& msg)
{
}

void IPlugP5js::OnReset()
{
//  auto sr = GetSampleRate();
}

bool IPlugP5js::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  return false;
}

void IPlugP5js::OnIdle()
{
  mSender.TransmitData(*this);
}

void IPlugP5js::OnParamChange(int paramIdx)
{
  DBGMSG("gain %f\n", GetParam(paramIdx)->Value());
}

void IPlugP5js::OnMidiMsgUI(const IMidiMsg& msg)
{
  
}
