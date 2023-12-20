#include "IPlugChunks.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

IPlugChunks::IPlugChunks(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
, mStepPos(0)
{
  GetParam(kParamGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    IRECT b = pGraphics->GetBounds().GetPadded(-10.f);
    const IVStyle style = DEFAULT_STYLE.WithDrawShadows(false);
    pGraphics->AttachControl(new IVBakedPresetManagerControl(b.ReduceFromTop(30.f), "", style));
    pGraphics->AttachControl(new IVMultiSliderControl<kNumSteps>(b, "", style), kCtrlMultiSlider)->SetActionFunction([pGraphics](IControl* pCaller) {

      double vals[kNumSteps];

      for (int i = 0; i < kNumSteps; i++) {
        vals[i] = pCaller->GetValue(i);
      }
      pGraphics->GetDelegate()->SendArbitraryMsgFromUI(kMsgTagSliderChanged, kCtrlMultiSlider, sizeof(vals), &vals);
    });
  };
#endif
}

bool IPlugChunks::SerializeState(IByteChunk &chunk) const
{
  // serialize the multislider state state before serializing the regular params
  for (int i = 0; i< kNumSteps; i++)
  {
    chunk.Put(&mSteps[i]);
  }
  
  return SerializeParams(chunk); // must remember to call SerializeParams at the end
}

// this over-ridden method is called when the host is trying to load the plug-in state and you need to unpack the data into your algorithm
int IPlugChunks::UnserializeState(const IByteChunk &chunk, int startPos)
{
  double v = 0.;
  
  // unserialize the steps state before unserializing the regular params
  for (int i = 0; i< kNumSteps; i++)
  {
    startPos = chunk.Get(&v, startPos);
    mSteps[i] = v;
  }
  
  // If UI exists
  if(GetUI())
    UpdateUIControls();
  
  // must remember to call UnserializeParams at the end
  return UnserializeParams(chunk, startPos);
}

#if IPLUG_DSP
void IPlugChunks::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  int samplesPerBeat = (int) GetSamplesPerBeat();
  int samplePos = (int) GetSamplePos();
    
  for (int s = 0; s < nFrames; s++)
  {
    int mod = (samplePos + s) % (samplesPerBeat * kBeatDiv);
    
    mStepPos = mod / (samplesPerBeat / kBeatDiv);
    
    if (mStepPos >= kNumSteps)
    {
      mStepPos = 0;
    }
  
    for (int c = 0; c < 2; c++) {
      outputs[c][s] = inputs[c][s];
    }
  }
}
#endif

void IPlugChunks::OnUIOpen()
{
  UpdateUIControls();
}

void IPlugChunks::UpdateUIControls()
{
  auto* pMultiSlider = GetUI()->GetControlWithTag(kCtrlMultiSlider);
  
  for (int i = 0; i< kNumSteps; i++)
  {
    pMultiSlider->SetValue(mSteps[i], i);
  }
  
  GetUI()->SetAllControlsDirty();
}

bool IPlugChunks::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  if(msgTag == kMsgTagSliderChanged)
  {
    auto* pVals = reinterpret_cast<const double*>(pData);
    memcpy(mSteps, pVals, kNumSteps * sizeof(double));
    return true;
  }
  return false;
}

void IPlugChunks::OnIdle()
{
  if(mStepPos != mPrevPos)
  {
    mPrevPos = mStepPos;
    SendControlMsgFromDelegate(kCtrlMultiSlider, IVMultiSliderControl<>::kMsgTagSetHighlight, sizeof(int), &mPrevPos);
  }
}
