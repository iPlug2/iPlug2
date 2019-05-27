#include "IPlugInstrument.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

IPlugInstrument::IPlugInstrument(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{
  GetParam(kParamGain)->InitDouble("Gain", 100., 0., 100.0, 0.01, "%");
  GetParam(kParamNoteGlideTime)->InitSeconds("Note Glide Time", 0.1, 2.);
  GetParam(kParamAttack)->InitDouble("Attack", 10., 1., 1000., 0.1, "ms", IParam::kFlagsNone, "ADSR", IParam::ShapePowCurve(3.));
  GetParam(kParamDecay)->InitDouble("Decay", 10., 1., 1000., 0.1, "ms", IParam::kFlagsNone, "ADSR", IParam::ShapePowCurve(3.));
  GetParam(kParamSustain)->InitDouble("Sustain", 50., 0., 100., 1, "%", IParam::kFlagsNone, "ADSR");
  GetParam(kParamRelease)->InitDouble("Release", 10., 1., 1000., 0.1, "ms", IParam::kFlagsNone, "ADSR");
  
#if IPLUG_EDITOR // All UI methods and member variables should be within an IPLUG_EDITOR guard, should you want distributed UI
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, 1.);
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(kUIResizerScale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->HandleMouseOver(true);
//    pGraphics->EnableLiveEdit(true);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    const IRECT b = pGraphics->GetBounds();
    pGraphics->AttachControl(new IVKeyboardControl(IRECT(10, 335, PLUG_WIDTH-10, PLUG_HEIGHT-10)), kCtrlTagKeyboard);
    pGraphics->AttachControl(new IVMultiSliderControl<4>(b.GetGridCell(0, 2, 2).GetPadded(-30), "", DEFAULT_STYLE, kParamAttack, kVertical, 0.f, 1.f));
    const IRECT controls = b.GetGridCell(1, 2, 2);
    pGraphics->AttachControl(new IVKnobControl(controls.GetGridCell(0, 2, 6).GetCentredInside(90), kParamGain, "Gain"));
    pGraphics->AttachControl(new IVKnobControl(controls.GetGridCell(1, 2, 6).GetCentredInside(90), kParamNoteGlideTime, "Glide"));
    const IRECT sliders = controls.GetGridCell(2, 2, 6).Union(controls.GetGridCell(3, 2, 6));
    pGraphics->AttachControl(new IVSliderControl(sliders.GetGridCell(0, 1, 4).GetMidHPadded(10.), kParamAttack));
    pGraphics->AttachControl(new IVSliderControl(sliders.GetGridCell(1, 1, 4).GetMidHPadded(10.), kParamDecay));
    pGraphics->AttachControl(new IVSliderControl(sliders.GetGridCell(2, 1, 4).GetMidHPadded(10.), kParamSustain));
    pGraphics->AttachControl(new IVSliderControl(sliders.GetGridCell(3, 1, 4).GetMidHPadded(10.), kParamRelease));
    pGraphics->AttachControl(new IVMeterControl<1>(controls.GetFromRight(100).GetPadded(-30), ""), kCtrlTagMeter);
    
    pGraphics->SetKeyHandlerFunc([&](const IKeyPress& key, bool isUp)
                                 {
                                   IMidiMsg msg;
                                   
                                   int note = 0;
                                   static int base = 60;
                                   static bool keysDown[128] = {};
                                   
                                   auto onOctSwitch = [&](){
                                     base = Clip(base, 24, 96);
                                     
                                     for(auto i=0;i<128;i++) {
                                       if(keysDown[i]) {
                                         msg.MakeNoteOffMsg(i, 0);
                                         SendMidiMsgFromUI(msg);
                                         dynamic_cast<IVKeyboardControl*>(GetUI()->GetControlWithTag(kCtrlTagKeyboard))->SetNoteFromMidi(i, false);
                                       }
                                     }
                                   };

                                   switch (key.VK) {
                                     case kVK_A: note = 0; break;
                                     case kVK_W: note = 1; break;
                                     case kVK_S: note = 2; break;
                                     case kVK_E: note = 3; break;
                                     case kVK_D: note = 4; break;
                                     case kVK_F: note = 5; break;
                                     case kVK_T: note = 6; break;
                                     case kVK_G: note = 7; break;
                                     case kVK_Y: note = 8; break;
                                     case kVK_H: note = 9; break;
                                     case kVK_U: note = 10; break;
                                     case kVK_J: note = 11; break;
                                     case kVK_K: note = 12; break;
                                     case kVK_O: note = 13; break;
                                     case kVK_L: note = 14; break;
                                     case kVK_Z: base -= 12; onOctSwitch(); return true;
                                     case kVK_X: base += 12; onOctSwitch(); return true;
                                     default: return true; // don't beep, but don't do anything
                                   }

                                   int pitch = base + note;

                                   if(!isUp)
                                   {
                                     if(keysDown[pitch] == false) {
                                       msg.MakeNoteOnMsg(pitch, 127, 0);
                                       keysDown[pitch] = true;
                                       SendMidiMsgFromUI(msg);
                                       dynamic_cast<IVKeyboardControl*>(GetUI()->GetControlWithTag(kCtrlTagKeyboard))->SetNoteFromMidi(pitch, true);
                                     }
                                   }
                                   else
                                   {
                                     if(keysDown[pitch] == true) {
                                       msg.MakeNoteOffMsg(pitch, 127, 0);
                                       keysDown[pitch] = false;
                                       SendMidiMsgFromUI(msg);
                                       dynamic_cast<IVKeyboardControl*>(GetUI()->GetControlWithTag(kCtrlTagKeyboard))->SetNoteFromMidi(pitch, false);
                                     }
                                   }
                                   
                                   return true;
                                 });
  };
#endif
}

#if IPLUG_DSP
void IPlugInstrument::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kParamGain)->Value() / 100.;
  const int nChans = NOutChansConnected();

  mDSP.ProcessBlock(inputs, outputs, 2, nFrames);
  
  for (auto s = 0; s < nFrames; s++) {
    for (auto c = 0; c < nChans; c++) {
      outputs[c][s] = outputs[c][s] * gain;
    }
  }

  mMeterBallistics.ProcessBlock(outputs, nFrames);
}

void IPlugInstrument::OnIdle()
{
  mMeterBallistics.TransmitData(*this);
}

void IPlugInstrument::OnReset()
{
  mDSP.Reset(GetSampleRate(), GetBlockSize());
}

void IPlugInstrument::ProcessMidiMsg(const IMidiMsg& msg)
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
  mDSP.ProcessMidiMsg(msg);
  SendMidiMsg(msg);
}

void IPlugInstrument::OnParamChange(int paramIdx)
{
  switch (paramIdx)
  {
    case kParamNoteGlideTime:
//      mDSP.mSynth.SetNoteGlideTime(GetParam(paramIdx)->Value());
      break;
      
    default:
      break;
  }
}
#endif
