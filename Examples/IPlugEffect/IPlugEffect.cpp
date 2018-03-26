#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "config.h"

IPlugEffect::IPlugEffect(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
, mDSP(MAX_VOICES)
{
  GetParam(kPolyMode)->InitEnum("Polyphony", MidiSynth::kPolyModePoly, MidiSynth::kNumPolyModes, "", IParam::kFlagsNone, "", "Polyphonic", "Monophonic", "Legato");
  
  PrintDebugInfo();
}

void IPlugEffect::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  mDSP.ProcessBlock(inputs, outputs, 2, nFrames);
}

void IPlugEffect::OnReset()
{
  mDSP.Reset(GetSampleRate(), GetBlockSize());
}

void IPlugEffect::ProcessMidiMsg(const IMidiMsg& msg)
{
  TRACE;
  
  int status = msg.StatusMsg();
  
  switch (status)
  {
    case IMidiMsg::kNoteOn:
    case IMidiMsg::kNoteOff:
    case IMidiMsg::kPolyAftertouch:
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

void IPlugEffect::OnParamChange(int paramIdx, EParamSource)
{
  switch (paramIdx)
  {
    case kPolyMode:
      mDSP.mSynth.SetPolyMode((MidiSynth::EPolyMode) GetParam(kPolyMode)->Int());
      break;
  }
}
