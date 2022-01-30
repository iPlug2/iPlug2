#include "IPlugCocoaUI.h"
#include "IPlug_include_in_plug_src.h"

#ifdef FRAMEWORK_BUILD
#import <AUv3Framework/IPlugCocoaUI-Swift.h>
#import <AUv3Framework/IPlugCocoaUI-Shared.h>
#else
#import <IPlugCocoaUI-Swift.h>
#endif

IPlugCocoaUI::IPlugCocoaUI(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kParamGain)->InitGain("Volume", -70.0);

  MakePreset("Gain = -70dB", -70.);
  MakePreset("Gain = -10dB", -10.);
  MakePreset("Gain = 0dB", 0.);
  
#ifdef OS_MAC
  NSStoryboard* pStoryBoard = [NSStoryboard storyboardWithName:@"IPlugCocoaUI-macOS-MainInterface"
                                                        bundle: [NSBundle bundleWithIdentifier:
                                                                 [NSString stringWithUTF8String:
                                                                  GetBundleID()]]];
  auto* vc = (IPlugCocoaUIViewController*) [pStoryBoard instantiateControllerWithIdentifier:@"main"];
  
  //  IPlugCocoaUIViewController* vc = [[IPlugCocoaUIViewController alloc] init];
  
  [vc retain];
  [vc setEditorDelegate: this];
  mViewController = vc;
  vc.view.frame = MAKERECT(0.f, 0.f, (float) PLUG_WIDTH, (float) PLUG_HEIGHT);
#endif
}

void* IPlugCocoaUI::OpenWindow(void* pParent)
{
  PLATFORM_VIEW* platformParent = (PLATFORM_VIEW*) pParent;

#ifdef FRAMEWORK_BUILD
  auto* vc = [[(PLATFORM_VC*) [platformParent nextResponder] childViewControllers] objectAtIndex:0];
  [vc setEditorDelegate: this];
  mViewController = vc;
#else
  IPlugCocoaUIViewController* vc = (IPlugCocoaUIViewController*) mViewController;
  [platformParent addSubview:vc.view];
#endif
  OnUIOpen();

  return vc.view;
}

IPlugCocoaUI::~IPlugCocoaUI()
{
#ifdef OS_MAC
  auto* vc = (IPlugCocoaUIViewController*) mViewController;
  [vc release];
#endif
}

void IPlugCocoaUI::OnParentWindowResize(int width, int height)
{
  auto* vc = (IPlugCocoaUIViewController*) mViewController;
  vc.view.frame = MAKERECT(0.f, 0.f, (float) width, (float) height);
}

// AUv3 only
bool IPlugCocoaUI::OnHostRequestingSupportedViewConfiguration(int width, int height)
{
#ifdef OS_MAC
  // Logic/GB offer one option with 0w, 0h, and if we allow that, our AUv3 has "our" size as its 100% setting
  return ((width + height) == 0);
#else
  return true;
#endif
}

void IPlugCocoaUI::OnIdle()
{
  mSender.TransmitData(*this);
}

bool IPlugCocoaUI::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
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

void IPlugCocoaUI::OnParamChange(int paramIdx)
{
  DBGMSG("Param change %i: %f\n", paramIdx, GetParam(paramIdx)->Value());
}

void IPlugCocoaUI::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kParamGain)->DBToAmp();
  const int nChans = NOutChansConnected();
    
  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s] * gain;
    }
  }
  
  mSender.ProcessBlock(inputs, nFrames, kCtrlTagVUMeter);
}


