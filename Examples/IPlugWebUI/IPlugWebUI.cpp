#include "IPlugWebUI.h"
#include "IPlug_include_in_plug_src.h"

IPlugWebUI::IPlugWebUI(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kInputLevel)->InitGain("Input", 0.0, -20.0, 20.0, 0.1);
  GetParam(kToneBass)->InitDouble("Bass", 5.0, 0.0, 10.0, 0.1);
  GetParam(kToneMid)->InitDouble("Middle", 5.0, 0.0, 10.0, 0.1);
  GetParam(kToneTreble)->InitDouble("Treble", 5.0, 0.0, 10.0, 0.1);
  GetParam(kOutputLevel)->InitGain("Output", 0.0, -40.0, 40.0, 0.1);
  GetParam(kNoiseGateThreshold)->InitGain("Gate", -80.0, -100.0, 0.0, 0.1);
  GetParam(kNoiseGateActive)->InitBool("NoiseGateActive", true);
  GetParam(kEQActive)->InitBool("ToneStack", true);
  GetParam(kOutNorm)->InitBool("OutNorm", false);
#ifdef _DEBUG
  SetEnableDevTools(true);
#endif
  
  mEditorInitFunc = [&]() {
    LoadIndexHtml(__FILE__);
    EnableScroll(false);
  };

  MakePreset("One", -70.);
  MakePreset("Two", -30.);
  MakePreset("Three", 0.);
}

void IPlugWebUI::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
//  const double gain = GetParam(kGain)->DBToAmp();
//  
//  sample maxVal = 0.;
//  
//  mOscillator.ProcessBlock(inputs[0], nFrames); // comment for audio in
//
//  for (int s = 0; s < nFrames; s++)
//  {
//    outputs[0][s] = inputs[0][s] * mGainSmoother.Process(gain);
//    outputs[1][s] = outputs[0][s]; // copy left
//    
//    maxVal += std::fabs(outputs[0][s]);
//  }
//  
//  mLastPeak = static_cast<float>(maxVal / (sample) nFrames);
}

void IPlugWebUI::OnReset()
{
  auto sr = GetSampleRate();
  mOscillator.SetSampleRate(sr);
  mGainSmoother.SetSmoothTime(20., sr);
}

bool IPlugWebUI::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  if (msgTag == -1)
  {
    const char* msg = reinterpret_cast<const char*>(pData);

    WDL_String str;
    str.Set(msg, dataSize);

    auto receivedMsg = nlohmann::json::parse(str.Get(), nullptr, false);

    if (receivedMsg["id"] == "load_model")
    {
      WDL_String fileName;
      WDL_String path;
      std::string extensions = receivedMsg["extensions"].get<std::string>();
      PromptForFile(fileName, path, EFileAction::Open, extensions.c_str(),
                    [&](const WDL_String& fileName, const WDL_String& path){
        nlohmann::json sendMsg;
        sendMsg["id"] = "load_model";
        sendMsg["selected_model"] = fileName.Get();
        SendJSONFromDelegate(sendMsg);
      });
    }
    else if (receivedMsg["id"] == "load_ir")
    {
      WDL_String fileName;
      WDL_String path;
      std::string extensions = receivedMsg["extensions"].get<std::string>();
      PromptForFile(fileName, path, EFileAction::Open, extensions.c_str(),
                    [&](const WDL_String& fileName, const WDL_String& path){
        nlohmann::json sendMsg;
        sendMsg["id"] = "load_ir";
        sendMsg["selected_ir"] = fileName.Get();
        SendJSONFromDelegate(sendMsg);
      });
    }
    else if (receivedMsg["id"] == "show_about")
    {
      ShowMessageBox("Neural Amp Modeler", "By Steve Atkinson & others", EMsgBoxType::kMB_OK);
    }
    
    return true;
  }
    
//    OpenURL("https://www.olilarkin.co.uk");
    // IPopupMenu menu {"test"};
    // menu.AddItem("Hello");
    // menu.AddItem("GoodBye");
    // CreatePopupMenu(menu, 100, 100, [&](IPopupMenu* pMenu) {
    //   if (pMenu && pMenu->GetChosenItem())
    //   {
    //     SendJSONFromDelegate({"chosen_item", pMenu->GetChosenItem()->GetText()});
    //   }
    //   else
    //   {
    //     SendJSONFromDelegate({"chosen_item", ""});
    //   }
    // });
//  }
//  else if (msgTag == kMsgTagMouseUp)
//  {
//    HideMouseCursor(false, true);
//  }
//  else if (msgTag == kMsgTagMouseDown)
//  {
//    HideMouseCursor(true, true);
//  }

  return false;
}

void IPlugWebUI::OnIdle()
{
  if (mLastPeak > 0.01)
    SendControlValueFromDelegate(kCtrlTagMeter, mLastPeak);
}

void IPlugWebUI::OnParamChange(int paramIdx)
{
}

void IPlugWebUI::ProcessMidiMsg(const IMidiMsg& msg)
{
  TRACE;
  
  msg.PrintMsg();
  SendMidiMsg(msg);
}
