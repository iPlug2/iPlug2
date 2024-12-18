#include "IPlugOSCEditor.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "IWebViewControl.h"

IPlugOSCEditor::IPlugOSCEditor(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
, OSCReceiver(8000)
, OSCSender("127.0.0.1", 8000)
{
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");

  auto logFunc = [&](WDL_String& log) {
    IGraphics* pGraphics = GetUI();
    
    if (pGraphics)
      pGraphics->GetControlWithTag(kCtrlTagWebView)->As<IWebViewControl>()->LoadHTML(log.Get());
    
    DBGMSG("%s\n", log.Get());
  };
  
  OSCReceiver::SetLogFunc(logFunc);
  OSCSender::SetLogFunc(logFunc);

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->EnableMouseOver(true);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    const IRECT b = pGraphics->GetBounds().GetPadded(-10.f);
    
    auto setSendIPAndPort = [&, pGraphics](IControl* pCaller) {
      WDL_String log;
      const char* ip = pGraphics->GetControlWithTag(kCtrlTagSendIP)->As<IEditableTextControl>()->GetStr();
      int port = static_cast<int>(pGraphics->GetControlWithTag(kCtrlTagSendPort)->As<IVNumberBoxControl>()->GetRealValue());
      SetDestination(ip, port);
    };
    
    IRECT topRow = b.SubRectVertical(3, 0).GetMidVPadded(40.f);
    IRECT bottomRow = b.SubRectVertical(3, 2);
    pGraphics->AttachControl(new IVLabelControl(topRow.SubRectHorizontal(3, 0).GetPadded(-10.f), "Send IP", DEFAULT_STYLE.WithValueText(DEFAULT_LABEL_TEXT).WithDrawShadows(false)));
    pGraphics->AttachControl(new IEditableTextControl(topRow.SubRectHorizontal(3, 0).GetPadded(-10.f).GetFromBottom(44.f), "127.0.0.1"), kCtrlTagSendIP)->SetActionFunction(setSendIPAndPort)->SetTextEntryLength(32);
    
    pGraphics->AttachControl(new IVNumberBoxControl(topRow.SubRectHorizontal(3, 1).GetPadded(-10.f), kNoParameter, setSendIPAndPort, "Send Port", DEFAULT_STYLE, true, 8000, 4000, 10000, "%0.0f", false), kCtrlTagSendPort);
    
    pGraphics->AttachControl(new IVNumberBoxControl(topRow.SubRectHorizontal(3, 2).GetPadded(-10.f), kNoParameter, [&](IControl* pCaller){
      SetReceivePort(static_cast<int>(pCaller->As<IVNumberBoxControl>()->GetRealValue()));
    }, "Receive Port", DEFAULT_STYLE, true, 8000, 4000, 10000, "%0.0f", false));
    
    pGraphics->AttachControl(new IVKnobControl(b.GetCentredInside(100), [&](IControl* pCaller) {
                                                OscMessageWrite msg;
                                                msg.PushWord("gain");
                                                msg.PushFloatArg((float) pCaller->GetValue());
                                                SendOSCMessage(msg);}
                                               , "Gain"), kCtrlTagGain);
    
    bool showDevTools = false;
    
#if DEBUG
    showDevTools = true;
#endif

    pGraphics->AttachControl(new IWebViewControl(bottomRow, true, [](IWebViewControl* pControl){
      pControl->LoadHTML("...");
    }, [](IWebViewControl* pControl, const char* jsonMsg){
      // You can handle key up/down events on the WebView here (or any other messages that you send from the UI)
      DBGMSG("Received message %s\n", jsonMsg);
    }, showDevTools), kCtrlTagWebView)->Hide(true);
        
    pGraphics->AttachControl(new IVButtonControl(bottomRow.GetFromTop(20).GetFromLeft(200).GetVShifted(-20),
    [](IControl* pControl){
      SplashClickActionFunc(pControl);
    }, "Show OSC Console", DEFAULT_STYLE.WithDrawShadows(false).WithDrawFrame(false)))->SetAnimationEndActionFunction([](IControl* pControl){
      pControl->GetUI()->GetControlWithTag(kCtrlTagWebView)->Hide(!pControl->GetUI()->GetControlWithTag(kCtrlTagWebView)->IsHidden());
    });
    
  };
#endif
}

#if IPLUG_DSP
void IPlugOSCEditor::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  const int nChans = NOutChansConnected();
  
  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s] * gain;
    }
  }
}
#endif

void IPlugOSCEditor::OnOSCMessage(OscMessageRead& msg)
{
  int nArgs = msg.GetNumArgs();
  char type;
  int index = 0;

  IGraphics* pGraphics = GetUI();

  if(strcmp(msg.GetMessage(), "/gain") == 0)
  {
    auto* pValue = msg.PopFloatArg(true);
    if (pValue) {
      if(pGraphics)
        pGraphics->GetControlWithTag(kCtrlTagGain)->SetValueFromDelegate(*pValue);
    }
  }

  WDL_String oscStr;

  oscStr.Append(msg.GetMessage());

  while (nArgs) {
    msg.GetIndexedArg(index, &type);

    switch (type) {
      case 'i':
      {
        auto* pValue = msg.PopIntArg(false);
        if (pValue)
          oscStr.AppendFormatted(256, " %i", *pValue);
        break;
      }
      case 'f':
      {
        auto* pValue = msg.PopFloatArg(false);
        if (pValue)
          oscStr.AppendFormatted(256, " %f", *pValue);
        break;
      }
      case 's':
      {
        auto* pValue = msg.PopStringArg(false);
        if (pValue)
          oscStr.AppendFormatted(256, " %s", pValue);
        break;
      }
      default :
        break;
    }
    nArgs--;
  }

  if(oscStr.GetLength()) {
    WDL_String str;
    str.Set("Received message: ");
    str.Append(&oscStr);
    DBGMSG("%s\n", str.Get());
    
    if(pGraphics) {
      pGraphics->GetControlWithTag(kCtrlTagWebView)->As<IWebViewControl>()->LoadHTML(str.Get());
    }
  }
}
