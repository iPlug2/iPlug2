#include "IPlugParametricEQ.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "IVParametricEQControl.h"

// Default frequencies for each band
static const double kDefaultFreqs[NUM_EQ_BANDS] = {80.0, 250.0, 1000.0, 4000.0, 12000.0};
// Default filter types for each band
static const int kDefaultTypes[NUM_EQ_BANDS] = {
  SVF<>::kLowPassShelf,  // Band 0 - Low shelf
  SVF<>::kBell,          // Band 1 - Bell
  SVF<>::kBell,          // Band 2 - Bell
  SVF<>::kBell,          // Band 3 - Bell
  SVF<>::kHighPassShelf  // Band 4 - High shelf
};

IPlugParametricEQ::IPlugParametricEQ(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  // Initialize parameters for all 5 bands
  for (int b = 0; b < NUM_EQ_BANDS; b++)
  {
    int baseIdx = b * 5;
    WDL_String name;

    // Frequency parameter (20Hz - 20kHz, log scale)
    name.SetFormatted(32, "Band %d Freq", b + 1);
    GetParam(baseIdx + kBandFreq)->InitFrequency(name.Get(), kDefaultFreqs[b], 20.0, 20000.0);

    // Gain parameter (-24dB to +24dB)
    name.SetFormatted(32, "Band %d Gain", b + 1);
    GetParam(baseIdx + kBandGain)->InitDouble(name.Get(), 0.0, -24.0, 24.0, 0.1, "dB");

    // Q parameter (0.1 to 20)
    name.SetFormatted(32, "Band %d Q", b + 1);
    GetParam(baseIdx + kBandQ)->InitDouble(name.Get(), 1.0, 0.1, 20.0, 0.01, "", IParam::kFlagsNone,
                                           "", IParam::ShapePowCurve(2.0));

    // Filter type parameter
    name.SetFormatted(32, "Band %d Type", b + 1);
    GetParam(baseIdx + kBandType)->InitEnum(name.Get(), kDefaultTypes[b], 8,
      "", IParam::kFlagsNone, "",
      "LowPass", "HighPass", "BandPass", "Notch", "Peak", "Bell", "LowShelf", "HighShelf");

    // Enable parameter
    name.SetFormatted(32, "Band %d Enable", b + 1);
    GetParam(baseIdx + kBandEnable)->InitBool(name.Get(), true);
  }

#if IPLUG_DSP
  // Initialize filters with default values
  for (int b = 0; b < NUM_EQ_BANDS; b++)
  {
    mFreq[b] = kDefaultFreqs[b];
    mGain[b] = 0.0;
    mQ[b] = 1.0;
    mType[b] = kDefaultTypes[b];
    mEnable[b] = true;

    mFilters[b].SetMode(static_cast<SVF<>::EMode>(mType[b]));
    mFilters[b].SetFreqCPS(mFreq[b]);
    mFilters[b].SetQ(mQ[b]);
    mFilters[b].SetGain(mGain[b]);
  }
#endif

#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };

  mLayoutFunc = [&](IGraphics* pGraphics) {
    const IRECT b = pGraphics->GetBounds();

    if (pGraphics->NControls())
    {
      pGraphics->GetBackgroundControl()->SetTargetAndDrawRECTs(b);
      pGraphics->GetControl(1)->SetTargetAndDrawRECTs(b.GetPadded(-10).GetReducedFromBottom(60));
      return;
    }

    pGraphics->AttachPanelBackground(COLOR_BLACK);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->EnableMouseOver(true);
    pGraphics->SetLayoutOnResize(true);

    // Style for the EQ control
    IVStyle style = DEFAULT_STYLE
      .WithColor(kBG, IColor(255, 20, 20, 25))
      .WithColor(kFG, IColor(255, 60, 60, 70))
      .WithLabelText(DEFAULT_LABEL_TEXT.WithFGColor(COLOR_WHITE))
      .WithValueText(DEFAULT_VALUE_TEXT.WithFGColor(COLOR_WHITE))
      .WithDrawFrame(true)
      .WithFrameThickness(1.f);

    // Main EQ analyzer area
    IRECT eqBounds = b.GetPadded(-10).GetReducedFromBottom(60);
    auto* pEQControl = new IVParametricEQControl<2, 4096, NUM_EQ_BANDS>(eqBounds, "Parametric EQ", style, kBand0Freq);
    pGraphics->AttachControl(pEQControl, kCtrlTagEQAnalyzer);

    // Bottom control area for band parameter knobs
    IRECT bottomArea = b.GetFromBottom(60).GetPadded(-10);
    float bandWidth = bottomArea.W() / NUM_EQ_BANDS;

    IVStyle knobStyle = DEFAULT_STYLE
      .WithColor(kFG, IColor(255, 100, 100, 100))
      .WithLabelText(IText(10, COLOR_WHITE))
      .WithValueText(IText(10, COLOR_WHITE))
      .WithShowLabel(false)
      .WithShowValue(true);

    // Band colors matching the control
    IColor bandColors[NUM_EQ_BANDS] = {
      IColor(255, 200, 100, 100),
      IColor(255, 200, 180, 100),
      IColor(255, 100, 200, 100),
      IColor(255, 100, 180, 200),
      IColor(255, 150, 100, 200)
    };

    for (int b = 0; b < NUM_EQ_BANDS; b++)
    {
      int baseIdx = b * 5;
      IRECT bandArea = bottomArea.GetGridCell(0, b, 1, NUM_EQ_BANDS).GetPadded(-2);

      // Band label
      WDL_String label;
      label.SetFormatted(16, "Band %d", b + 1);

      // Enable button
      IRECT enableRect = bandArea.GetFromLeft(20).GetCentredInside(18, 18);
      pGraphics->AttachControl(new IVToggleControl(enableRect, baseIdx + kBandEnable,
        "", knobStyle.WithColor(kFG, bandColors[b]), "", ""));

      // Gain knob (main control for each band)
      IRECT gainRect = bandArea.GetCentredInside(40, 40);
      pGraphics->AttachControl(new IVKnobControl(gainRect, baseIdx + kBandGain,
        "", knobStyle.WithColor(kFG, bandColors[b])));

      // Frequency label under knob
      IRECT freqLabelRect = bandArea.GetFromBottom(12);
      pGraphics->AttachControl(new ICaptionControl(freqLabelRect, baseIdx + kBandFreq,
        IText(9, COLOR_WHITE), IColor(), false));
    }
  };
#endif
}

