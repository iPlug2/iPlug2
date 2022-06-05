/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
 */

#pragma once

/**
 * @file
 * @ingroup Controls
 * @copydoc IVSpectrumAnalyzerControl
 */

#include "IControl.h"
#include "ISender.h"
#include "IPlugStructs.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** Vectorial multi-channel capable spectrum analyzer control
 * @ingroup IControls
 * Derived from work by Alex Harker and Matthew Witmer
 */
template <int MAXNC = 2, int MAX_FFT_SIZE = 4096>
class IVSpectrumAnalyzerControl : public IControl
                                , public IVectorBase
{
public:
  enum MsgTags
  {
    kMsgTagSampleRate = 1,
    kMsgTagFFTSize,
    kMsgTagOverlap,
    kMsgTagWindowType,
    kMsgTagOctaveGain
  };
  
  static constexpr auto numExtraPoints = 2;
  using TDataPacket = std::array<float, MAX_FFT_SIZE>;
  enum class EChannelType { Left = 0, Right, LeftAndRight };
  enum class EFrequencyScale { Linear, Log };
  enum class EAmplitudeScale { Linear, Decibel };

  /** Create a IVSpectrumAnalyzerControl
   * @param bounds The rectangular area that the control occupies
   * @param label A CString to label the control
   * @param style /see IVStyle
   * @param colors A list of colors to use for the curves
   * @param freqScale The frequency scale to use
   * @param ampScale The amplitude scale to use
   * @param curveThickness The thickness of the curves
   * @param gridThickness The thickness of the grid lines
   * @param fillOpacity Fill in the spectrums 
   * @param attackTimeMs Attack smoothing in milliseconds
   * @param decayTimeMs Decay smoothing in milliseconds */
  IVSpectrumAnalyzerControl(const IRECT& bounds, const char* label = "", const IVStyle& style = DEFAULT_STYLE,
                            std::initializer_list<IColor> colors = {COLOR_RED, COLOR_GREEN},
                            EFrequencyScale freqScale = EFrequencyScale::Log,
                            EAmplitudeScale ampScale = EAmplitudeScale::Decibel,
                            float curveThickness = 2.0,
                            float gridThickness = 1.0,
                            float fillOpacity = 0.25,
                            float attackTimeMs = 3.0,
                            float decayTimeMs = 50.0)
  : IControl(bounds)
  , IVectorBase(style)
  , mChannelColors(colors)
  , mFreqScale(freqScale)
  , mAmpScale(ampScale)
  , mCurveThickness(curveThickness)
  , mGridThickness(gridThickness)
  , mFillOpacity(fillOpacity)
  , mAttackTimeMs(attackTimeMs)
  , mDecayTimeMs(decayTimeMs)
  {
    assert(colors.size() >= MAXNC);
    AttachIControl(this, label);
    SetFFTSize(1024);
    SetFreqRange(FirstBinFreq(), NyquistFreq());
    SetAmpRange(DBToAmp(-90.0f), 1.0f);
  }
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    mMenu.Clear(true);
    auto* pFftSizeMenu = mMenu.AddItem("FFT Size", new IPopupMenu("FFT Size", { "64", "128", "256", "512", "1024", "2048", "4096"}))->GetSubmenu();
    auto* pChansMenu = mMenu.AddItem("Channels", new IPopupMenu("Channels", { "L", "R", "L + R"}))->GetSubmenu();
    auto* pFreqScaleMenu = mMenu.AddItem("Freq Scaling", new IPopupMenu("Freq Scaling", { "Linear", "Log"}))->GetSubmenu();

    pFftSizeMenu->CheckItem(0, mFFTSize == 64);
    pFftSizeMenu->CheckItem(1, mFFTSize == 128);
    pFftSizeMenu->CheckItem(2, mFFTSize == 256);
    pFftSizeMenu->CheckItem(3, mFFTSize == 512);
    pFftSizeMenu->CheckItem(4, mFFTSize == 1024);
    pFftSizeMenu->CheckItem(5, mFFTSize == 2048);
    pFftSizeMenu->CheckItem(6, mFFTSize == 4096);
    
    pChansMenu->CheckItem(0, mChanType == EChannelType::Left);
    pChansMenu->CheckItem(1, mChanType == EChannelType::Right);
    pChansMenu->CheckItem(2, mChanType == EChannelType::LeftAndRight);
    pFreqScaleMenu->CheckItem(0, mFreqScale == EFrequencyScale::Linear);
    pFreqScaleMenu->CheckItem(1, mFreqScale == EFrequencyScale::Log);
    
    GetUI()->CreatePopupMenu(*this, mMenu, x, y);
  }

  void OnMouseOver(float x, float y, const IMouseMod& mod) override
  {
    mWidgetBounds.Constrain(x, y);
    mCursorAmp = CalcYNorm(1.0 - y/mWidgetBounds.H(), mAmpScale, true);
    mCursorFreq = CalcXNorm(x/mWidgetBounds.W(), mFreqScale, true) * NyquistFreq();
  }

  void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int valIdx) override
  {
    if (pSelectedMenu)
    {
      const char* title = pSelectedMenu->GetRootTitle();
      
      if (strcmp(title, "FFT Size") == 0)
      {
        int fftSize = atoi(pSelectedMenu->GetChosenItem()->GetText());
        GetDelegate()->SendArbitraryMsgFromUI(kMsgTagFFTSize, kNoTag, sizeof(int), &fftSize);
        SetFFTSize(fftSize);
      }
      else if (strcmp(title, "Channels") == 0)
      {
        const char* chanStr = pSelectedMenu->GetChosenItem()->GetText();
        if (strcmp(chanStr, "L") == 0) mChanType = EChannelType::Left;
        else if (strcmp(chanStr, "R") == 0) mChanType = EChannelType::Right;
        else if (strcmp(chanStr, "L + R") == 0) mChanType = EChannelType::LeftAndRight;
      }
      else if (strcmp(title, "Freq Scaling") == 0)
      {
        auto index = pSelectedMenu->GetChosenItemIdx();
        SetFrequencyScale(index == 0 ? EFrequencyScale::Linear : EFrequencyScale::Log);
      }
    }
  }

  void OnResize() override
  {
    SetTargetRECT(MakeRects(mRECT));
    SetDirty(false);
  }
  
  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    IByteStream stream(pData, dataSize);

    if (!IsDisabled() && msgTag == ISender<>::kUpdateMessage)
    {
      ISenderData<MAXNC, TDataPacket> d;
      stream.Get(&d, 0);
      
      for (auto c = d.chanOffset; c < (d.chanOffset + d.nChans); c++)
      {
        CalculateYPoints(c, d.vals[c]);
      }
    }
    else if (msgTag == kMsgTagSampleRate)
    {
      double sr;
      stream.Get(&sr, 0);
      SetSampleRate(sr);
    }
    else if (msgTag == kMsgTagFFTSize)
    {
      int fftSize;
      stream.Get(&fftSize, 0);
      SetFFTSize(fftSize);
    }
    else if (msgTag == kMsgTagOctaveGain)
    {
      double octaveGain;
      stream.Get(&octaveGain, 0);
      SetOctaveGain(octaveGain);
    }
  }
  
  void Draw(IGraphics& g) override
  {
    DrawBackground(g, mRECT);
    DrawWidget(g);
    DrawLabel(g);
    DrawCursorValues(g);
    
    if (mStyle.drawFrame)
      g.DrawRect(GetColor(kFR), mWidgetBounds, &mBlend, mStyle.frameThickness);
  }

