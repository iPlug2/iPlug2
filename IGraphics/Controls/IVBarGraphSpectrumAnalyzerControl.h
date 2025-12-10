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
 * @copydoc IVBarGraphSpectrumAnalyzerControl
 */

#include "IControl.h"
#include "ISender.h"
#include "IPlugStructs.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** LED Range for spectrum analyzer bar segments */
struct SpectrumLEDRange
{
  float lowRangeDB;
  float highRangeDB;
  int nSegs;
  IColor color;

  SpectrumLEDRange(float low, float high, int segs, IColor c)
  : lowRangeDB(low), highRangeDB(high), nSegs(segs), color(c) {}
};

// Default LED colors for spectrum analyzer
const static IColor SPEC_LED1 = {255, 36, 157, 16};   // Green (quiet)
const static IColor SPEC_LED2 = {255, 153, 191, 28};  // Yellow-green
const static IColor SPEC_LED3 = {255, 215, 222, 37};  // Yellow
const static IColor SPEC_LED4 = {255, 247, 153, 33};  // Orange
const static IColor SPEC_LED5 = COLOR_RED;            // Red (loud)

// ISO standard 1/3 octave center frequencies (Hz)
const static float kThirdOctaveCenterFreqs[] = {
  20.f, 25.f, 31.5f, 40.f, 50.f, 63.f, 80.f, 100.f, 125.f, 160.f,
  200.f, 250.f, 315.f, 400.f, 500.f, 630.f, 800.f, 1000.f, 1250.f, 1600.f,
  2000.f, 2500.f, 3150.f, 4000.f, 5000.f, 6300.f, 8000.f, 10000.f, 12500.f, 16000.f, 20000.f
};
const static int kNumThirdOctaveBands = 31;

/** Vectorial bar graph spectrum analyzer control with segmented LEDs
 * @ingroup IControls */
