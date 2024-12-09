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
#include "Smoothers.h"
//#include "IFilterBank.h"
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
  /** Channel configuration types */
  enum class EChannelType { Left = 0, Right, LeftAndRight };

  /** Constructor
   * @param bounds The rectangular area that the control occupies
   * @param label A CString to label the control
   * @param style, /see IVStyle
   * @param colors A list of colors to use for the curves
   * @param freqScale The frequency scale to use
   * @param ampScale The amplitude scale to use
   * @param curveThickness The thickness of the curves
   * @param gridThickness The thickness of the grid lines
   * @param fillCurves Whether to fill the curves
  */
  IVSpectrumAnalyzerControl(const IRECT& bounds, const char* label = "", const IVStyle& style = DEFAULT_STYLE,
                            std::initializer_list<IColor> colors = {COLOR_RED, COLOR_BLUE, COLOR_GREEN},
                            EFrequencyScale freqScale = EFrequencyScale::Log,
                            EAmplitudeScale ampScale = EAmplitudeScale::Decibel,
                            float curveThickness = 2.0,
                            float gridThickness = 1.0,
                            bool fillCurves = false)
  : IControl(bounds)
  , IVectorBase(style)
  , mChannelColors(colors)
  , mFreqScale(freqScale)
  , mAmpScale(ampScale)
  , mCurveThickness(curveThickness)
  , mGridThickness(gridThickness)
  , mFillCurves(fillCurves)
  {
    assert(colors.size() >= MAXNC);
    AttachIControl(this, label);
    mXPoints.resize(MAX_FFT_SIZE/2 + numExtraPoints);
    
    for (auto c = 0; c < MAXNC; c++)
    {
      mYPoints[c].resize(MAX_FFT_SIZE/2 + numExtraPoints);
    }
    
    SetFFTSize(1024);
    SetAxes();
    SetFreqRange(NyquistFreq()/mFFTSize, NyquistFreq(), mSampleRate);
    SetAmpRange(DBToAmp(-90.0f), 1.0f);
    SetSmoothTime(mSmoothTime);
  }
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    mMenu.Clear(true);
    mMenu.AddItem("FFTSize", new IPopupMenu("FFTSize", { "64", "128", "256", "512", "1024", "2048", "4096"}))
      ->GetSubmenu()->CheckItemWithText(std::to_string(mFFTSize).c_str());
    mMenu.AddItem("Channels", new IPopupMenu("Channels", { "L", "R", "L + R"}));
    auto* pSpeedMenu = mMenu.AddItem("Speed", new IPopupMenu("Speed", { "100%", "75%", "50%", "25%"}))->GetSubmenu();
    
    if      (mSmoothTime == 0.0) pSpeedMenu->CheckItemAlone(0);
    else if (mSmoothTime == 1.0) pSpeedMenu->CheckItemAlone(1);
    else if (mSmoothTime == 2.0) pSpeedMenu->CheckItemAlone(2);
    else if (mSmoothTime == 5.0) pSpeedMenu->CheckItemAlone(3);
    
    GetUI()->CreatePopupMenu(*this, mMenu, x, y);
  }

  void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int valIdx) override
  {
    if (pSelectedMenu)
    {
      const char* title = pSelectedMenu->GetRootTitle();
      
      if (strcmp(title, "FFTSize") == 0)
      {
        int fftSize = atoi(pSelectedMenu->GetChosenItem()->GetText());
        GetDelegate()->SendArbitraryMsgFromUI(kMsgTagFFTSize, kNoTag, sizeof(int), &fftSize);
        SetFFTSize(fftSize);
      }
      else if (strcmp(title, "Channels") == 0)
      {
        const char* chanStr = pSelectedMenu->GetChosenItem()->GetText();
        if (strcmp(chanStr, "L") == 0)
          mChanType = EChannelType::Left;
        else if (strcmp(chanStr, "R") == 0)
          mChanType = EChannelType::Right;
        else if (strcmp(chanStr, "L + R") == 0)
          mChanType = EChannelType::LeftAndRight;
      }
      else if (strcmp(title, "Speed") == 0)
      {
        const char* speedStr = pSelectedMenu->GetChosenItem()->GetText();
        
        if (strcmp(speedStr, "100%") == 0)
          SetSmoothTime(0.0);
        else if (strcmp(speedStr, "75%") == 0)
          SetSmoothTime(1.0);
        else if (strcmp(speedStr, "50%") == 0)
          SetSmoothTime(2.0);
        else if (strcmp(speedStr, "25%") == 0)
          SetSmoothTime(5.0);
      }
    }
  }

  void OnMouseOver(float x, float y, const IMouseMod& mod) override
  {
    mWidgetBounds.Constrain(x, y);
    mCursorAmp = CalcYNorm(1.0 - y/mWidgetBounds.H(), mAmpScale, true);
    mCursorFreq = CalcXNorm(x/mWidgetBounds.W(), mFreqScale, mSampleRate, true) * NyquistFreq();
  }
  
  
  void SetText(const IText& txt) override
  {
    IControl::SetText(txt);
//    mFreqAxis->SetText(txt);
//    mAmpAxis->SetText(txt);
  }
  
  void Draw(IGraphics& g) override
  { 
    DrawBackground(g, mRECT);
    DrawWidget(g);
//    DrawAxes(g);
    DrawLabel(g);
    DrawPointerValues(g);
    
    if (mStyle.drawFrame)
      g.DrawRect(GetColor(kFR), mWidgetBounds, &mBlend, mStyle.frameThickness);
  }

  void DrawGrids(IGraphics& g)
  {
    // Draw Frequency Grid
    auto freq = mFreqLo;
    
    while (freq <= mFreqHi)
    {
      auto t = CalcXNorm(freq, mFreqScale, mSampleRate);
      // Horizontal freq axis
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

    // Draw Amp Grid
    if (mAmpScale == EAmplitudeScale::Decibel)
    {
      auto ampDB = AmpToDB(mAmpLo);
      const auto dBYHi = AmpToDB(mAmpHi);

      while (ampDB <= dBYHi)
      {
        auto t = Clip(CalcYNorm(ampDB, EAmplitudeScale::Decibel), 0.0f, 1.0f);
        
        // Vertical amp axis
        auto x0 = mWidgetBounds.L;
        auto y0 = t * mWidgetBounds.H();
        auto x1 = mWidgetBounds.R;
        auto y1 = y0;
        
        // Draw one line
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
      auto* pFillColor = mFillCurves ? &fillColor : nullptr;
      g.DrawData(mChannelColors[c], mWidgetBounds, mYPoints[c].data(), nPoints, mXPoints.data(), 0, mCurveThickness, pFillColor);
    }
  }

//  void DrawAxes(IGraphics& g)
//  {
//    mFreqAxis->Draw(g, mWidgetBounds);
//    mAmpAxis->Draw(g, mWidgetBounds);
//  }

  void DrawPointerValues(IGraphics& g)
  {
    // Draw frequency value under pointer
    if (mCursorFreq >= 0.0)
    {
      WDL_String freqLabel;
      freqLabel.SetFormatted(64, "%.1fHz", mCursorFreq);
      g.DrawText(mStyle.valueText, freqLabel.Get(), mWidgetBounds.GetFromTRHC(100, 50).FracRectVertical(0.5));
    }
    
    // Draw amp value under pointer
    WDL_String ampLabel;
    if (mAmpScale == EAmplitudeScale::Linear)
      ampLabel.SetFormatted(64, "%.3fs", mCursorAmp);
    else
      ampLabel.SetFormatted(64, "%ddB", (int) mCursorAmp);
    g.DrawText(mStyle.valueText, ampLabel.Get(), mWidgetBounds.GetFromTRHC(100, 50).FracRectVertical(0.5, true));
  }
  
  void OnResize() override
  {
    SetTargetRECT(MakeRects(mRECT));
    SetAxes();
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
  
#pragma mark -
  
  void SetFFTSize(int fftSize)
  {
    assert(fftSize > 0);
    assert(fftSize <= MAX_FFT_SIZE);
    mFFTSize = fftSize;
    
    // Resize points vectors
    const auto numBins = NumBins();
    mXPoints.resize(numBins + numExtraPoints);
    
    for (auto c = 0; c < MAXNC; c++)
    {
      mYPoints[c].resize(numBins + numExtraPoints);
    }
    
    // Calculate x coordinates
    float xIncr = 1.0f / static_cast<float>(numBins-1);
    for (auto i = 0; i < numBins; i++)
    {
      mXPoints[i] = float(i) * xIncr;
    }
    mXPoints[numBins] = mXPoints[numBins-1];
    mXPoints[numBins+1] = mXPoints[0];
    
    SetFreqRange(NyquistFreq()/mFFTSize, NyquistFreq(), mSampleRate);
    SetSmoothTime(mSmoothTime);
    SetDirty(false);
  }

  void SetSampleRate(double sampleRate)
  {
    mSampleRate = sampleRate;

//    mFreqAxis->SetRangeHi(NyquistFreq());
    SetFreqRange(NyquistFreq()/mFFTSize, NyquistFreq(), mSampleRate);

    for (auto i = 0; i < MAXNC; i++)
      mDataSmooth[i].SetSampleRate(sampleRate);

    SetDirty(false);
  }
  
  void SetFreqRange(float freqLo, float freqHi, float sampleRate)
  {
    mFreqLo = freqLo;
    mFreqHi = freqHi;
//    mFreqAxis->SetRange(freqLo, freqHi);
    SetDirty(false);
  }
  
  void SetAmpRange(float ampLo, float ampHi)
  {
    mAmpLo = ampLo;
    mAmpHi = ampHi;
    SetDirty(false);
  }

  void SetSmoothTime(float timeMs)
  {
    mSmoothTime = timeMs;
    
    float fftSizeCoeff = 1024.0/mFFTSize;
    timeMs *= fftSizeCoeff;
    
    for (auto i = 0; i < MAXNC; i++)
      mDataSmooth[i].SetSmoothTime(timeMs);
  }

  void SetOctaveGain(float octaveGain)
  {
    mOctaveGain = octaveGain;
    SetDirty(false);
  }
  
  void SetAxes()
  {
    // const std::vector<float> freqs {100.0, 500.0, 1000.0, 5000.0, 10000.0, 20000.0};
    // mFreqAxis = std::make_unique<IFrequencyAxis>(mRECT, mText, freqs, 20.0, 20000.0, mFreqScale, EDirection::Horizontal);
    // const std::vector<float> amps {-100.0, -80.0, -60.0, -40.0, -20.0, 0.0};
    // mAmpAxis = std::make_unique<IAmpAxis>(mRECT, mText, amps, -90.0, 0.0, mAmpScale, EDirection::Vertical);
  }
  
protected:

  float ApplyOctaveGain(float amp, float freqNorm)
  {
    // Center on 500Hz
    const float centerFreq = 500.0;
    float centerFreqNorm = (centerFreq - mFreqLo)/(mFreqHi - mFreqLo);

    if (mOctaveGain > 0.0)
    {
      amp *= freqNorm/centerFreqNorm;
    }

    return amp;
  }
    
  void CalculateYPoints(int ch, const TDataPacket& powerSpectrum)
  {
    const auto numBins = NumBins();

    // Scale y coordinates
    if (mTmpBuf.size() != numBins)
      mTmpBuf.resize(numBins);
    
    // First scale y coordinates depending on the range
    // and depending if the scale is linear or dB

    // NOTE: it is important to scale amplitudes first,
    // and only then scale along frequencies 
    for (auto i = 0; i < numBins; i++)
    {
      float amp = ApplyOctaveGain(powerSpectrum[i], float(i)/(numBins-1));
      
      if (mAmpScale == EAmplitudeScale::Linear)
        amp = CalcYNorm(amp, EAmplitudeScale::Linear);
      else
        amp = Clip(CalcYNorm(AmpToDB(amp + 1e-30), EAmplitudeScale::Decibel), 0.0f, 1.0f);

      mTmpBuf[i] = amp;
    }
    
    // mFilterBank.Process(mTmpBuf.data(), mTmpBuf.data(), numBins);
  
    // Finally, smooth using the time integration value
    mDataSmooth[ch].Process(mTmpBuf);

    for (auto i = 0; i < numBins; i++)
      mYPoints[ch][i] = mTmpBuf[i];

    if (mFillCurves)
    {
      // Used to close the path outside the bounds of the control
      auto offset = mCurveThickness/mWidgetBounds.H();

      mYPoints[ch][numBins] = -offset;
      mYPoints[ch][numBins+1] = -offset;
    }
    
    SetDirty(false);
  }

  float CalcXNorm(float x, EFrequencyScale scale, float sampleRate, bool inverted = false)
  {
    const auto nyquist = NyquistFreq();
    
    switch (scale)
    {
      case EFrequencyScale::Linear:
      {
        if (!inverted)
          return (x / nyquist - mFreqLo) / (mFreqHi - mFreqLo);
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
      case EFrequencyScale::Mel:
      {
        const auto melXLo = HzToMel(mFreqLo);
        const auto melXHi = HzToMel(mFreqHi);
        
        if (!inverted)
          return (HzToMel(x) - melXLo)/(melXHi - melXLo);
        else
          return (1.0/nyquist) * MelToHz(melXLo + x * (melXHi - melXLo));
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

  int NumPoints() const
  {
    return mFillCurves ? NumBins() + 2 : NumBins();
  }
  
  int NumBins() const { return mFFTSize / 2; }

  double NyquistFreq() const { return mSampleRate * 0.5; }

private:
  std::vector<IColor> mChannelColors;
  EFrequencyScale mFreqScale;
  EAmplitudeScale mAmpScale;

  double mSampleRate = 44100.0;
  
  int mFFTSize = 1024;

  // std::unique_ptr<IFrequencyAxis> mFreqAxis;
  // std::unique_ptr<IAmpAxis> mAmpAxis;

  float mCurveThickness = 1.0f;
  float mGridThickness = 1.0f;
  bool mFillCurves = false;
  float mFillOpacity = 0.5f;
  
  float mFreqLo = 20.0;
  float mFreqHi = 22050.0;
  float mAmpLo = 0.0;
  float mAmpHi = 1.0;

  std::vector<float> mXPoints;
  std::array<std::vector<float>, MAXNC> mYPoints;
  
  float mCursorAmp = 0.0;
  float mCursorFreq = -1.0;

  EChannelType mChanType = EChannelType::LeftAndRight;

  LogDataSmooth<float> mDataSmooth[MAXNC];
  float mSmoothTime = 2.0;
  float mOctaveGain = 0.0;
  
  IPopupMenu mMenu {"Options"};

  std::vector<float> mTmpBuf;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