private:
  void DrawGrids(IGraphics& g)
  {
    // Frequency Grid
    auto freq = mFreqLo;
    
    while (freq <= mFreqHi)
    {
      auto t = CalcXNorm(freq, mFreqScale);
      auto x0 = t * mWidgetBounds.W();
      auto y0 = mWidgetBounds.B;
      auto x1 = x0;
      auto y1 = mWidgetBounds.T;
      
      g.DrawLine(GetColor(kFG), x0, y0, x1, y1, 0, mGridThickness);
      
      if (freq < 10.0)
        freq += 1.0;
      else if (freq < 100.0)
        freq += 10.0;
      else if (freq < 1000.0)
        freq += 100.0;
      else if (freq < 10000.0)
        freq += 1000.0;
      else
        freq += 10000.0;
    }

    // Amplitude Grid
    if (mAmpScale == EAmplitudeScale::Decibel)
    {
      auto ampDB = AmpToDB(mAmpLo);
      const auto dBYHi = AmpToDB(mAmpHi);

      while (ampDB <= dBYHi)
      {
        auto t = Clip(CalcYNorm(ampDB, EAmplitudeScale::Decibel), 0.0f, 1.0f);
        
        auto x0 = mWidgetBounds.L;
        auto y0 = t * mWidgetBounds.H();
        auto x1 = mWidgetBounds.R;
        auto y1 = y0;
        
        g.DrawLine(GetColor(kFG), x0, y0, x1, y1, 0, mGridThickness);
        
        ampDB += 10.0;
      }
    }
  }
    
  void DrawWidget(IGraphics& g) override
  {
    DrawGrids(g);
    
    for (auto c = 0; c < MAXNC; c++)
    {
      if ((c == 0) && (mChanType == EChannelType::Right))
        continue;
      if ((c == 1) && (mChanType == EChannelType::Left))
        continue;
      
      int nPoints = NumPoints();
      IColor fillColor = mChannelColors[c].WithOpacity(mFillOpacity);
      g.DrawData(mChannelColors[c], mWidgetBounds, mYPoints[c].data(), nPoints, mXPoints.data(), 0, mCurveThickness, &fillColor);
    }
  }

  void DrawCursorValues(IGraphics& g)
  {
    WDL_String label;

    if (mCursorFreq >= 0.0)
    {
      label.SetFormatted(64, "%.1fHz", mCursorFreq);
      g.DrawText(mStyle.valueText, label.Get(), mWidgetBounds.GetFromTRHC(100, 50).FracRectVertical(0.5));
    }
    
    if (mAmpScale == EAmplitudeScale::Linear)
      label.SetFormatted(64, "%.3fs", mCursorAmp);
    else
      label.SetFormatted(64, "%ddB", (int) mCursorAmp);
    
    g.DrawText(mStyle.valueText, label.Get(), mWidgetBounds.GetFromTRHC(100, 50).FracRectVertical(0.5, true));
  }
  
