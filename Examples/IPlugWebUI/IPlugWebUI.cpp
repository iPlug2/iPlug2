#include "IPlugWebUI.h"
#include "IPlug_include_in_plug_src.h"

IPlugWebUI::IPlugWebUI(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitGain("Gain", -70., -70, 0.);
  
#ifdef _DEBUG
  SetEnableDevTools(true);
#endif
  
  mEditorInitFunc = [&]() {
    
    // Hard-coded paths must be modified!
#ifdef _DEBUG
  #ifdef OS_WIN
      LoadFile(R"(C:\Users\oli\Dev\iPlug2\Examples\IPlugWebUI\resources\web\index.html)", nullptr);
  #else
      LoadFile(R"(/Users/oli/Dev/iPlug2/Examples/IPlugWebUI/resources/web/index.html)", nullptr, true);
  #endif
#else
    LoadFile("index.html", GetBundleID(), true); // TODO: make this work for windows
#endif
    
    EnableScroll(false);
  };
  
  MakePreset("One", -70.);
  MakePreset("Two", -30.);
  MakePreset("Three", 0.);
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
  if (msgTag == kMsgTagButton1)
    Resize(512, 335);
  else if(msgTag == kMsgTagButton2)
    Resize(1024, 335);
  else if(msgTag == kMsgTagButton3)
    Resize(1024, 768);
  else if (msgTag == kMsgTagBinaryTest)
  {
    auto uint8Data = reinterpret_cast<const uint8_t*>(pData);
    WDL_String msg;
    msg.SetFormatted(64, "Received Data Size %i bytes\n",  dataSize);
    WDL_String caption;
    caption.SetFormatted(64, "Byte values: %i, %i, %i, %i\n", uint8Data[0], uint8Data[1], uint8Data[2], uint8Data[3]);
//    ShowMessageBox(msg.Get(), caption.Get(), EMsgBoxType::kMB_OK,
//    [&](EMsgBoxResult result){
//      SendJSONFromDelegate({"msgboxresult", result});
//    });
    WDL_String fileName;
    WDL_String path;
//    PromptForDirectory(path,
//    [&](const WDL_String& fileName, const WDL_String& path){
//      SendJSONFromDelegate({"chosen_path", path.Get()});
//    });
//
//    PromptForFile(fileName, path, EFileAction::Open, "",
//    [&](const WDL_String& fileName, const WDL_String& path){
//      SendJSONFromDelegate({"chosen_file", fileName.Get(), "chosen_path", path.Get()});
//    });
    
//    OpenURL("https://www.olilarkin.co.uk");
    IPopupMenu menu {"test"};
    menu.AddItem("Hello");
    menu.AddItem("GoodBye");
    CreatePopupMenu(menu, 100, 100, [&](IPopupMenu* pMenu) {
      if (pMenu && pMenu->GetChosenItem())
      {
        SendJSONFromDelegate({"chosen_item", pMenu->GetChosenItem()->GetText()});
      }
      else
      {
        SendJSONFromDelegate({"chosen_item", ""});
      }
    });
  }
  else if (msgTag == kMsgTagMouseUp)
  {
    HideMouseCursor(false, true);
  }
  else if (msgTag == kMsgTagMouseDown)
  {
    HideMouseCursor(true, true);
  }

  return false;
}

void IPlugWebUI::OnIdle()
{
  if (mLastPeak > 0.01)
    SendControlValueFromDelegate(kCtrlTagMeter, mLastPeak);
}

void IPlugWebUI::OnParamChange(int paramIdx)
{
  DBGMSG("gain %f\n", GetParam(paramIdx)->Value());
}

void IPlugWebUI::ProcessMidiMsg(const IMidiMsg& msg)
{
  TRACE;
  
  msg.PrintMsg();
  SendMidiMsg(msg);
}
