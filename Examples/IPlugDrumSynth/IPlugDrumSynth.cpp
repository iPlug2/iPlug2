#include "IPlugDrumSynth.h"
#include "IPlug_include_in_plug_src.h"

#if IPLUG_EDITOR
#include "IControls.h"

class DrumPadControl : public IControl,
                       public IVectorBase
{
public:
  DrumPadControl(const IRECT& bounds, const IVStyle& style, int midiNoteNumber)
  : IControl(bounds)
  , IVectorBase(style)
  , mMidiNoteNumber(midiNoteNumber)
  {
    mDblAsSingleClick = true;
    AttachIControl(this, "");
  }
  
  void Draw(IGraphics& g) override
  {
//    g.FillRect(mFlash ? COLOR_WHITE : COLOR_BLACK, mRECT);
    DrawPressableShape(g, EVShape::AllRounded, mRECT, mFlash, mMouseIsOver, false);
    mFlash = false;
  }
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    TriggerAnimation();
    IMidiMsg msg;
    msg.MakeNoteOnMsg(mMidiNoteNumber, 127, 0);
    GetDelegate()->SendMidiMsgFromUI(msg);
  }
  
  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    IMidiMsg msg;
    msg.MakeNoteOffMsg(mMidiNoteNumber, 0);
    GetDelegate()->SendMidiMsgFromUI(msg);
  }
  
  void TriggerAnimation()
  {
    mFlash = true;
    SetAnimation(SplashAnimationFunc, DEFAULT_ANIMATION_DURATION);
  }

private:
  bool mFlash = false;
  int mMidiNoteNumber;
};
#endif

IPlugDrumSynth::IPlugDrumSynth(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPrograms))
{
  GetParam(kParamGain)->InitDouble("Gain", 100., 0., 100.0, 0.01, "%");

#if IPLUG_EDITOR // All UI methods and member variables should be within an IPLUG_EDITOR guard, should you want distributed UI
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, 1.);
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_WHITE);
    pGraphics->EnableMouseOver(true);
//    pGraphics->EnableMultiTouch(true);
//    pGraphics->ShowFPSDisplay(true);
//    pGraphics->EnableLiveEdit(true);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    const IRECT b = pGraphics->GetBounds().GetPadded(-5);
    
    IVStyle style = DEFAULT_STYLE.WithRoundness(0.1f).WithFrameThickness(3.f);
    for (int i=0; i<kNumDrums; i++) {
      pGraphics->AttachControl(new DrumPadControl(b.GetGridCell(i, 3, 3).GetPadded(-10.f), style, 60 + i), i /* controlTag */);
    }
  };
#endif
}

#if IPLUG_EDITOR
void IPlugDrumSynth::OnMidiMsgUI(const IMidiMsg& msg)
{
  if(GetUI() && msg.StatusMsg() == IMidiMsg::kNoteOn)
  {
    int pitchClass = msg.NoteNumber() % 12;

    if(pitchClass < kNumDrums)
    {
      DrumPadControl* pPad = dynamic_cast<DrumPadControl*>(GetUI()->GetControlWithTag(pitchClass));
      pPad->SetSplashPoint(pPad->GetRECT().MW(), pPad->GetRECT().MH());
      pPad->TriggerAnimation();
    }
  }
}
#endif

#if IPLUG_DSP
void IPlugDrumSynth::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kParamGain)->Value() / 100.;
  const int nChans = NOutChansConnected();

  mDSP.ProcessBlock(outputs, nFrames);
  
  for (auto s = 0; s < nFrames; s++) {
    for (auto c = 0; c < nChans; c++) {
      outputs[c][s] = outputs[c][s] * gain;
    }
  }
}

void IPlugDrumSynth::OnReset()
{
  mDSP.Reset(GetSampleRate(), GetBlockSize());
}

void IPlugDrumSynth::ProcessMidiMsg(const IMidiMsg& msg)
{
  TRACE;
  
  int status = msg.StatusMsg();
  
  switch (status)
  {
    case IMidiMsg::kNoteOn:
    case IMidiMsg::kNoteOff:
    {
      goto handle;
    }
    default:
      return;
  }
  
handle:
  mDSP.ProcessMidiMsg(msg);
  SendMidiMsg(msg);
}

void IPlugDrumSynth::OnParamChange(int paramIdx)
{
  switch (paramIdx)
  {
    default:
      break;
  }
}

bool IPlugDrumSynth::GetMidiNoteText(int noteNumber, char* name) const
{
  int pitch = noteNumber % 12;
  
  WDL_String str;
  str.SetFormatted(32, "Drum %i", pitch);
  
  strcpy(name, str.Get());
  
//  switch (pitch)
//  {
//    case 0:
//      strcpy(name, "drum 1");
//      return true;
//    default:
//      *name = '\0';
//      return false;
//  }
  
  return true;
}
#endif