#pragma mark -
  
  void SetFFTSize(int fftSize)
  {
    assert(fftSize > 0);
    assert(fftSize <= MAX_FFT_SIZE);
    mFFTSize = fftSize;
    
    ResizePoints();
    CalculateXPoints();
    SetFreqRange(FirstBinFreq(), NyquistFreq());
    SetSmoothing(mAttackTimeMs, mDecayTimeMs);
    SetDirty(false);
  }

  void SetSampleRate(double sampleRate)
  {
    mSampleRate = sampleRate;
    SetFreqRange(FirstBinFreq(), NyquistFreq());
    SetSmoothing(mAttackTimeMs, mDecayTimeMs);
    SetDirty(false);
  }
  
  void SetFreqRange(float freqLo, float freqHi)
  {
    mFreqLo = freqLo;
    mFreqHi = freqHi;
    SetDirty(false);
  }
  
  void SetFrequencyScale(EFrequencyScale scale)
  {
    mFreqScale = scale;
    CalculateXPoints();
    SetDirty(false);
  }
  
  void SetAmpRange(float ampLo, float ampHi)
  {
    mAmpLo = ampLo;
    mAmpHi = ampHi;
    SetDirty(false);
  }

  void SetOctaveGain(float octaveGain)
  {
    mOctaveGain = octaveGain;
    SetDirty(false);
  }
  
  void SetSmoothing(float attackTimeMs, float releaseTimeMs)
  {
    auto attackTimeSec = attackTimeMs * 0.001f;
    auto releaseTimeSec = releaseTimeMs * 0.001f;
    auto updatePeriod = (float) mFFTSize / (float) mSampleRate;
    mAttackCoeff = exp(-updatePeriod / attackTimeSec);
    mReleaseCoeff = exp(-updatePeriod / releaseTimeSec);
  }
  
