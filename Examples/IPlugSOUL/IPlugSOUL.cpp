#include "IPlugSOUL.h"
#include "IPlug_include_in_plug_src.h"
//#include "IControls.h"

IPlugSOUL::IPlugSOUL(const InstanceInfo& info)
: Plugin(info, MakeConfig(DSP::numParameters, kNumPresets))
{
  mSOULParams = mDSP.createParameterList();
  
  int paramIdx = 0;
  for (auto& p : DSP::getParameterProperties()) {
    int flags = 0;
    flags |= !p.isAutomatable ? IParam::EFlags::kFlagCannotAutomate : 0;
    flags |= CStringHasContents(p.textValues) ? IParam::EFlags::kFlagStepped : 0;

    GetParam(paramIdx)->InitDouble(p.name, p.initialValue, p.minValue, p.maxValue, p.step, p.unit, flags, p.group);
    if(CStringHasContents(p.textValues)) {
      WDL_String vals {p.textValues};
      char* pChar = strtok(vals.Get(), "|");
      int tokIdx = 0;
      while (pChar != nullptr) {
        GetParam(paramIdx)->SetDisplayText(static_cast<double>(tokIdx++), pChar);
        pChar = strtok(nullptr, "|");
      }
    }
    
    paramIdx++;
  }
  
  //TODO: GUI
}

#if IPLUG_DSP
void IPlugSOUL::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  DSP::RenderContext<sample> renderCtx;

  assert(NInChansConnected() <= DSP::numAudioInputChannels);
  assert(NOutChansConnected() <= DSP::numAudioOutputChannels);
  
  int paramIdx;
  while (mParamsToUpdate.Pop(paramIdx)) {
    mSOULParams[paramIdx].setValue(GetParam(paramIdx)->Value());
  }
  
  if constexpr (DSP::numAudioInputChannels > 0) {
    for (auto i=0; i<DSP::numAudioInputChannels; i++) {
      renderCtx.inputChannels[i] = inputs[i];
    }
  }
  
  for (auto i=0; i<DSP::numAudioOutputChannels; i++) {
    renderCtx.outputChannels[i] = outputs[i];
  }
  
  renderCtx.numFrames = nFrames;
  
  mDSP.setTempo(GetTempo());
  mDSP.setPosition(mTimeInfo.mSamplePos, mTimeInfo.mPPQPos, mTimeInfo.mLastBar);
  mDSP.setTimeSignature(mTimeInfo.mNumerator, mTimeInfo.mDenominator);
  mDSP.setTransportState(mTimeInfo.mTransportIsRunning ? 1 : 0);
  
  mDSP.render(renderCtx);
}

void IPlugSOUL::OnReset()
{
  mDSP.init(GetSampleRate(), mSessionID++);
}

void IPlugSOUL::OnParamChange(int paramIdx)
{
  mParamsToUpdate.Push(paramIdx);
}
#endif