#if IPLUG_DSP
void IPlugParametricEQ::OnReset()
{
  double sr = GetSampleRate();

  for (int b = 0; b < NUM_EQ_BANDS; b++)
  {
    mFilters[b].SetSampleRate(sr);
    mFilters[b].Reset();
  }

  mSender.Reset(sr);
  SyncUIControl();
}

void IPlugParametricEQ::OnParamChange(int paramIdx)
{
  // Determine which band and parameter changed
  if (paramIdx < kNumParams)
  {
    int band = paramIdx / 5;
    int param = paramIdx % 5;

    if (band < NUM_EQ_BANDS)
    {
      switch (param)
      {
        case kBandFreq:
          mFreq[band] = GetParam(paramIdx)->Value();
          mFilters[band].SetFreqCPS(mFreq[band]);
          break;
        case kBandGain:
          mGain[band] = GetParam(paramIdx)->Value();
          mFilters[band].SetGain(mGain[band]);
          break;
        case kBandQ:
          mQ[band] = GetParam(paramIdx)->Value();
          mFilters[band].SetQ(mQ[band]);
          break;
        case kBandType:
          mType[band] = static_cast<int>(GetParam(paramIdx)->Value());
          mFilters[band].SetMode(static_cast<SVF<>::EMode>(mType[band]));
          break;
        case kBandEnable:
          mEnable[band] = GetParam(paramIdx)->Value() > 0.5;
          break;
      }
    }
  }
}