protected:
  float ApplyOctaveGain(float amp, float freqNorm)
  {
    // Center on 500Hz
    const float centerFreq = 500.0f;
    float centerFreqNorm = (centerFreq - mFreqLo)/(mFreqHi - mFreqLo);

    if (mOctaveGain > 0.0)
    {
      amp *= freqNorm/centerFreqNorm;
    }

    return amp;
  }
  
  void ResizePoints()
  {
    mXPoints.resize(NumPoints());
    
    for (auto c = 0; c < MAXNC; c++)
    {
      mYPoints[c].assign(NumPoints(), 0.0f);
      mEnvelopeValues[c].assign(NumPoints(), 0.0f);
    }
  }
  
  void CalculateXPoints()
  {
    const auto numBins = NumBins();
    const auto xIncr = (1.0f / static_cast<float>(numBins-1)) * NyquistFreq();
    mXPoints[0] = 0.0f;
    for (auto i = 1; i < numBins; i++)
    {
      auto xVal = CalcXNorm(float(i) * xIncr, mFreqScale);
      mXPoints[i] = xVal;
    }
    mXPoints[numBins] = mXPoints[numBins-1];
    mXPoints[numBins+1] = mXPoints[0];
  }
  
  void CalculateYPoints(int ch, const TDataPacket& powerSpectrum)
  {
    const auto numBins = NumBins();

    for (auto i = 0; i < numBins; i++)
    {
      const auto adjustedAmp = ApplyOctaveGain(powerSpectrum[i], static_cast<float>(numBins-1));
      float rawVal = (mAmpScale == EAmplitudeScale::Decibel)
                       ? AmpToDB(adjustedAmp + 1e-30f)
                       : adjustedAmp;
      rawVal = Clip(CalcYNorm(rawVal, mAmpScale), 0.0f, 1.0f);
      
      float prevVal = mEnvelopeValues[ch][i];
      float newVal;
      if (rawVal > prevVal)
        newVal = mAttackCoeff * prevVal + (1.0f - mAttackCoeff) * rawVal;  // Attack phase
      else
        newVal = mReleaseCoeff * prevVal + (1.0f - mReleaseCoeff) * rawVal; // Release phase

      mEnvelopeValues[ch][i] = newVal; // Store smoothed value
      mYPoints[ch][i] = newVal; // Use smoothed value for drawing
    }
    
    if (FillCurves())
    {
      // Used to close the path outside the bounds of the control
      auto offset = mCurveThickness/mWidgetBounds.H();

      mYPoints[ch][numBins] = -offset;
      mYPoints[ch][numBins+1] = -offset;
    }
    
    SetDirty(false);
  }

  float CalcXNorm(float x, EFrequencyScale scale, bool inverted = false)
  {
    const auto nyquist = NyquistFreq();
    
    switch (scale)
    {
      case EFrequencyScale::Linear:
      {
        if (!inverted)
          return (x - mFreqLo) / (mFreqHi - mFreqLo);
        else
          return (mFreqLo + x * (mFreqHi - mFreqLo)) / nyquist;
      }
      case EFrequencyScale::Log:
      {
        const auto logXLo = std::log(mFreqLo / nyquist);
        const auto logXHi = std::log(mFreqHi / nyquist);
        
        if (!inverted)
          return (std::log(x / nyquist) - logXLo) / (logXHi - logXLo);
        else
          return std::exp(logXLo + x * (logXHi - logXLo));
      }
    }
  }

  // Amplitudes
  float CalcYNorm(float y, EAmplitudeScale scale, bool inverted = false) const
  {
    switch (scale)
    {
      case EAmplitudeScale::Linear:
      {
        if (!inverted)
          return (y - mAmpLo) / (mAmpHi - mAmpLo);
        else
          return mAmpLo + y * (mAmpHi - mAmpLo);
      }
      case EAmplitudeScale::Decibel:
      {
        const auto dBYLo = AmpToDB(mAmpLo);
        const auto dBYHi = AmpToDB(mAmpHi);
        
        if (!inverted)
          return (y - dBYLo) / (dBYHi - dBYLo);
        else
          return dBYLo + y * (dBYHi - dBYLo);
      }
    }
  }

  int NumPoints() const { return FillCurves() ? NumBins() + numExtraPoints : NumBins(); }
  int NumBins() const { return mFFTSize / 2; }
  double FirstBinFreq() const { return NyquistFreq()/mFFTSize; }
  double NyquistFreq() const { return mSampleRate * 0.5; }
  bool FillCurves() const { return mFillOpacity > 0.0f; }
  
private:
  std::vector<IColor> mChannelColors;
  EFrequencyScale mFreqScale;
  EAmplitudeScale mAmpScale;

  double mSampleRate = 44100.0;
  int mFFTSize = 1024;
  float mOctaveGain = 0.0;
  float mFreqLo = 20.0;
  float mFreqHi = 22050.0;
  float mAmpLo = 0.0;
  float mAmpHi = 1.0;
  float mAttackTimeMs = 3.0;
  float mDecayTimeMs = 50.0;
  EChannelType mChanType = EChannelType::LeftAndRight;

  float mCurveThickness = 1.0f;
  float mGridThickness = 1.0f;
  float mFillOpacity = 0.5f;
  float mCursorAmp = 0.0;
  float mCursorFreq = -1.0;
  IPopupMenu mMenu {"Options"};

  std::vector<float> mXPoints;
  std::array<std::vector<float>, MAXNC> mYPoints;
  std::array<std::vector<float>, MAXNC> mEnvelopeValues;
  float mAttackCoeff = 0.2f;
  float mReleaseCoeff = 0.99f;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
