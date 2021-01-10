#include "IPlugSOUL.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

IPlugSOUL::IPlugSOUL(const InstanceInfo& info)
: Plugin(info, MakeConfig(DSP::numParameters, kNumPresets))
{
  int paramIdx = 0;
  for (auto& p : DSP::getParameterProperties()) {
    mParamMap.insert(std::make_pair(p.name, paramIdx));

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
  
  #if IPLUG_DSP
    mSOULParams = mDSP.createParameterList();
  #endif
  
  #if IPLUG_EDITOR
    mMakeGraphicsFunc = [&]() {
      return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
    };
    
    mLayoutFunc = [&](IGraphics* pGraphics) {
      pGraphics->AttachPanelBackground(COLOR_ORANGE);
      pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
      const IRECT b = pGraphics->GetBounds();
      pGraphics->AttachControl(new IVKnobControl(b.GetCentredInside(100).GetVShifted(-50), GetIPlugParamIdx("volume")));
      pGraphics->AttachControl(new IVKeyboardControl(b.GetFromBottom(50)));
    };
  #endif // IPLUG_EDITOR
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
    memset(outputs[i], 0, nFrames * sizeof(sample));
    renderCtx.outputChannels[i] = outputs[i];
  }
  
  renderCtx.numFrames = nFrames;
  
  mDSP.setTempo(GetTempo());
  mDSP.setPosition(mTimeInfo.mSamplePos, mTimeInfo.mPPQPos, mTimeInfo.mLastBar);
  mDSP.setTimeSignature(mTimeInfo.mNumerator, mTimeInfo.mDenominator);
  mDSP.setTransportState(mTimeInfo.mTransportIsRunning ? 1 : 0);
  
  if(mIncomingMIDIMessages.size()) {
    renderCtx.incomingMIDI.messages = std::addressof (mIncomingMIDIMessages[0]);
    renderCtx.incomingMIDI.numMessages = static_cast<uint32_t>(mIncomingMIDIMessages.size());
  }
  
  mDSP.render(renderCtx);
  mIncomingMIDIMessages.clear();
}

void IPlugSOUL::ProcessMidiMsg(const IMidiMsg& msg)
{
  mIncomingMIDIMessages.push_back({static_cast<uint32_t>(msg.mOffset), msg.mStatus, msg.mData1, msg.mData2});
}

void IPlugSOUL::OnReset()
{
  mDSP.init(GetSampleRate(), mSessionID++);
  mIncomingMIDIMessages.reserve(GetBlockSize());
}

void IPlugSOUL::OnParamChange(int paramIdx)
{
  mParamsToUpdate.Push(paramIdx);
}
#endif // IPLUG_DSP