bool IPlugParametricEQ::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  if (msgTag == EMsgTags::kMsgTagFFTSize)
  {
    int fftSize = *reinterpret_cast<const int*>(pData);
    mSender.SetFFTSize(fftSize);
    return true;
  }
  else if (msgTag == EMsgTags::kMsgTagOverlap)
  {
    int overlap = *reinterpret_cast<const int*>(pData);
    mSender.SetFFTSizeAndOverlap(mSender.GetFFTSize(), overlap);
    return true;
  }
  else if (msgTag == EMsgTags::kMsgTagWindowType)
  {
    int idx = *reinterpret_cast<const int*>(pData);
    mSender.SetWindowType(static_cast<ISpectrumSender<2>::EWindowType>(idx));
    return true;
  }

  return false;
}

void IPlugParametricEQ::OnIdle()
{
  mSender.TransmitData(*this);
}

void IPlugParametricEQ::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const int nChans = std::min(NInChansConnected(), NOutChansConnected());

  // Copy input to output first
  for (int c = 0; c < nChans; c++)
  {
    for (int s = 0; s < nFrames; s++)
    {
      outputs[c][s] = inputs[c][s];
    }
  }

  // Process through each enabled filter band
  for (int b = 0; b < NUM_EQ_BANDS; b++)
  {
    if (!mEnable[b])
      continue;

    // For Bell and Shelf filters, skip if gain is 0 (no effect)
    if ((mType[b] == SVF<>::kBell ||
         mType[b] == SVF<>::kLowPassShelf ||
         mType[b] == SVF<>::kHighPassShelf) && std::abs(mGain[b]) < 0.01)
      continue;

    mFilters[b].ProcessBlock(outputs, outputs, nChans, nFrames);
  }

  // Send processed output to spectrum analyzer
  mSender.ProcessBlock(outputs, nFrames, kCtrlTagEQAnalyzer, nChans);
}

void IPlugParametricEQ::UpdateFilter(int band)
{
  mFilters[band].SetMode(static_cast<SVF<>::EMode>(mType[band]));
  mFilters[band].SetFreqCPS(mFreq[band]);
  mFilters[band].SetQ(mQ[band]);
  mFilters[band].SetGain(mGain[band]);
}
#endif

#if IPLUG_EDITOR
void IPlugParametricEQ::OnParamChangeUI(int paramIdx, EParamSource source)
{
  // Notify the EQ control about parameter changes
  if (paramIdx < kNumParams)
  {
    // Send band params message to update the control
    SendControlMsgFromDelegate(kCtrlTagEQAnalyzer,
      IVParametricEQControl<>::kMsgTagBandParams, 0, nullptr);
  }
}

bool IPlugParametricEQ::SerializeState(IByteChunk& chunk) const
{
  int fftSize = mSender.GetFFTSize();
  int overlap = mSender.GetOverlap();
  int windowType = static_cast<int>(mSender.GetWindowType());
  chunk.Put(&fftSize);
  chunk.Put(&overlap);
  chunk.Put(&windowType);

  return SerializeParams(chunk);
}

int IPlugParametricEQ::UnserializeState(const IByteChunk& chunk, int startPos)
{
  int fftSize, overlap, windowType;

  startPos = chunk.Get(&fftSize, startPos);
  startPos = chunk.Get(&overlap, startPos);
  startPos = chunk.Get(&windowType, startPos);

  return UnserializeParams(chunk, startPos);
}

void IPlugParametricEQ::SyncUIControl()
{
  if (GetUI())
  {
    auto sr = GetSampleRate();
    auto fftSize = mSender.GetFFTSize();
    auto overlap = mSender.GetOverlap();
    auto windowType = static_cast<int>(mSender.GetWindowType());

    SendControlMsgFromDelegate(kCtrlTagEQAnalyzer,
      IVParametricEQControl<>::kMsgTagSampleRate, sizeof(double), &sr);
    SendControlMsgFromDelegate(kCtrlTagEQAnalyzer,
      IVParametricEQControl<>::kMsgTagFFTSize, sizeof(int), &fftSize);
    SendControlMsgFromDelegate(kCtrlTagEQAnalyzer,
      IVParametricEQControl<>::kMsgTagBandParams, 0, nullptr);
  }
}

void IPlugParametricEQ::OnUIOpen()
{
  SyncUIControl();
}
#endif