template <int MAXNC = 2, int MAX_FFT_SIZE = 4096>
class IVBarGraphSpectrumAnalyzerControl : public IControl
                                        , public IVectorBase
{
public:
  using TDataPacket = std::array<float, MAX_FFT_SIZE>;

  /** Message tags for synchronization with ISpectrumSender */
  enum MsgTags
  {
    kMsgTagSampleRate = 1,
    kMsgTagFFTSize,
    kMsgTagOverlap,
    kMsgTagWindowType,
    kMsgTagOctaveGain
  };

  enum class EFrequencyScale { Linear, Log, ThirdOctave };
  enum class EColorMode { Segments, Smooth };
  enum class EChannelMode { Left, Right, Sum, SideBySide, Split };

  static constexpr float kPeakDecayRate = 0.95f;

  /** Create a bar graph spectrum analyzer
   * @param bounds The rectangular area that the control occupies
   * @param label A CString to label the control
   * @param style IVStyle for the control
   * @param nBars Number of frequency bars to display
   * @param nSegsPerBar Number of vertical LED segments per bar
   * @param freqScale Linear or logarithmic frequency distribution
   * @param colorMode Segments (LED-style) or Smooth (continuous gradient)
   * @param channelMode How to display stereo channels (Left, Right, Sum, SideBySide)
   * @param barPattern IPattern for Smooth mode - solid color or gradient
   * @param ledRanges LED color ranges for Segments mode
   * @param gapRatio Gap between bars as ratio of bar width (0-1)
   * @param segGapRatio Gap between segments as ratio of segment height (0-1)
   * @param attackTimeMs Attack smoothing in milliseconds
   * @param decayTimeMs Decay smoothing in milliseconds
   * @param lowRangeDB Lower dB range for display
   * @param highRangeDB Upper dB range for display */
  IVBarGraphSpectrumAnalyzerControl(const IRECT& bounds,
                                     const char* label = "",
                                     const IVStyle& style = DEFAULT_STYLE,
                                     int nBars = 32,
                                     int nSegsPerBar = 16,
                                     EFrequencyScale freqScale = EFrequencyScale::Log,
                                     EColorMode colorMode = EColorMode::Segments,
                                     EChannelMode channelMode = EChannelMode::Sum,
                                     const IPattern& barPattern = IPattern(COLOR_GREEN),
                                     const std::vector<SpectrumLEDRange>& ledRanges = {
                                       {0.f, 6.f, 2, SPEC_LED5},
                                       {-12.f, 0.f, 3, SPEC_LED4},
                                       {-24.f, -12.f, 3, SPEC_LED3},
                                       {-48.f, -24.f, 4, SPEC_LED2},
                                       {-72.f, -48.f, 4, SPEC_LED1}
                                     },
                                     float gapRatio = 0.2f,
                                     float segGapRatio = 0.1f,
                                     float attackTimeMs = 5.0f,
                                     float decayTimeMs = 50.0f,
                                     float lowRangeDB = -72.f,
                                     float highRangeDB = 6.f)
  : IControl(bounds)
  , IVectorBase(style)
  , mNBars(nBars)
  , mNSegsPerBar(nSegsPerBar)
  , mFreqScale(freqScale)
  , mColorMode(colorMode)
  , mChannelMode(channelMode)
  , mBarPattern(barPattern)
  , mLEDRanges(ledRanges)
  , mGapRatio(gapRatio)
  , mSegGapRatio(segGapRatio)
  , mAttackTimeMs(attackTimeMs)
  , mDecayTimeMs(decayTimeMs)
  , mLowRangeDB(lowRangeDB)
  , mHighRangeDB(highRangeDB)
  {
    AttachIControl(this, label);
    ResizeBarArrays();
    SetFFTSize(1024);
    CalculateFrequencyBands();
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
      ProcessSpectrumData(d);
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
      SetOctaveGain(static_cast<float>(octaveGain));
    }
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if (mod.R)
    {
      mMenu.Clear(true);
      auto* pFftSizeMenu = mMenu.AddItem("FFT Size", new IPopupMenu("FFT Size", { "64", "128", "256", "512", "1024", "2048", "4096" }))->GetSubmenu();
      auto* pChansMenu = mMenu.AddItem("Channels", new IPopupMenu("Channels", { "L", "R", "Sum", "L + R", "L | R" }))->GetSubmenu();
      auto* pFreqScaleMenu = mMenu.AddItem("Freq Scaling", new IPopupMenu("Freq Scaling", { "Linear", "Log", "1/3 Octave" }))->GetSubmenu();
      auto* pOverlapMenu = mMenu.AddItem("Overlap", new IPopupMenu("Overlap", { "1x", "2x", "4x", "8x" }))->GetSubmenu();
      auto* pWindowMenu = mMenu.AddItem("Window", new IPopupMenu("Window", { "Hann", "Blackman Harris", "Hamming", "Flattop", "Rectangular" }))->GetSubmenu();

      pFftSizeMenu->CheckItem(0, mFFTSize == 64);
      pFftSizeMenu->CheckItem(1, mFFTSize == 128);
      pFftSizeMenu->CheckItem(2, mFFTSize == 256);
      pFftSizeMenu->CheckItem(3, mFFTSize == 512);
      pFftSizeMenu->CheckItem(4, mFFTSize == 1024);
      pFftSizeMenu->CheckItem(5, mFFTSize == 2048);
      pFftSizeMenu->CheckItem(6, mFFTSize == 4096);

      pChansMenu->CheckItem(0, mChannelMode == EChannelMode::Left);
      pChansMenu->CheckItem(1, mChannelMode == EChannelMode::Right);
      pChansMenu->CheckItem(2, mChannelMode == EChannelMode::Sum);
      pChansMenu->CheckItem(3, mChannelMode == EChannelMode::SideBySide);
      pChansMenu->CheckItem(4, mChannelMode == EChannelMode::Split);

      pFreqScaleMenu->CheckItem(0, mFreqScale == EFrequencyScale::Linear);
      pFreqScaleMenu->CheckItem(1, mFreqScale == EFrequencyScale::Log);
      pFreqScaleMenu->CheckItem(2, mFreqScale == EFrequencyScale::ThirdOctave);

      pOverlapMenu->CheckItem(0, mOverlap == 1);
      pOverlapMenu->CheckItem(1, mOverlap == 2);
      pOverlapMenu->CheckItem(2, mOverlap == 4);
      pOverlapMenu->CheckItem(3, mOverlap == 8);

      pWindowMenu->CheckItem(0, mWindowType == 0);
      pWindowMenu->CheckItem(1, mWindowType == 1);
      pWindowMenu->CheckItem(2, mWindowType == 2);
      pWindowMenu->CheckItem(3, mWindowType == 3);
      pWindowMenu->CheckItem(4, mWindowType == 4);

      GetUI()->CreatePopupMenu(*this, mMenu, x, y);
    }
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
        int idx = pSelectedMenu->GetChosenItemIdx();
        switch (idx)
        {
          case 0: SetChannelMode(EChannelMode::Left); break;
          case 1: SetChannelMode(EChannelMode::Right); break;
          case 2: SetChannelMode(EChannelMode::Sum); break;
          case 3: SetChannelMode(EChannelMode::SideBySide); break;
          case 4: SetChannelMode(EChannelMode::Split); break;
        }
      }
      else if (strcmp(title, "Freq Scaling") == 0)
      {
        int idx = pSelectedMenu->GetChosenItemIdx();
        EFrequencyScale scale = EFrequencyScale::Linear;
        if (idx == 1) scale = EFrequencyScale::Log;
        else if (idx == 2) scale = EFrequencyScale::ThirdOctave;
        SetFrequencyScale(scale);
      }
      else if (strcmp(title, "Overlap") == 0)
      {
        const char* txt = pSelectedMenu->GetChosenItem()->GetText();
        int overlap = atoi(txt);
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

  void Draw(IGraphics& g) override
  {
    DrawBackground(g, mRECT);
    DrawWidget(g);
    DrawLabel(g);

    if (mStyle.drawFrame)
    {
      if (mChannelMode == EChannelMode::Split)
      {
        // Draw separate frames for left and right halves
        const float halfWidth = mWidgetBounds.W() * 0.5f;
        IRECT leftBounds(mWidgetBounds.L, mWidgetBounds.T, mWidgetBounds.L + halfWidth, mWidgetBounds.B);
        IRECT rightBounds(mWidgetBounds.L + halfWidth, mWidgetBounds.T, mWidgetBounds.R, mWidgetBounds.B);
        g.DrawRect(GetColor(kFR), leftBounds, &mBlend, mStyle.frameThickness);
        g.DrawRect(GetColor(kFR), rightBounds, &mBlend, mStyle.frameThickness);
      }
      else
      {
        g.DrawRect(GetColor(kFR), mWidgetBounds, &mBlend, mStyle.frameThickness);
      }
    }
  }

  void SetNumBars(int nBars)
  {
    mNBars = nBars;
    ResizeBarArrays();
    CalculateFrequencyBands();
    SetDirty(false);
  }

  void SetNumSegments(int nSegs)
  {
    mNSegsPerBar = nSegs;
    SetDirty(false);
  }

  void SetColorMode(EColorMode mode)
  {
    mColorMode = mode;
    SetDirty(false);
  }

  void SetChannelMode(EChannelMode mode)
  {
    mChannelMode = mode;
    ResizeBarArrays();
    SetDirty(false);
  }

  /** Set the bar pattern for Smooth color mode
   * @param pattern An IPattern - solid color or gradient */
  void SetBarPattern(const IPattern& pattern)
  {
    mBarPattern = pattern;
    SetDirty(false);
  }

  void SetFrequencyScale(EFrequencyScale scale)
  {
    // Store current bar count when leaving non-ThirdOctave mode
    if (mFreqScale != EFrequencyScale::ThirdOctave && scale == EFrequencyScale::ThirdOctave)
    {
      mStoredNBars = mNBars;
    }
    // Restore bar count when leaving ThirdOctave mode
    else if (mFreqScale == EFrequencyScale::ThirdOctave && scale != EFrequencyScale::ThirdOctave)
    {
      mNBars = mStoredNBars;
      ResizeBarArrays();
    }

    mFreqScale = scale;
    CalculateFrequencyBands();
    SetDirty(false);
  }

  void SetPeakHoldTime(int timeMs)
  {
    mPeakHoldTimeMs = timeMs;
  }

  void SetShowPeaks(bool show)
  {
    mShowPeaks = show;
    SetDirty(false);
  }

  /** Enable a clip indicator segment above a threshold
   * @param thresholdDB The dB level where clipping starts (e.g., 0.0 for 0dB)
   * @param clipColor The color for the clip indicator segment */
  void SetClipIndicator(float thresholdDB, const IColor& clipColor = COLOR_RED)
  {
    mClipThresholdDB = thresholdDB;
    mClipColor = clipColor;
    mShowClipIndicator = true;
    SetDirty(false);
  }

  void SetShowClipIndicator(bool show)
  {
    mShowClipIndicator = show;
    SetDirty(false);
  }

  void SetFFTSize(int fftSize)
  {
    mFFTSize = fftSize;
    SetSmoothing(mAttackTimeMs, mDecayTimeMs);
    CalculateFrequencyBands();
    SetDirty(false);
  }

  void SetSampleRate(double sr)
  {
    mSampleRate = sr;
    SetSmoothing(mAttackTimeMs, mDecayTimeMs);
    CalculateFrequencyBands();
    SetDirty(false);
  }

  void SetOctaveGain(float octaveGain)
  {
    mOctaveGain = octaveGain;
    SetDirty(false);
  }

private:
  void ResizeBarArrays()
  {
    const int numBars = (mChannelMode == EChannelMode::SideBySide || mChannelMode == EChannelMode::Split) ? mNBars * 2 : mNBars;
    mBarValues.resize(numBars, 0.f);
    mPeakValues.resize(numBars, 0.f);
    mPeakHoldCounters.resize(numBars, 0);
  }

  void DrawWidget(IGraphics& g) override
  {
    const float totalWidth = mWidgetBounds.W();
    const float totalHeight = mWidgetBounds.H();
    const float segHeightWithGap = totalHeight / mNSegsPerBar;
    const float segHeight = segHeightWithGap * (1.f - mSegGapRatio);
    const float segGap = segHeightWithGap * mSegGapRatio;

    if (mChannelMode == EChannelMode::SideBySide)
    {
      // Draw L and R bars side by side for each frequency band
      const float pairWidthWithGap = totalWidth / mNBars;
      const float pairGap = pairWidthWithGap * mGapRatio;
      const float pairWidth = pairWidthWithGap - pairGap;
      const float singleBarWidth = pairWidth * 0.5f;

      for (int bar = 0; bar < mNBars; bar++)
      {
        const float pairX = mWidgetBounds.L + bar * pairWidthWithGap + pairGap * 0.5f;

        // Left channel bar
        const float leftBarX = pairX;
        const float leftValue = mBarValues[bar * 2];
        DrawBar(g, leftBarX, singleBarWidth, segHeight, segGap, leftValue, 0);

        if (mShowPeaks && mPeakValues[bar * 2] > 0.001f)
        {
          DrawPeakIndicator(g, leftBarX, singleBarWidth, segHeightWithGap, mPeakValues[bar * 2], 0);
        }

        // Right channel bar
        const float rightBarX = pairX + singleBarWidth;
        const float rightValue = mBarValues[bar * 2 + 1];
        DrawBar(g, rightBarX, singleBarWidth, segHeight, segGap, rightValue, 1);

        if (mShowPeaks && mPeakValues[bar * 2 + 1] > 0.001f)
        {
          DrawPeakIndicator(g, rightBarX, singleBarWidth, segHeightWithGap, mPeakValues[bar * 2 + 1], 1);
        }
      }
    }
    else if (mChannelMode == EChannelMode::Split)
    {
      // Draw all L bars on left half, all R bars on right half
      const float halfWidth = totalWidth * 0.5f;
      const float barWidthWithGap = halfWidth / mNBars;
      const float barWidth = barWidthWithGap * (1.f - mGapRatio);
      const float gapWidth = barWidthWithGap * mGapRatio;

      // Left channel bars (left half)
      for (int bar = 0; bar < mNBars; bar++)
      {
        const float barX = mWidgetBounds.L + bar * barWidthWithGap + gapWidth * 0.5f;
        const float barValue = mBarValues[bar * 2];

        DrawBar(g, barX, barWidth, segHeight, segGap, barValue, 0);

        if (mShowPeaks && mPeakValues[bar * 2] > 0.001f)
        {
          DrawPeakIndicator(g, barX, barWidth, segHeightWithGap, mPeakValues[bar * 2], 0);
        }
      }

      // Right channel bars (right half)
      for (int bar = 0; bar < mNBars; bar++)
      {
        const float barX = mWidgetBounds.L + halfWidth + bar * barWidthWithGap + gapWidth * 0.5f;
        const float barValue = mBarValues[bar * 2 + 1];

        DrawBar(g, barX, barWidth, segHeight, segGap, barValue, 1);

        if (mShowPeaks && mPeakValues[bar * 2 + 1] > 0.001f)
        {
          DrawPeakIndicator(g, barX, barWidth, segHeightWithGap, mPeakValues[bar * 2 + 1], 1);
        }
      }
    }
    else
    {
      // Single bar per frequency band (Left, Right, or Sum)
      const float barWidthWithGap = totalWidth / mNBars;
      const float barWidth = barWidthWithGap * (1.f - mGapRatio);
      const float gapWidth = barWidthWithGap * mGapRatio;

      for (int bar = 0; bar < mNBars; bar++)
      {
        const float barX = mWidgetBounds.L + bar * barWidthWithGap + gapWidth * 0.5f;
        const float barValue = mBarValues[bar];

        DrawBar(g, barX, barWidth, segHeight, segGap, barValue);

        if (mShowPeaks && mPeakValues[bar] > 0.001f)
        {
          DrawPeakIndicator(g, barX, barWidth, segHeightWithGap, mPeakValues[bar]);
        }
      }
    }
  }

  void DrawBar(IGraphics& g, float barX, float barWidth, float segHeight, float segGap, float value, int channelIdx = -1)
  {
    if (mColorMode == EColorMode::Smooth)
    {
      // Smooth mode - continuous fill using IPattern (solid or gradient)
      if (value > 0.001f)
      {
        // Calculate clip threshold as normalized value
        const float clipThresholdNorm = mShowClipIndicator ?
          (mClipThresholdDB - mLowRangeDB) / (mHighRangeDB - mLowRangeDB) : 1.0f;

        const bool isClipping = mShowClipIndicator && value > clipThresholdNorm;

        // Draw gradient portion - if clipping, only draw up to threshold; otherwise draw full value
        const float gradientValue = isClipping ? clipThresholdNorm : value;
        if (gradientValue > 0.001f)
        {
          const float barHeight = gradientValue * mWidgetBounds.H();
          const float barY = mWidgetBounds.B - barHeight;
          IRECT barRect(barX, barY, barX + barWidth, mWidgetBounds.B);

          // Create a vertical gradient for this bar based on the stored pattern
          IPattern pattern = IPattern::CreateLinearGradient(barX, mWidgetBounds.B, barX, mWidgetBounds.T);

          // Copy stops from the stored pattern
          for (int i = 0; i < mBarPattern.NStops(); i++)
          {
            const IColorStop& stop = mBarPattern.GetStop(i);
            pattern.AddStop(stop.mColor, stop.mOffset);
          }

          g.PathRect(barRect);
          g.PathFill(pattern);
        }

        // Draw clip indicator on top if value exceeds threshold
        if (isClipping)
        {
          const float clipStartY = mWidgetBounds.B - (clipThresholdNorm * mWidgetBounds.H());
          const float clipEndY = mWidgetBounds.T;
          IRECT clipRect(barX, clipEndY, barX + barWidth, clipStartY);

          IColor clipColor = mClipColor;
          if (channelIdx == 1 && mChannelMode != EChannelMode::Split)
          {
            clipColor = clipColor.WithContrast(0.2f);
          }
          g.FillRect(clipColor, clipRect);
        }
      }
    }
    else
    {
      // Segments mode - LED-style segments with color ranges
      int segIdx = 0;

      for (const auto& range : mLEDRanges)
      {
        for (int i = 0; i < range.nSegs; i++)
        {
          const int totalSeg = mNSegsPerBar - 1 - segIdx;
          const float segNorm = static_cast<float>(totalSeg + 1) / mNSegsPerBar;

          if (value >= segNorm - 0.001f)
          {
            const float segY = mWidgetBounds.B - (totalSeg + 1) * (segHeight + segGap) + segGap * 0.5f;
            IRECT segRect(barX, segY, barX + barWidth, segY + segHeight);
            IColor segColor = range.color;
            // In SideBySide mode, slightly adjust color for R channel (not in Split mode)
            if (channelIdx == 1 && mChannelMode != EChannelMode::Split)
            {
              segColor = segColor.WithContrast(0.2f);
            }
            g.FillRect(segColor, segRect);
          }

          segIdx++;
          if (segIdx >= mNSegsPerBar) break;
        }
        if (segIdx >= mNSegsPerBar) break;
      }
    }
  }

  void DrawPeakIndicator(IGraphics& g, float barX, float barWidth, float segHeightWithGap, float peakValue, int channelIdx = -1)
  {
    if (peakValue <= 0.001f) return;

    // In Smooth mode with clip indicator, don't draw peak if it's in the clip region
    if (mColorMode == EColorMode::Smooth && mShowClipIndicator)
    {
      const float clipThresholdNorm = (mClipThresholdDB - mLowRangeDB) / (mHighRangeDB - mLowRangeDB);
      if (peakValue >= clipThresholdNorm)
        return; // Peak is in clip region, don't draw
    }

    float peakY;
    if (mColorMode == EColorMode::Smooth)
    {
      // Continuous positioning for smooth mode
      peakY = mWidgetBounds.B - (peakValue * mWidgetBounds.H());
    }
    else
    {
      // Segment-based positioning for LED mode
      const int peakSeg = static_cast<int>(peakValue * mNSegsPerBar);
      if (peakSeg <= 0) return;
      peakY = mWidgetBounds.B - peakSeg * segHeightWithGap;
    }

    IRECT peakRect(barX, peakY, barX + barWidth, peakY + 2.f);

    IColor peakColor;
    if (mColorMode == EColorMode::Segments)
    {
      peakColor = GetColorForLevel(peakValue);
    }
    else
    {
      peakColor = GetPatternColorAtPosition(peakValue);
    }
    if (channelIdx == 1 && mChannelMode != EChannelMode::Split)
    {
      peakColor = peakColor.WithContrast(0.2f);
    }

    g.FillRect(peakColor, peakRect);
  }

  IColor GetColorForLevel(float normalizedLevel)
  {
    const float dB = mLowRangeDB + normalizedLevel * (mHighRangeDB - mLowRangeDB);

    for (const auto& range : mLEDRanges)
    {
      if (dB >= range.lowRangeDB && dB <= range.highRangeDB)
      {
        return range.color;
      }
    }

    return mLEDRanges.empty() ? COLOR_GREEN : mLEDRanges.back().color;
  }

  /** Get interpolated color from the bar pattern at a normalized position (0-1) */
  IColor GetPatternColorAtPosition(float pos)
  {
    pos = Clip(pos, 0.f, 1.f);
    const int nStops = mBarPattern.NStops();

    if (nStops == 0)
      return COLOR_GREEN;
    if (nStops == 1)
      return mBarPattern.GetStop(0).mColor;

    // Find the two stops to interpolate between
    const IColorStop* prevStop = &mBarPattern.GetStop(0);
    const IColorStop* nextStop = &mBarPattern.GetStop(nStops - 1);

    for (int i = 0; i < nStops - 1; i++)
    {
      const IColorStop& s1 = mBarPattern.GetStop(i);
      const IColorStop& s2 = mBarPattern.GetStop(i + 1);
      if (pos >= s1.mOffset && pos <= s2.mOffset)
      {
        prevStop = &s1;
        nextStop = &s2;
        break;
      }
    }

    // Interpolate
    const float range = nextStop->mOffset - prevStop->mOffset;
    const float t = (range > 0.f) ? (pos - prevStop->mOffset) / range : 0.f;

    return IColor::LinearInterpolateBetween(prevStop->mColor, nextStop->mColor, t);
  }

  void ProcessSpectrumData(const ISenderData<MAXNC, TDataPacket>& d)
  {
    const float rangeDB = mHighRangeDB - mLowRangeDB;
    const float lowPointAbs = std::fabs(mLowRangeDB);
    const int numBins = mFFTSize / 2;

    if (mChannelMode == EChannelMode::SideBySide || mChannelMode == EChannelMode::Split)
    {
      // Process L and R channels separately
      for (int bar = 0; bar < mNBars; bar++)
      {
        const int startBin = mBandStartBins[bar];
        const int endBin = mBandEndBins[bar];
        const float freqNorm = static_cast<float>(startBin + endBin) / (2.f * numBins);

        for (int ch = 0; ch < 2; ch++)
        {
          if (d.nChans <= 0) break; // Protect against invalid channel count
          const int channelIdx = d.chanOffset + std::min(ch, d.nChans - 1);
          const int barIdx = bar * 2 + ch;

          float maxMag = 0.f;
          for (int bin = startBin; bin < endBin; bin++)
          {
            float mag = d.vals[channelIdx][bin];
            if (mag > maxMag) maxMag = mag;
          }

          UpdateBarValue(barIdx, maxMag, lowPointAbs, rangeDB, freqNorm);
        }
      }
    }
    else
    {
      // Left, Right, or Sum mode
      for (int bar = 0; bar < mNBars; bar++)
      {
        const int startBin = mBandStartBins[bar];
        const int endBin = mBandEndBins[bar];
        const float freqNorm = static_cast<float>(startBin + endBin) / (2.f * numBins);

        float maxMag = 0.f;
        for (int bin = startBin; bin < endBin; bin++)
        {
          float mag = 0.f;

          if (mChannelMode == EChannelMode::Left)
          {
            mag = d.vals[d.chanOffset][bin];
          }
          else if (mChannelMode == EChannelMode::Right)
          {
            const int rightCh = d.chanOffset + std::min(1, d.nChans - 1);
            mag = d.vals[rightCh][bin];
          }
          else // Sum
          {
            for (int ch = d.chanOffset; ch < d.chanOffset + d.nChans; ch++)
            {
              mag += d.vals[ch][bin];
            }
            mag /= d.nChans;
          }

          if (mag > maxMag) maxMag = mag;
        }

        UpdateBarValue(bar, maxMag, lowPointAbs, rangeDB, freqNorm);
      }
    }

    SetDirty(false);
  }

  void UpdateBarValue(int barIdx, float maxMag, float lowPointAbs, float rangeDB, float freqNorm = 0.5f)
  {
    // Apply octave gain compensation (boost higher frequencies)
    if (mOctaveGain > 0.f)
    {
      const float centerFreqNorm = 500.f / (mSampleRate * 0.5f);
      if (centerFreqNorm > 1e-6f) // Protect against division by zero
      {
        maxMag *= (freqNorm / centerFreqNorm) * mOctaveGain;
      }
    }

    // Convert to dB and normalize
    float dB = AmpToDB(maxMag + 1e-30f);
    float normalizedValue = (dB + lowPointAbs) / rangeDB;
    normalizedValue = Clip(normalizedValue, 0.f, 1.f);

    // Apply smoothing
    float prevValue = mBarValues[barIdx];
    float newValue;
    if (normalizedValue > prevValue)
      newValue = mAttackCoeff * prevValue + (1.f - mAttackCoeff) * normalizedValue;
    else
      newValue = mReleaseCoeff * prevValue + (1.f - mReleaseCoeff) * normalizedValue;

    mBarValues[barIdx] = newValue;

    // Peak hold
    if (mShowPeaks)
    {
      if (newValue > mPeakValues[barIdx])
      {
        mPeakValues[barIdx] = newValue;
        mPeakHoldCounters[barIdx] = mPeakHoldTimeMs;
      }
      else if (mPeakHoldCounters[barIdx] > 0)
      {
        const int hopSize = mFFTSize / std::max(1, mOverlap);
        mPeakHoldCounters[barIdx] -= static_cast<int>(1000.f * hopSize / mSampleRate);
      }
      else
      {
        mPeakValues[barIdx] *= kPeakDecayRate;
      }
    }
  }

  void CalculateFrequencyBands()
  {
    const int numBins = mFFTSize / 2;
    const float nyquist = static_cast<float>(mSampleRate) * 0.5f;

    if (mFreqScale == EFrequencyScale::ThirdOctave)
    {
      // 1/3 octave bandwidth factor: 2^(1/6) ≈ 1.1225
      const float bandwidthFactor = std::pow(2.f, 1.f / 6.f);

      // Find valid bands: must fit within nyquist AND have at least 1 unique bin
      // We track the previous end bin to avoid duplicate bin ranges
      std::vector<int> validIndices;
      std::vector<int> startBins;
      std::vector<int> endBins;
      int prevEndBin = -1;

      for (int i = 0; i < kNumThirdOctaveBands; i++)
      {
        const float centerFreq = kThirdOctaveCenterFreqs[i];
        if (centerFreq >= nyquist)
          break;

        const float lowFreq = centerFreq / bandwidthFactor;
        const float highFreq = std::min(centerFreq * bandwidthFactor, nyquist);

        int startBin = std::max(1, static_cast<int>(lowFreq / nyquist * numBins));
        int endBin = static_cast<int>(highFreq / nyquist * numBins);

        // Skip bands that don't have at least one bin beyond the previous band
        // This ensures each displayed band represents unique frequency content
        if (endBin > prevEndBin && endBin > startBin)
        {
          // Adjust startBin to avoid overlap with previous band
          if (startBin <= prevEndBin)
            startBin = prevEndBin + 1;

          if (endBin > startBin)
          {
            validIndices.push_back(i);
            startBins.push_back(startBin);
            endBins.push_back(endBin);
            prevEndBin = endBin;
          }
        }
      }

      mNBars = static_cast<int>(validIndices.size());
      ResizeBarArrays();
      mBandStartBins.resize(mNBars);
      mBandEndBins.resize(mNBars);
      mThirdOctaveIndices = validIndices;

      for (int bar = 0; bar < mNBars; bar++)
      {
        mBandStartBins[bar] = startBins[bar];
        mBandEndBins[bar] = endBins[bar];
      }
    }
    else if (mFreqScale == EFrequencyScale::Log)
    {
      mBandStartBins.resize(mNBars);
      mBandEndBins.resize(mNBars);

      const float minFreq = 20.f;
      const float maxFreq = nyquist;
      const float logMin = std::log(minFreq);
      const float logMax = std::log(maxFreq);
      const float logRange = logMax - logMin;

      for (int bar = 0; bar < mNBars; bar++)
      {
        const float logFreqStart = logMin + (static_cast<float>(bar) / mNBars) * logRange;
        const float logFreqEnd = logMin + (static_cast<float>(bar + 1) / mNBars) * logRange;
        const float freqStart = std::exp(logFreqStart);
        const float freqEnd = std::exp(logFreqEnd);

        mBandStartBins[bar] = std::max(1, static_cast<int>(freqStart / nyquist * numBins));
        mBandEndBins[bar] = std::max(mBandStartBins[bar] + 1, static_cast<int>(freqEnd / nyquist * numBins));
      }
    }
    else // Linear
    {
      mBandStartBins.resize(mNBars);
      mBandEndBins.resize(mNBars);

      const float binWidth = static_cast<float>(numBins) / mNBars;
      for (int bar = 0; bar < mNBars; bar++)
      {
        mBandStartBins[bar] = static_cast<int>(bar * binWidth);
        mBandEndBins[bar] = static_cast<int>((bar + 1) * binWidth);
      }
    }
  }

  void SetSmoothing(float attackTimeMs, float releaseTimeMs)
  {
    mAttackTimeMs = attackTimeMs;
    mDecayTimeMs = releaseTimeMs;
    float attackTimeSec = attackTimeMs * 0.001f;
    float releaseTimeSec = releaseTimeMs * 0.001f;
    float updatePeriod = static_cast<float>(mFFTSize) / static_cast<float>(mSampleRate);
    mAttackCoeff = std::exp(-updatePeriod / attackTimeSec);
    mReleaseCoeff = std::exp(-updatePeriod / releaseTimeSec);
  }

  int mNBars = 32;
  int mStoredNBars = 32; // Stores bar count when switching to ThirdOctave mode
  int mNSegsPerBar = 16;
  int mFFTSize = 1024;
  int mOverlap = 1;
  int mWindowType = 0;
  double mSampleRate = 44100.0;
  float mOctaveGain = 0.f;
  EFrequencyScale mFreqScale = EFrequencyScale::Log;
  EColorMode mColorMode = EColorMode::Segments;
  EChannelMode mChannelMode = EChannelMode::Sum;
  IPattern mBarPattern = IPattern(COLOR_GREEN);
  std::vector<SpectrumLEDRange> mLEDRanges;

  float mGapRatio = 0.2f;
  float mSegGapRatio = 0.1f;
  float mLowRangeDB = -72.f;
  float mHighRangeDB = 6.f;
  float mAttackTimeMs = 5.f;
  float mDecayTimeMs = 50.f;
  float mAttackCoeff = 0.9f;
  float mReleaseCoeff = 0.99f;

  bool mShowPeaks = true;
  int mPeakHoldTimeMs = 1000;

  bool mShowClipIndicator = false;
  float mClipThresholdDB = 0.f;
  IColor mClipColor = COLOR_RED;

  std::vector<float> mBarValues;
  std::vector<float> mPeakValues;
  std::vector<int> mPeakHoldCounters;
  std::vector<int> mBandStartBins;
  std::vector<int> mBandEndBins;
  std::vector<int> mThirdOctaveIndices; // Maps displayed bars to kThirdOctaveCenterFreqs indices
  IPopupMenu mMenu {"Options"};
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
