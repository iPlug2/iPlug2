#include "IPlugResponsiveUI.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "Test/TestSizeControl.h"

IPlugResponsiveUI::IPlugResponsiveUI(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPrograms))
{
  GetParam(kGain)->InitGain("Gain", -70.);

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
#ifdef OS_WEB
    int w, h;
    GetScreenDimensions(w, h);
    return MakeGraphics(*this, w, h, 1.f);
#else
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_HEIGHT));
#endif
  };

  mLayoutFunc = [&](IGraphics* pGraphics) {
    const IRECT b = pGraphics->GetBounds();

    auto GetBounds = [pGraphics](int ctrlIdx, const IRECT& b) {
      IRECT main = b.GetPadded(-40.f);
      IRECT keys = main.FracRectVertical(0.25, false);
      IRECT scope = main.FracRectVertical(0.75, true).GetPadded(-10.f);
      IRECT gain = scope.ReduceFromRight(100.f);
      switch (ctrlIdx) {
        case 2: return keys;
        case 3: return gain;
        case 4: return scope;
        case 0:
        case 1: return b;
        default: return pGraphics->GetControl(ctrlIdx)->GetRECT();
      }
    };

    // Layout controls on resize
    if(pGraphics->NControls()) {
      for (int ctrlIdx = 0; ctrlIdx < pGraphics->NControls(); ctrlIdx++) {
        pGraphics->GetControl(ctrlIdx)->SetTargetAndDrawRECTs(GetBounds(ctrlIdx, b));
      }
      return;
    }

    pGraphics->SetLayoutOnResize(true);
    // pGraphics->AttachCornerResizer(EUIResizerMode::Size, true);
    pGraphics->SetSizeConstraints(10,10000,10,10000);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->AttachPopupMenuControl();

    //Create controls
    pGraphics->AttachPanelBackground(COLOR_GREEN);
    pGraphics->AttachControl(new TestSizeControl(GetBounds(1, b)))->Hide(true);
    pGraphics->AttachControl(new IVKeyboardControl(GetBounds(2, b)));
    pGraphics->AttachControl(new IVSliderControl(GetBounds(3, b), kGain));
    pGraphics->AttachControl(new IVScopeControl<>(GetBounds(4, b), "", DEFAULT_STYLE.WithColor(kBG, COLOR_BLACK).WithColor(kFG, COLOR_WHITE)), kCtrlTagScope);

//    pGraphics->AttachImGui([](IGraphics* pGraphics){
//      static bool liveEdit = false;
//      static bool showFPS = false;
//      static bool showDrawnArea = false;
//      static bool showControlBounds = false;
//      static float bgColor [3] = {0.f, 1.f, 0.f};
//      static float scopeBgColor [3] = {0.f, 0.f, 0.f};
//      static float scopeFgColor [3] = {1.f, 1.f, 1.f};
//      if(ImGui::Checkbox("Live Edit", &liveEdit)) pGraphics->EnableLiveEdit(liveEdit);
//      if(ImGui::Checkbox("Show FPS", &showFPS)) pGraphics->ShowFPSDisplay(showFPS);
//      if(ImGui::Checkbox("Show Drawn Area", &showDrawnArea)) pGraphics->ShowAreaDrawn(showDrawnArea);
//      if(ImGui::Checkbox("Show Ctrl Bounds", &showControlBounds)) pGraphics->ShowControlBounds(showControlBounds);
//      ImGui::SetColorEditOptions(ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs);
//      if(ImGui::ColorEdit3("BG Color", bgColor)) pGraphics->GetBackgroundControl()->As<IPanelControl>()->SetPattern(IColor::FromRGBf(bgColor));
//      if(ImGui::ColorEdit3("Scope BG Color", scopeBgColor)) pGraphics->GetControlWithTag(kCtrlTagScope)->As<IVScopeControl<>>()->SetColor(kBG, IColor::FromRGBf(scopeBgColor));
//      if(ImGui::ColorEdit3("Scope FG Color", scopeFgColor)) pGraphics->GetControlWithTag(kCtrlTagScope)->As<IVScopeControl<>>()->SetColor(kFG, IColor::FromRGBf(scopeFgColor));
//    });
  };
#endif
}

#if IPLUG_EDITOR
void IPlugResponsiveUI::OnParentWindowResize(int width, int height)
{
  GetUI()->Resize(width, height, 1.f, false);
}
#endif

#if IPLUG_DSP
void IPlugResponsiveUI::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->DBToAmp();
  
  for (int s = 0; s < nFrames; s++) {
    outputs[0][s] = mOsc.Process() * gain;
    outputs[1][s] = outputs[0][s];
  }
  
  mScopeSender.ProcessBlock(outputs, nFrames, kCtrlTagScope, 1);
}

void IPlugResponsiveUI::ProcessMidiMsg(const IMidiMsg& msg)
{
  TRACE;
  
  int status = msg.StatusMsg();
  
  switch (status)
  {
    case IMidiMsg::kNoteOn:
//    case IMidiMsg::kNoteOff:
    {
      goto handle;
    }
    default:
      return;
  }
  
handle:
  
  auto midi2CPS = [](int pitch) {
    return 440. * pow(2., (pitch - 69.) / 12.);
  };
  
  mOsc.SetFreqCPS(midi2CPS(msg.NoteNumber()));
  SendMidiMsg(msg);
}

void IPlugResponsiveUI::OnIdle()
{
  mScopeSender.TransmitData(*this);
}

#endif
