#include "IPlugMidiEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

IPlugMidiEffect::IPlugMidiEffect(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kParamGain)->InitDouble("Gain", 100., 0., 100.0, 0.01, "%");
  
#if IPLUG_DSP
  SetTailSize(4410000);
#endif
  
#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    
    auto actionFunc = [&](IControl* pCaller) {
      static bool onoff = false;
      onoff = !onoff;
      IMidiMsg msg;
      constexpr int pitches[3] = {60, 65, 67};
      
      for (int i = 0; i<3; i++)
      {
        if(onoff)
          msg.MakeNoteOnMsg(pitches[i], 60, 0);
        else
          msg.MakeNoteOffMsg(pitches[i], 0);
      
        SendMidiMsgFromUI(msg);
      }
      
      SplashClickActionFunc(pCaller);
    };
    
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->AttachControl(new IVButtonControl(pGraphics->GetBounds().GetPadded(-10), actionFunc, "Trigger Chord"));
  };
#endif
}

#if IPLUG_DSP
void IPlugMidiEffect::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kParamGain)->Value() / 100.;
  const int nChans = NOutChansConnected();

  for (auto s = 0; s < nFrames; s++) {
    for (auto c = 0; c < nChans; c++) {
      outputs[c][s] = outputs[c][s] * gain;
    }
  }
}

void IPlugMidiEffect::ProcessMidiMsg(const IMidiMsg& msg)
{
  TRACE;
  
  int status = msg.StatusMsg();
  
  switch (status)
  {
    case IMidiMsg::kNoteOn:
    case IMidiMsg::kNoteOff:
    case IMidiMsg::kPolyAftertouch:
    case IMidiMsg::kControlChange:
    case IMidiMsg::kProgramChange:
    case IMidiMsg::kChannelAftertouch:
    case IMidiMsg::kPitchWheel:
    {
      goto handle;
    }
    default:
      return;
  }
  
handle:
  SendMidiMsg(msg);
}
#endif
