#include "IPlugSwift.h"
#include "IPlug_include_in_plug_src.h"

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
    
    mCount %= kDataPacketSize;
    
    if(mCount == 0)
    {
      mCount = 0;
      mBufferFull = true;
    }

    mVizBuffer[mCount++] = outputs[0][s];
  }
}

void IPlugSwift::OnIdle()
{
  if(mBufferFull)
  {
    SendArbitraryMsgFromDelegate(kMsgTagData, kDataPacketSize * sizeof(float), mVizBuffer);
    mBufferFull = false;
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
  
  return CocoaEditorDelegate::OnMessage(messageTag, controlTag, dataSize, pData);
}

void IPlugSwift::OnParamChange(int paramIdx)
{
  DBGMSG("Param change %i: %f\n", paramIdx, GetParam(paramIdx)->Value());
}
