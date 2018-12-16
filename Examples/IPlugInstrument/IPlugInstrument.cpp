#include "IPlugInstrument.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

IPlugInstrument::IPlugInstrument(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{
  GetParam(kGain)->InitDouble("Gain", 100., 0., 100.0, 0.01, "%");
  GetParam(kNoteGlideTime)->InitSeconds("Note Glide Time", 0.1, 2.);

#if IPLUG_EDITOR // All UI methods and member variables should be within an IPLUG_EDITOR guard, should you want distributed UI
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, 1.);
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(kUIResizerScale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
//    pGraphics->EnableLiveEdit(true);
    pGraphics->LoadFont(ROBOTTO_FN);
    const IRECT b = pGraphics->GetBounds();
    pGraphics->AttachControl(new IVKeyboardControl(*this, IRECT(10, 335, PLUG_WIDTH-10, PLUG_HEIGHT-10)));
    pGraphics->AttachControl(new IVMultiSliderControl<8>(*this, b.GetGridCell(0, 2, 2).GetPadded(-30)));
    const IRECT knobs = b.GetGridCell(1, 2, 2);
    pGraphics->AttachControl(new IVKnobControl(*this, knobs.GetGridCell(0, 1, 3).GetCentredInside(100), kGain, "Gain", true));
    pGraphics->AttachControl(new IVKnobControl(*this, knobs.GetGridCell(1, 1, 3).GetCentredInside(100), kNoteGlideTime, "Glide"));
  };
#endif
}

#if IPLUG_DSP
void IPlugInstrument::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  const int nChans = NOutChansConnected();

  mDSP.ProcessBlock(inputs, outputs, 2, nFrames);
  
  for (auto s = 0; s < nFrames; s++) {
    for (auto c = 0; c < nChans; c++) {
      outputs[c][s] = outputs[c][s] * gain;
    }
  }
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
    case kNoteGlideTime:
      mDSP.mSynth.SetNoteGlideTime(GetParam(paramIdx)->Value());
      break;
      
    default:
      break;
  }
}
#endif
