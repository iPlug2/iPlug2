#include "IPlugSideChain.h"
#include "IPlug_include_in_plug_src.h"

IPlugSideChain::IPlugSideChain(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  SetChannelLabel(ERoute::kInput, 0, "Main L");
  SetChannelLabel(ERoute::kInput, 1, "Main R");
  SetChannelLabel(ERoute::kInput, 2, "SideChain L");
  SetChannelLabel(ERoute::kInput, 3, "SideChain R");

  GetParam(kGain)->InitGain("Gain");

  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, 1.);
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    IRECT b = pGraphics->GetBounds().GetPadded(-10.f);
    IRECT s = b.ReduceFromRight(50.f);
    
    const IVStyle meterStyle = DEFAULT_STYLE.WithColor(kFG, COLOR_WHITE.WithOpacity(0.3f));
    pGraphics->AttachControl(mInputMeter = new IVPeakAvgMeterControl<4>(b.FracRectVertical(0.5, true), "Inputs", meterStyle, EDirection::Horizontal, {"Main L", "Main R", "SideChain L", "SideChain R"}), kCtrlTagInputMeter);
    pGraphics->AttachControl(mOutputMeter = new IVPeakAvgMeterControl<2>(b.FracRectVertical(0.5, false), "Outputs", meterStyle, EDirection::Vertical, {"Main L", "Main R"}), kCtrlTagOutputMeter);
    pGraphics->AttachControl(new IVSliderControl(s, kGain));
  };

}

void IPlugSideChain::OnIdle()
{
  mInputPeakSender.TransmitData(*this);
  mOutputPeakSender.TransmitData(*this);
  
  if (mSendUpdate)
  {
    if(GetUI())
    {
      mInputMeter->SetTrackName(0, mInputChansConnected[0] ? "Main L (Connected)" : "Main L (Not connected)");
      mInputMeter->SetTrackName(1, mInputChansConnected[1] ? "Main R (Connected)" : "Main R (Not connected)");
      mInputMeter->SetTrackName(2, mInputChansConnected[2] ? "SideChain L (Connected)" : "SideChain L (Not connected)");
      mInputMeter->SetTrackName(3, mInputChansConnected[3] ? "SideChain R (Connected)" : "SideChain R (Not connected)");
      
      mOutputMeter->SetTrackName(0, mOutputChansConnected[0] ? "Main L (Connected)" : "Main L (Not connected)");
      mOutputMeter->SetTrackName(1, mOutputChansConnected[1] ? "Main R (Connected)" : "Main R (Not connected)");
      
      GetUI()->SetAllControlsDirty();
    }
    mSendUpdate = false;
  }
}

void IPlugSideChain::OnReset()
{
  mInputPeakSender.Reset(GetSampleRate());
  mOutputPeakSender.Reset(GetSampleRate());
}

void IPlugSideChain::GetBusName(ERoute direction, int busIdx, int nBuses, WDL_String& str) const
{
  if (direction == ERoute::kInput)
  {
    if (busIdx == 0)
      str.Set("Main Input");
    else
      str.Set("SideChain");
  }
  else
  {
    str.Set("Output");
  }
}

void IPlugSideChain::OnActivate(bool enable)
{
  mSendUpdate = true;
}

void IPlugSideChain::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->DBToAmp();
  const int nChans = NOutChansConnected();
  for (int i=0; i < 4; i++) {
    bool connected = IsChannelConnected(ERoute::kInput, i);
    if(connected != mInputChansConnected[i]) {
      mInputChansConnected[i] = connected;
      mSendUpdate = true;
    }
  }
  
  for (int i=0; i < 2; i++) {
    bool connected = IsChannelConnected(ERoute::kOutput, i);
    if(connected != mOutputChansConnected[i]) {
      mOutputChansConnected[i] = connected;
      mSendUpdate = true;
    }
  }
  
  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s] * gain;
    }
  }
  
  /*
    
     Logic/Garageband have an long-standing bug where if no sidechain is selected, the same buffers that are sent to the first bus, are sent to the sidechain bus
     https://forum.juce.com/t/sidechain-is-not-silent-as-expected-au-logic-x-10-2-2/17068/8
     https://lists.apple.com/archives/coreaudio-api/2012/Feb/msg00127.html
   
     Imperfect hack around it here. Probably a better solution is to have an enable sidechain button in the plug-in UI, in addition to the host sidechain routing.
  */
  
#if defined OS_MAC && defined AU_API
  if(GetHost() == kHostLogic || GetHost() == kHostGarageBand) {
    const int sz = nFrames * sizeof(sample);
    if(!memcmp(inputs[0], inputs[2], sz)) {
      memset(inputs[2], 0, sz);
      mInputChansConnected[2] = false;
    }
    if(!memcmp(inputs[1], inputs[3], sz)) {
      memset(inputs[3], 0, sz);
      mInputChansConnected[3] = false;
    }
  }
#endif

  mInputPeakSender.ProcessBlock(inputs, nFrames, kCtrlTagInputMeter, 4, 0);
  mOutputPeakSender.ProcessBlock(outputs, nFrames, kCtrlTagOutputMeter, 2, 0);
}
