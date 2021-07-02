#include "IPlugUIKit.h"
#include "IPlug_include_in_plug_src.h"

#ifdef FRAMEWORK_BUILD
#import <AUv3Framework/IPlugUIKit-Swift.h>
#else
#import <IPlugUIKit-Swift.h>
#endif

IPlugUIKit::IPlugUIKit(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kParamGain)->InitGain("Volume");

  MakePreset("Gain = 0dB", 0.);
  MakePreset("Gain = -10dB", -10.);
  MakePreset("Gain = -20dB", -20.);
}

void IPlugUIKit::OnIdle()
{
  mSender.TransmitData(*this);
}

void* IPlugUIKit::OpenWindow(void* pParent)
{
  PLATFORM_VIEW* platformParent = (PLATFORM_VIEW*) pParent;
  IPlugUIKitViewController* vc = [[(IPlugAUViewController*) [platformParent nextResponder] childViewControllers] objectAtIndex:0];
  [vc setEditorDelegate: this];
  mViewController = vc;
  return vc.view;
}

void IPlugUIKit::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kParamGain)->Value() / 100.;
  const int nChans = NOutChansConnected();
    
  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s] * gain;
    }
  }
  
//  mSender.PushData({kCtrlTagVUMeter, {(float) outputs[0][nFrames-1]}});
}

bool IPlugUIKit::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  if(msgTag == kMsgTagHello)
  {
    DBGMSG("MsgTagHello received on C++ side\n");
    return true;
  }
  else if(msgTag == kMsgTagRestorePreset)
  {
    RestorePreset(ctrlTag);
  }
  
  return CocoaEditorDelegate::OnMessage(msgTag, ctrlTag, dataSize, pData);
}

void IPlugUIKit::OnParamChange(int paramIdx)
{
  DBGMSG("Param change %i: %f\n", paramIdx, GetParam(paramIdx)->Value());
}
