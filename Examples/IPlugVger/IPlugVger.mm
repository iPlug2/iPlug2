#include "IPlugVger.h"
#include "IPlug_include_in_plug_src.h"

#ifdef FRAMEWORK_BUILD
#import <AUv3Framework/IPlugVger-Swift.h>
#import <AUv3Framework/IPlugVger-Shared.h>
#else
#import <IPlugVger-Swift.h>
#endif

const NSInteger kScopeBufferSize = SCOPE_BUFFER_SIZE;

IPlugVger::IPlugVger(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kParamGain)->InitDouble("Gain", 100., 0., 100.0, 0.01, "%");
  
  MakeDefaultPreset();
}

void IPlugVger::OnIdle()
{
  mScopeSender.TransmitData(*this);
}

void* IPlugVger::OpenWindow(void* pParent)
{
  PLATFORM_VIEW* platformParent = (PLATFORM_VIEW*) pParent;
  auto* vc = [[IPlugVgerViewController alloc] initWithEditorDelegateAndBundleID: this : GetBundleID()];
  mViewController = vc;
  vc.view.frame = MAKERECT(0.f, 0.f, (float) PLUG_WIDTH, (float) PLUG_HEIGHT);
  [platformParent addSubview:vc.view];
  
  OnUIOpen();
  
  return vc.view;
}

void IPlugVger::OnParentWindowResize(int width, int height)
{
  auto* vc = (IPlugVgerViewController*) mViewController;
  vc.view.frame = MAKERECT(0.f, 0.f, (float) width, (float) height);
}

// AUv3 only
bool IPlugVger::OnHostRequestingSupportedViewConfiguration(int width, int height)
{
#ifdef OS_MAC
  // Logic/GB offer one option with 0w, 0h, and if we allow that, our AUv3 has "our" size as its 100% setting
  return ((width + height) == 0);
#else
  return true;
#endif
}

void IPlugVger::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kParamGain)->Value() / 100.;
  const int nChans = NOutChansConnected();
    
  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[0][s] * gain;
    }
  }
  
  mScopeSender.ProcessBlock(outputs, nFrames, kCtrlTagScope);
}

bool IPlugVger::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  if (msgTag == kMsgTagHello)
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
