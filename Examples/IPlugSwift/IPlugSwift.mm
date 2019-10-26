#include "IPlugSwift.h"
#include "IPlug_include_in_plug_src.h"
#include "IPlugSwiftSharedConstants.h"

IPlugSwift::IPlugSwift(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPrograms))
{
  GetParam(kParamGain)->InitGain("Volume");
  
  MakePreset("One", 0.);
  MakePreset("Two", -10.);
  MakePreset("Three", -20.);
}

void IPlugSwift::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kParamGain)->DBToAmp();
  
  for (int s = 0; s < nFrames; s++)
  {
    outputs[0][s] = mOsc.Process(mFreqCPS) * mGainSmoother.Process(gain);
    outputs[1][s] = outputs[0][s]; // copy L to R
  }
}

void IPlugSwift::ProcessMidiMsg(const IMidiMsg& msg)
{
  auto midi2CPS = [](int pitch, double tune = 440.) {
    return tune * std::pow(2., (pitch - 69.) / 12.);
  };
  
  switch (msg.StatusMsg())
  {
    case IMidiMsg::kNoteOn:
      mFreqCPS = midi2CPS(msg.NoteNumber());
      break;
      
    default:
      break;
  }
}

bool IPlugSwift::OnMessage(int messageTag, int controlTag, int dataSize, const void* pData)
{
  if(messageTag == kMsgTagHello)
  {
    DBGMSG("MsgTagHello received\n");
    return true;
  }
  
  return false;
}

void IPlugSwift::OnParamChange(int paramIdx)
{
  DBGMSG("Param change %i: %f\n", paramIdx, GetParam(paramIdx)->Value());
}
