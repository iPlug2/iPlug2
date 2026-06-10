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
    auto* pOverlapMenu = mMenu.AddItem("Overlap", new IPopupMenu("Overlap", { "1x", "2x", "4x", "8x" }))->GetSubmenu();
    auto* pWindowMenu = mMenu.AddItem("Window", new IPopupMenu("Window", { "Hann", "Blackman Harris", "Hamming", "Flattop", "Rectangular" }))->GetSubmenu();

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

    // Overlap checks
    pOverlapMenu->CheckItem(0, mOverlap == 1);
    pOverlapMenu->CheckItem(1, mOverlap == 2);
    pOverlapMenu->CheckItem(2, mOverlap == 4);
    pOverlapMenu->CheckItem(3, mOverlap == 8);

    // Window checks (indices match enum order)
    pWindowMenu->CheckItem(0, mWindowType == 0);
    pWindowMenu->CheckItem(1, mWindowType == 1);
    pWindowMenu->CheckItem(2, mWindowType == 2);
    pWindowMenu->CheckItem(3, mWindowType == 3);
    pWindowMenu->CheckItem(4, mWindowType == 4);
    
    GetUI()->CreatePopupMenu(*this, mMenu, x, y);
  }

  void OnMouseOver(float x, float y, const IMouseMod& mod) override
  {
    if (mPlotBounds.W() <= 0.f || mPlotBounds.H() <= 0.f)
      return;

    mPlotBounds.Constrain(x, y);

    const float normalizedX = (x - mPlotBounds.L) / mPlotBounds.W();
    const float normalizedY = 1.f - ((y - mPlotBounds.T) / mPlotBounds.H());

    mCursorAmp = CalcYNorm(normalizedY, mAmpScale, true);
    mCursorFreq = CalcXNorm(normalizedX, mFreqScale, true) * NyquistFreq();
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
      else if (strcmp(title, "Overlap") == 0)
      {
        const char* txt = pSelectedMenu->GetChosenItem()->GetText();
        int overlap = atoi(txt); // works for strings like "1x", "2x"
        if (overlap <= 0)
          overlap = 1;
        GetDelegate()->SendArbitraryMsgFromUI(kMsgTagOverlap, kNoTag, sizeof(int), &overlap);
        mOverlap = overlap;
      }
      else if (strcmp(title, "Window") == 0)
      {
        int idx = pSelectedMenu->GetChosenItemIdx();
        GetDelegate()->SendArbitraryMsgFromUI(kMsgTagWindowType, kNoTag, sizeof(int), &idx);
        mWindowType = idx;
      }
    }
  }

  void OnResize() override
  {
    SetTargetRECT(MakeRects(mRECT));
    UpdatePlotBounds();
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
    else if (msgTag == kMsgTagOverlap)
    {
      int overlap;
      stream.Get(&overlap, 0);
      mOverlap = overlap;
    }
    else if (msgTag == kMsgTagWindowType)
    {
      int windowType;
      stream.Get(&windowType, 0);
      mWindowType = windowType;
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
    DrawAxisLabels(g);
    DrawLabel(g);
    DrawCursorValues(g);
    
    if (mStyle.drawFrame)
      g.DrawRect(GetColor(kFR), mWidgetBounds, &mBlend, mStyle.frameThickness);
  }

private:
  IText GetAxisLabelText() const
  {
    return mStyle.valueText.WithFGColor(GetColor(kFG))
                           .WithSize(std::max(11.f, mStyle.valueText.mSize * 0.95f));
  }

  void UpdatePlotBounds()
  {
    mPlotBounds = mWidgetBounds;

    const float topPadding = 8.f;
    const float rightPadding = 10.f;
    float leftPadding = 48.f;
    float bottomPadding = 22.f;

    if (auto* pUI = GetUI())
    {
      const IText axisText = GetAxisLabelText();
      IRECT textBounds;
      WDL_String ampLabel;

      ampLabel.SetFormatted(64, "%ddB", static_cast<int>(std::floor(AmpToDB(mAmpLo))));
      pUI->MeasureText(axisText, ampLabel.Get(), textBounds);
      leftPadding = textBounds.W() + 14.f;

      pUI->MeasureText(axisText, "20kHz", textBounds);
      bottomPadding = textBounds.H() + 12.f;
    }

    mPlotBounds.L += leftPadding;
    mPlotBounds.T += topPadding;
    mPlotBounds.R -= rightPadding;
    mPlotBounds.B -= bottomPadding;

    if (mPlotBounds.W() <= 0.f || mPlotBounds.H() <= 0.f)
      mPlotBounds = mWidgetBounds;
  }

  void DrawAxisLabels(IGraphics& g)
  {
    if (mPlotBounds.W() <= 0.f || mPlotBounds.H() <= 0.f)
      return;

    const IText axisText = GetAxisLabelText();
    const IText freqText = axisText.WithAlign(EAlign::Center).WithVAlign(EVAlign::Top);
    const IText ampText = axisText.WithAlign(EAlign::Far).WithVAlign(EVAlign::Middle);
    IRECT textBounds;

    g.MeasureText(axisText, "20kHz", textBounds);
    const float freqLabelHalfWidth = textBounds.W() * 0.5f;
    const float freqLabelHeight = textBounds.H();

    float previousRight = mPlotBounds.L - 6.f;
    constexpr float kFrequencyTicks[] = {50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f};

    for (const float freq : kFrequencyTicks)
    {
      if (freq < mFreqLo || freq > mFreqHi)
        continue;

      WDL_String label;
      if (freq >= 1000.f)
        label.SetFormatted(32, "%.0fkHz", freq / 1000.f);
      else
        label.SetFormatted(32, "%.0fHz", freq);

      g.MeasureText(freqText, label.Get(), textBounds);

      const float x = mPlotBounds.L + CalcXNorm(freq, mFreqScale) * mPlotBounds.W();
      IRECT labelRect(x - std::max(freqLabelHalfWidth, textBounds.W() * 0.5f),
                      mPlotBounds.B + 4.f,
                      x + std::max(freqLabelHalfWidth, textBounds.W() * 0.5f),
                      mPlotBounds.B + 4.f + freqLabelHeight);

      if (labelRect.L <= previousRight + 4.f)
        continue;

      labelRect.L = std::max(labelRect.L, mPlotBounds.L);
      labelRect.R = std::min(labelRect.R, mWidgetBounds.R);
      g.DrawText(freqText, label.Get(), labelRect, &mBlend);
      previousRight = labelRect.R;
    }

    const float dBLo = AmpToDB(mAmpLo);
    const float dBHi = AmpToDB(mAmpHi);
    float previousBottom = mWidgetBounds.T - 1.f;

    for (float ampDB = dBHi; ampDB >= dBLo; ampDB -= 10.f)
    {
      WDL_String label;
      label.SetFormatted(32, "%ddB", static_cast<int>(ampDB));
      g.MeasureText(ampText, label.Get(), textBounds);

      const float t = Clip(CalcYNorm(ampDB, EAmplitudeScale::Decibel), 0.0f, 1.0f);
      const float y = mPlotBounds.B - t * mPlotBounds.H();
      IRECT labelRect(mWidgetBounds.L + 2.f,
                      y - textBounds.H() * 0.5f,
                      mPlotBounds.L - 6.f,
                      y + textBounds.H() * 0.5f);

      if (labelRect.T <= previousBottom + 2.f)
        continue;

      g.DrawText(ampText, label.Get(), labelRect, &mBlend);
      previousBottom = labelRect.B;
    }
  }

  void DrawGrids(IGraphics& g)
  {
    // Frequency Grid
    auto freq = mFreqLo;
    
    while (freq <= mFreqHi)
    {
      auto t = CalcXNorm(freq, mFreqScale);
      auto x0 = mPlotBounds.L + t * mPlotBounds.W();
      auto y0 = mPlotBounds.B;
      auto x1 = x0;
      auto y1 = mPlotBounds.T;
      
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
        
        auto x0 = mPlotBounds.L;
        auto y0 = mPlotBounds.B - t * mPlotBounds.H();
        auto x1 = mPlotBounds.R;
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

      const int nBins = NumBins();
      const IColor baseColor = mChannelColors[c];

      // Build the spectrum path (optionally smoothed using cubic Beziers)
      g.PathClear();
      float x0 = mPlotBounds.L + mXPoints[0] * mPlotBounds.W();
      float y0 = mPlotBounds.B - mYPoints[c][0] * mPlotBounds.H();
      g.PathMoveTo(x0, y0);

      if (mCurveSmoothing > 0.f && nBins > 3)
      {
        // Catmull-Rom to Bezier conversion with adjustable tension (mCurveSmoothing in [0,1])
        const float s = mCurveSmoothing; // 0 -> straight lines, 1 -> classic Catmull-Rom
        for (int i = 0; i < nBins - 1; ++i)
        {
          const int i0 = std::max(i - 1, 0);
          const int i1 = i;
          const int i2 = i + 1;
          const int i3 = std::min(i + 2, nBins - 1);

          const float x1 = mPlotBounds.L + mXPoints[i1] * mPlotBounds.W();
          const float y1 = mPlotBounds.B - mYPoints[c][i1] * mPlotBounds.H();
          const float x2 = mPlotBounds.L + mXPoints[i2] * mPlotBounds.W();
          const float y2 = mPlotBounds.B - mYPoints[c][i2] * mPlotBounds.H();

          const float x0s = mPlotBounds.L + mXPoints[i0] * mPlotBounds.W();
          const float y0s = mPlotBounds.B - mYPoints[c][i0] * mPlotBounds.H();
          const float x3 = mPlotBounds.L + mXPoints[i3] * mPlotBounds.W();
          const float y3 = mPlotBounds.B - mYPoints[c][i3] * mPlotBounds.H();

          const float c1x = x1 + (x2 - x0s) * (s / 6.f);
          const float c1y = y1 + (y2 - y0s) * (s / 6.f);
          const float c2x = x2 - (x3 - x1) * (s / 6.f);
          const float c2y = y2 - (y3 - y1) * (s / 6.f);

          g.PathCubicBezierTo(c1x, c1y, c2x, c2y, x2, y2);
        }
      }
      else
      {
        for (int i = 1; i < nBins; ++i)
        {
          float xi = mPlotBounds.L + mXPoints[i] * mPlotBounds.W();
          float yi = mPlotBounds.B - mYPoints[c][i] * mPlotBounds.H();
          g.PathLineTo(xi, yi);
        }
      }

      // Fill under the curve with a vertical gradient to transparent
      if (FillCurves())
      {
        // Close the path down to the baseline and back to the start
        float xLast = mPlotBounds.L + mXPoints[nBins-1] * mPlotBounds.W();
        g.PathLineTo(xLast, mPlotBounds.B);
        g.PathLineTo(x0, mPlotBounds.B);
        g.PathClose();

        const IColor topCol = baseColor.WithOpacity(mFillOpacity);
        const IColor botCol = baseColor.WithOpacity(0.f);
        IPattern fill = IPattern::CreateLinearGradient(mPlotBounds, EDirection::Vertical, { IColorStop(topCol, 0.f), IColorStop(botCol, 1.f) });
        g.PathFill(fill);

        // Recreate the path for stroking (PathFill may consume it on some backends)
        g.PathClear();
        g.PathMoveTo(x0, y0);
        if (mCurveSmoothing > 0.f && nBins > 3)
        {
          const float s = mCurveSmoothing;
          for (int i = 0; i < nBins - 1; ++i)
          {
            const int i0 = std::max(i - 1, 0);
            const int i1 = i;
            const int i2 = i + 1;
            const int i3 = std::min(i + 2, nBins - 1);

            const float x1 = mPlotBounds.L + mXPoints[i1] * mPlotBounds.W();
            const float y1 = mPlotBounds.B - mYPoints[c][i1] * mPlotBounds.H();
            const float x2 = mPlotBounds.L + mXPoints[i2] * mPlotBounds.W();
            const float y2 = mPlotBounds.B - mYPoints[c][i2] * mPlotBounds.H();

            const float x0s = mPlotBounds.L + mXPoints[i0] * mPlotBounds.W();
            const float y0s = mPlotBounds.B - mYPoints[c][i0] * mPlotBounds.H();
            const float x3 = mPlotBounds.L + mXPoints[i3] * mPlotBounds.W();
            const float y3 = mPlotBounds.B - mYPoints[c][i3] * mPlotBounds.H();

            const float c1x = x1 + (x2 - x0s) * (s / 6.f);
            const float c1y = y1 + (y2 - y0s) * (s / 6.f);
            const float c2x = x2 - (x3 - x1) * (s / 6.f);
            const float c2y = y2 - (y3 - y1) * (s / 6.f);

            g.PathCubicBezierTo(c1x, c1y, c2x, c2y, x2, y2);
          }
        }
        else
        {
          for (int i = 1; i < nBins; ++i)
          {
            float xi = mPlotBounds.L + mXPoints[i] * mPlotBounds.W();
            float yi = mPlotBounds.B - mYPoints[c][i] * mPlotBounds.H();
            g.PathLineTo(xi, yi);
          }
        }
      }

      // Stroke the curve for crispness
      g.PathStroke(IPattern(baseColor), mCurveThickness);
    }
  }

  void DrawCursorValues(IGraphics& g)
  {
    WDL_String label;

    if (mCursorFreq >= 0.0)
    {
      label.SetFormatted(64, "%.1fHz", mCursorFreq);
      g.DrawText(mStyle.valueText, label.Get(), mPlotBounds.GetFromTRHC(100, 50).FracRectVertical(0.5));
    }
    
    if (mAmpScale == EAmplitudeScale::Linear)
      label.SetFormatted(64, "%.3fs", mCursorAmp);
    else
      label.SetFormatted(64, "%ddB", (int) mCursorAmp);
    
    g.DrawText(mStyle.valueText, label.Get(), mPlotBounds.GetFromTRHC(100, 50).FracRectVertical(0.5, true));
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
    UpdatePlotBounds();
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
    UpdatePlotBounds();
    SetDirty(false);
  }

  void SetOctaveGain(float octaveGain)
  {
    mOctaveGain = octaveGain;
    SetDirty(false);
  }

  void SetCurveSmoothing(float amount)
  {
    mCurveSmoothing = Clip(amount, 0.f, 1.f);
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
      const auto binNorm = numBins > 1 ? static_cast<float>(i) / static_cast<float>(numBins - 1) : 0.f;
      const auto adjustedAmp = ApplyOctaveGain(powerSpectrum[i], binNorm);
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
      auto offset = mCurveThickness / std::max(mPlotBounds.H(), 1.f);

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
  IRECT mPlotBounds;

  std::vector<float> mXPoints;
  std::array<std::vector<float>, MAXNC> mYPoints;
  std::array<std::vector<float>, MAXNC> mEnvelopeValues;
  float mAttackCoeff = 0.2f;
  float mReleaseCoeff = 0.99f;
  int mOverlap = 1;
  int mWindowType = 0; // matches ISpectrumSender<>::EWindowType::Hann
  float mCurveSmoothing = 0.6f; // 0 = straight lines, 1 = full Catmull-Rom
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
