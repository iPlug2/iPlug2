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
 * @copydoc ISpectrogramControl
 */

#include <vector>
#include <array>
#include <cmath>
#include <algorithm>

#include "IControl.h"
#include "ISender.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** ISpectrogramControl displays a scrolling spectrogram visualization.
 *
 * This control receives FFT data via ISender and displays it as a
 * color-mapped spectrogram that scrolls in real-time.
 *
 * Features:
 * - Multiple frequency scales (Linear, Log, Mel)
 * - Four colormaps (Jet, Hot, Cool, Greyscale)
 * - Smooth scrolling with clock drift correction
 * - Adjustable FFT size and overlap
 * - Range and contrast controls
 * - Uses IGraphics layer abstraction for backend independence
 *
 * @tparam MAXNC Maximum number of channels (default 1)
 * @ingroup IControls
 */
template <int MAXNC = 1>
class ISpectrogramControl : public IControl
{
public:
  static constexpr int MAX_FFT_SIZE = 4096;

  /** Frequency scale options for spectrogram display */
  enum class EFrequencyScale
  {
    Linear = 0,
    Log,
    Mel
  };

  /** Convert frequency in Hz to Mel scale */
  static inline float HzToMel(float hz)
  {
    return 2595.0f * std::log10(1.0f + hz / 700.0f);
  }

  /** Convert Mel scale to frequency in Hz */
  static inline float MelToHz(float mel)
  {
    return 700.0f * (std::pow(10.0f, mel / 2595.0f) - 1.0f);
  }

  /** Colormap options */
  enum class EColorMap
  {
    Jet = 0,
    Hot,
    Cool,
    GreyScale,
    NumColorMaps
  };

  /** Get colormap name as string */
  inline const char* ColorMapName(EColorMap colorMap)
  {
    switch (colorMap)
    {
      case EColorMap::Jet:       return "Jet";
      case EColorMap::Hot:       return "Hot";
      case EColorMap::Cool:      return "Cool";
      case EColorMap::GreyScale: return "Greyscale";
      default:                   return "Jet";
    }
  }

  /** Apply MATLAB colormap to normalized value (0-1) */
  inline IColor ApplyColorMap(float x, EColorMap colorMap)
  {
    x = std::clamp(x, 0.0f, 1.0f);
    float r = 0.0f, g = 0.0f, b = 0.0f;

    switch (colorMap)
    {
      case EColorMap::Jet:
      {
        if (x < 0.7f)
          r = 4.0f * x - 1.5f;
        else
          r = -4.0f * x + 4.5f;

        if (x < 0.5f)
          g = 4.0f * x - 0.5f;
        else
          g = -4.0f * x + 3.5f;

        if (x < 0.3f)
          b = 4.0f * x + 0.5f;
        else
          b = -4.0f * x + 2.5f;
        break;
      }
      case EColorMap::Hot:
      {
        r = 8.0f / 3.0f * x;
        g = 8.0f / 3.0f * x - 1.0f;
        b = 4.0f * x - 3.0f;
        break;
      }
      case EColorMap::Cool:
      {
        r = x;
        g = 1.0f - x;
        b = 1.0f;
        break;
      }
      case EColorMap::GreyScale:
      {
        r = x;
        g = x;
        b = x;
        break;
      }
      default:
        break;
    }

    r = std::clamp(r, 0.0f, 1.0f);
    g = std::clamp(g, 0.0f, 1.0f);
    b = std::clamp(b, 0.0f, 1.0f);

    return IColor(255,
                  static_cast<int>(r * 255.0f),
                  static_cast<int>(g * 255.0f),
                  static_cast<int>(b * 255.0f));
  }


  enum MsgTags
  {
    kMsgTagSampleRate = 1,
    kMsgTagFFTSize,
    kMsgTagOverlap,
    kMsgTagWindowType,
    kMsgTagScrollEnabled,
    kMsgTagColorMapRange,
    kMsgTagColorMapContrast
  };

  using TDataPacket = std::array<float, MAX_FFT_SIZE>;

  /** Constructor
   * @param bounds Control bounds
   * @param fftSize FFT size (default 1024)
   * @param overlap Overlap factor (default 2)
   * @param numRows Number of spectrogram rows (default 128)
   * @param direction Scroll direction
   * @param freqScale Frequency scale type */
  ISpectrogramControl(const IRECT& bounds, int fftSize = 1024,
                      int overlap = 2, int numRows = 128,
                      EDirection direction = EDirection::Horizontal,
                      EFrequencyScale freqScale = EFrequencyScale::Mel)
  : IControl(bounds)
  , mFFTSize(fftSize)
  , mOverlap(overlap)
  , mNumRows(numRows)
  , mDirection(direction)
  , mFreqScale(freqScale)
  {
    SetFreqRange(NyquistFreq() / mFFTSize, NyquistFreq());
    SetDBRange(-90.0f, 0.0f);

    mTransport.SetSampleRate(mSampleRate);
    mTransport.SetOverlap(mOverlap);

    IText txt(DEFAULT_TEXT_SIZE, IColor(255, 255, 255, 255));
    SetText(txt);

    CheckSpectrogramDataSize();
  }

  ~ISpectrogramControl()
  {
    ReleaseBitmapResources();
  }

  void OnAttached() override
  {
    IControl::OnAttached();
    mImageNeedsCreate = true;
  }

  void Draw(IGraphics& g) override
  {
    if (mScrollSpeed != mQueuedScrollSpeed)
    {
      mScrollSpeed = mQueuedScrollSpeed;
      CheckSpectrogramDataSize();
      mTransport.Reset();
    }

    g.FillRect(COLOR_BLACK, mRECT);
    DrawSpectrogram(g);
    PostDraw(g);
  }

  void OnResize() override
  {
    CheckSpectrogramDataSize();
    IControl::OnResize();
  }

  void OnRescale() override
  {
    CheckSpectrogramDataSize();
    IControl::OnRescale();
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    mMenu.Clear(true);
    mMenu.AddItem("Scale", new IPopupMenu("Scale", {"Linear", "Log", "Mel"}))
      ->GetSubmenu()->CheckItemAlone(int(mFreqScale));
    mMenu.AddItem("Direction", new IPopupMenu("Direction", {"Horizontal", "Vertical"}))
      ->GetSubmenu()->CheckItemAlone(mDirection == EDirection::Horizontal ? 0 : 1);
    mMenu.AddItem("ColorMap", new IPopupMenu("ColorMap", {"Jet", "Hot", "Cool", "Greyscale"}))
      ->GetSubmenu()->CheckItemAlone(static_cast<int>(mColorMap));
    mMenu.AddItem("FFTSize", new IPopupMenu("FFTSize", {"64", "128", "256", "512", "1024", "2048", "4096"}))
      ->GetSubmenu()->CheckItemWithText(std::to_string(mFFTSize).c_str());
    mMenu.AddItem("Overlap", new IPopupMenu("Overlap", {"2", "4"}))
      ->GetSubmenu()->CheckItemAlone(mOverlap == 2 ? 0 : 1);
    mMenu.AddItem("ScrollSpeed", new IPopupMenu("ScrollSpeed", {"1", "2", "4", "8"}))
      ->GetSubmenu()->CheckItemWithText(std::to_string(mScrollSpeed).c_str());

    GetUI()->CreatePopupMenu(*this, mMenu, x, y);
  }

  void OnMouseOver(float x, float y, const IMouseMod& mod) override
  {
    const auto xNorm = (mDirection == EDirection::Horizontal) ? (1.0 - y / mRECT.H()) : x / mRECT.W();
    mCursorFreq = CalcXNorm(xNorm, mFreqScale, mSampleRate) * NyquistFreq();
  }

  void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int valIdx) override
  {
    if (pSelectedMenu)
    {
      const char* title = pSelectedMenu->GetRootTitle();

      if (strcmp(title, "Scale") == 0)
        SetFreqScale(EFrequencyScale(pSelectedMenu->GetChosenItemIdx()));
      else if (strcmp(title, "Direction") == 0)
        SetDirection(pSelectedMenu->GetChosenItemIdx() == 0 ? EDirection::Horizontal : EDirection::Vertical);
      else if (strcmp(title, "ColorMap") == 0)
        SetColorMap(static_cast<EColorMap>(pSelectedMenu->GetChosenItemIdx()));
      else if (strcmp(title, "FFTSize") == 0)
      {
        int fftSize = atoi(pSelectedMenu->GetChosenItem()->GetText());
        GetDelegate()->SendArbitraryMsgFromUI(kMsgTagFFTSize, kNoTag, sizeof(int), &fftSize);
        SetFFTSize(fftSize);
      }
      else if (strcmp(title, "Overlap") == 0)
      {
        int overlap = atoi(pSelectedMenu->GetChosenItem()->GetText());
        GetDelegate()->SendArbitraryMsgFromUI(kMsgTagOverlap, kNoTag, sizeof(int), &overlap);
        SetOverlap(overlap);
      }
      else if (strcmp(title, "ScrollSpeed") == 0)
        SetScrollSpeed(atoi(pSelectedMenu->GetChosenItem()->GetText()));
    }
  }

  bool IsDirty() override
  {
    return true; // Continuous refresh for smooth scrolling
  }

  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    IByteStream stream(pData, dataSize);

    if (!IsDisabled() && msgTag == ISender<>::kUpdateMessage)
    {
      if (!mScrollEnabled)
        return;

      ISenderData<MAXNC, TDataPacket> d;
      stream.Get(&d, 0);

      // Sum channels (or use first channel if mono)
      TDataPacket summedSpectrum{};
      int numChans = std::min(d.nChans, MAXNC);

      for (auto c = d.chanOffset; c < (d.chanOffset + numChans); c++)
      {
        for (int i = 0; i < MAX_FFT_SIZE; i++)
          summedSpectrum[i] += d.vals[c - d.chanOffset][i];
      }

      // Average if stereo
      if (numChans > 1)
      {
        for (int i = 0; i < MAX_FFT_SIZE; i++)
          summedSpectrum[i] /= static_cast<float>(numChans);
      }

      ScaleSpectrogramData(0, summedSpectrum);
      UpdateSpectrogram(0, summedSpectrum);
      mTransport.OnNewData(mFFTSize);

      SetDirty(false);
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
      SetOverlap(overlap);
    }
    else if (msgTag == kMsgTagScrollEnabled)
    {
      bool scrollEnabled;
      stream.Get(&scrollEnabled, 0);
      SetScrollEnabled(scrollEnabled);
    }
    else if (msgTag == kMsgTagColorMapRange)
    {
      double range;
      stream.Get(&range, 0);
      SetColorMapRange(range);
    }
    else if (msgTag == kMsgTagColorMapContrast)
    {
      double contrast;
      stream.Get(&contrast, 0);
      SetColorMapContrast(contrast);
    }
  }

  // Setters
  void SetFFTSize(int fftSize)
  {
    assert(fftSize > 0 && fftSize <= MAX_FFT_SIZE);
    mFFTSize = fftSize;
    CheckSpectrogramDataSize();
    ResetSpectrogramData();
  }

  void SetOverlap(int overlap)
  {
    assert(overlap > 0);
    mOverlap = overlap;
    mTransport.SetOverlap(mOverlap);
    CheckSpectrogramDataSize();
  }

  void SetSampleRate(double sampleRate)
  {
    mSampleRate = sampleRate;
    mTransport.SetSampleRate(sampleRate);
    SetFreqRange(NyquistFreq() / mFFTSize, NyquistFreq());
    ResetSpectrogramData();
  }

  void SetFreqRange(float freqLo, float freqHi)
  {
    mFreqLo = freqLo;
    mFreqHi = freqHi;
  }

  void SetFreqScale(EFrequencyScale freqScale) { mFreqScale = freqScale; }
  void SetDirection(EDirection direction) { mDirection = direction; }

  void SetDBRange(float dbLo, float dbHi)
  {
    mAmpLo = std::log(std::pow(10.f, dbLo / 10.0));
    mAmpHi = std::log(std::pow(10.f, dbHi / 10.0));
  }

  void SetScrollEnabled(bool enabled)
  {
    if (mScrollEnabled && !enabled)
      mTransport.Reset();
    mScrollEnabled = enabled;
    if (!mScrollEnabled)
      mTransport.Reset();
  }

  void SetScrollSpeed(int scrollSpeed) { mQueuedScrollSpeed = scrollSpeed; }

  void SetColorMap(EColorMap colormap)
  {
    mColorMap = colormap;
    // Rerender all existing data with new colormap
    RecolorAllPixels();
  }

  void SetColorMapRange(float range) { mColorMapRange = range; }
  void SetColorMapContrast(float contrast) { mColorMapContrast = contrast; }

private:
  /** ITransport is a utility class which manages real elapsed time and theoric time based on amount of incoming data received */
  class ITransport
  {
  public:
    using clock = std::chrono::high_resolution_clock;

    void Reset()
    {
      mStartTime = clock::now();
      mStartTimeDiff = mStartTime;
    }

    void SetSampleRate(double sampleRate)
    {
      mSampleRate = sampleRate;
    }

    void SetOverlap(int overlap)
    {
      mOverlap = overlap;
    }

    /** Compute time duration in ms for a given data size */
    double ComputeDataDuration(int dataSize) const
    {
      return (1000.0 * dataSize) / mSampleRate;
    }

    void OnNewData(int dataSize)
    {
      dataSize *= mOverlap;

      if (mFirstDataReceived)
      {
        mNumSamplesProcessed += dataSize;
      }
      else
      {
        mFirstDataReceived = true;
        Reset();
      }
    }

    double GetElapsedTime() const
    {
      if (!mFirstDataReceived)
        return 0.0;

      auto now = clock::now();
      return (now - mStartTime).count();
    }

    /** Compute time difference in milliseconds between the real elapsed
        time and the elapsed time computed from incoming data */
    double ComputeTimeDifference(double maxTimeDiff)
    {
      if (!mFirstDataReceived)
        return 0.0;

      auto now = clock::now();
      double realDuration = (now - mStartTimeDiff).count();
      double dataDuration = (1000.0 * mNumSamplesProcessed)/(mSampleRate*mOverlap);

      // Reset offset if too much drift
      double diff = realDuration - dataDuration;

      if (std::fabs(diff) > maxTimeDiff)
      {
        diff = 0.0;

        // Reset
        mStartTimeDiff = now;
        mNumSamplesProcessed = 0;
      }

      return diff;
    }

   private:
    double mSampleRate = 44100.0;
    int mOverlap = 2;

    bool mFirstDataReceived = false;
    TimePoint mStartTime;
    TimePoint mStartTimeDiff;
    long long mNumSamplesProcessed = 0;
  };


  void PostDraw(IGraphics& g)
  {
  }

  void ReleaseBitmapResources()
  {
    if (mBitmapValid && GetUI())
    {
      GetUI()->ReleaseDynamicBitmap(mBitmap);
      mBitmapValid = false;
    }
  }

  void DrawSpectrogram(IGraphics& g)
  {
    int numRows = NumRows();
    int numBins = NumBins();

    // Update bitmap if dimensions changed or needs creation
    if (mImageNeedsCreate || mImageWidth != numBins || mImageHeight != numRows)
    {
      ReleaseBitmapResources();
      mBitmap = g.CreateBitmapFromRGBA(mPixelBuffer.data(), numBins, numRows);
      mBitmapValid = true;
      mImageWidth = numBins;
      mImageHeight = numRows;
      mImageNeedsCreate = false;
      mImageNeedsUpdate = false;
    }
    else if (mImageNeedsUpdate)
    {
      g.UpdateBitmapRGBA(mBitmap, mPixelBuffer.data());
      mImageNeedsUpdate = false;
    }

    if (!mBitmapValid)
      return;

    // Calculate the normalized write position (0 to 1)
    float writePos = static_cast<float>(mWriteIndex % numRows) / static_cast<float>(numRows);

    // Apply smooth scroll offset for sub-frame interpolation
    if (mScrollEnabled)
    {
      double spectrogramDuration = mTransport.ComputeDataDuration(numRows * mFFTSize);
      double driftThreshold = mTransport.ComputeDataDuration(mMaxDriftRows * mFFTSize);
      double timeDiff = mTransport.ComputeTimeDifference(driftThreshold);
      float scrollOffset = static_cast<float>(timeDiff / spectrogramDuration);

      writePos += scrollOffset;
      if (writePos >= 1.0f) writePos -= 1.0f;
      if (writePos < 0.0f) writePos += 1.0f;
    }

    float oldPartFrac = 1.0f - writePos;
    float newPartFrac = writePos;

    float imgW = static_cast<float>(numBins);
    float imgH = static_cast<float>(numRows);

    if (mDirection == EDirection::Horizontal)
    {
      // Horizontal mode: time flows left to right, frequency is vertical
      // Image needs to be rotated -90 degrees
      if (oldPartFrac > 0.001f)
      {
        IRECT dstRect = mRECT.GetFromLeft(mRECT.W() * oldPartFrac);
        IRECT srcRect(0.f, writePos * imgH, imgW, imgH);
        DrawImagePortionRotated(g, dstRect, srcRect);
      }
      if (newPartFrac > 0.001f)
      {
        IRECT dstRect = mRECT.GetFromRight(mRECT.W() * newPartFrac);
        IRECT srcRect(0.f, 0.f, imgW, newPartFrac * imgH);
        DrawImagePortionRotated(g, dstRect, srcRect);
      }
    }
    else
    {
      // Vertical mode: time flows bottom to top, frequency is horizontal
      // Image needs to be flipped vertically
      if (oldPartFrac > 0.001f)
      {
        IRECT dstRect = mRECT.GetFromBottom(mRECT.H() * oldPartFrac);
        IRECT srcRect(0.f, writePos * imgH, imgW, imgH);
        DrawImagePortionFlipped(g, dstRect, srcRect);
      }
      if (newPartFrac > 0.001f)
      {
        IRECT dstRect = mRECT.GetFromTop(mRECT.H() * newPartFrac);
        IRECT srcRect(0.f, 0.f, imgW, newPartFrac * imgH);
        DrawImagePortionFlipped(g, dstRect, srcRect);
      }
    }
  }

  void DrawImagePortionRotated(IGraphics& g, const IRECT& dst, const IRECT& src)
  {
    // Draw source region rotated -90 degrees to destination
    g.PathTransformSave();

    float cx = dst.L + dst.W() * 0.5f;
    float cy = dst.T + dst.H() * 0.5f;
    g.PathTransformTranslate(cx, cy);
    g.PathTransformRotate(-90.f);

    // After rotation, the dimensions are swapped
    float rotW = dst.H();
    float rotH = dst.W();
    IRECT rotDst(-rotW * 0.5f, -rotH * 0.5f, rotW * 0.5f, rotH * 0.5f);

    g.DrawBitmapRegion(mBitmap, src, rotDst);

    g.PathTransformRestore();
  }

  void DrawImagePortionFlipped(IGraphics& g, const IRECT& dst, const IRECT& src)
  {
    // Draw source region flipped vertically to destination
    g.PathTransformSave();

    float cy = dst.T + dst.H() * 0.5f;
    g.PathTransformTranslate(0.f, cy);
    g.PathTransformScale(1.f, -1.f);
    g.PathTransformTranslate(0.f, -cy);

    g.DrawBitmapRegion(mBitmap, src, dst);

    g.PathTransformRestore();
  }

  void ScaleSpectrogramData(int ch, TDataPacket& powerSpectrum)
  {
    auto numBins = NumBins();

    if (mTmpBuf.size() != static_cast<size_t>(numBins))
      mTmpBuf.resize(numBins);

    std::copy(powerSpectrum.begin(), powerSpectrum.begin() + numBins, mTmpBuf.begin());

    float xRecip = 1.0f / float(numBins - 1);
    for (auto i = 1; i < numBins; i++)
    {
      float x = CalcXNorm(i * xRecip, mFreqScale, mSampleRate) * float(numBins - 1);
      int id0 = static_cast<int>(x);
      int id1 = std::min(id0 + 1, numBins - 1);
      float t = x - id0;
      powerSpectrum[i] = (1.0f - t) * mTmpBuf[id0] + t * mTmpBuf[id1];
    }

    for (auto i = 0; i < numBins; i++)
      powerSpectrum[i] = CalcYNorm(powerSpectrum[i]);
  }

  void UpdateSpectrogram(int ch, TDataPacket& powerSpectrum)
  {
    int numRows = NumRows();
    int numBins = NumBins();
    int rowIndex = mWriteIndex++ % numRows;

    // Copy normalized magnitude data to internal buffer
    std::copy(powerSpectrum.begin(), powerSpectrum.begin() + numBins,
              mMagnitudeBuffer.begin() + rowIndex * numBins);

    // Convert this row to RGBA pixels using colormap
    WriteRowToPixels(rowIndex);

    mImageNeedsUpdate = true;
  }

  void WriteRowToPixels(int rowIndex)
  {
    int numBins = NumBins();

    // Pixel buffer layout: RGBA, row-major (each row is a time slice)
    // Row y, column x = pixel at (y * numBins + x) * 4
    uint8_t* rowPixels = mPixelBuffer.data() + rowIndex * numBins * 4;
    const float* rowMagnitude = mMagnitudeBuffer.data() + rowIndex * numBins;

    for (int x = 0; x < numBins; x++)
    {
      float value = rowMagnitude[x];

      // Apply contrast and range adjustment
      value = ContrastRange(value, mColorMapContrast, mColorMapRange);
      value = std::clamp(value, 0.0f, 1.0f);

      // CPU colormap lookup
      IColor color = ApplyColorMap(value, mColorMap);

      rowPixels[x * 4 + 0] = color.R;
      rowPixels[x * 4 + 1] = color.G;
      rowPixels[x * 4 + 2] = color.B;
      rowPixels[x * 4 + 3] = 255;
    }
  }

  void RecolorAllPixels()
  {
    int numRows = NumRows();
    for (int row = 0; row < numRows; row++)
      WriteRowToPixels(row);
    mImageNeedsUpdate = true;
  }

  float ContrastRange(float t, float contrast, float range) const
  {
    // Schlick sigmoid for contrast/range adjustment
    t *= range * 2.0f;
    t = std::clamp(t, 0.0f, 1.0f);

    float a = contrast;
    float f0 = (1.0f / (1.0f - a) - 2.0f) * (1.0f - 2.0f * t);
    float ga;
    if (t < 0.5f)
      ga = t / (f0 + 1.0f);
    else
      ga = (f0 - t) / (f0 - 1.0f);
    return ga;
  }

  void CheckSpectrogramDataSize()
  {
    int numRows = NumRows();
    int numBins = NumBins();
    size_t magnitudeSize = static_cast<size_t>(numRows * numBins);
    size_t pixelSize = magnitudeSize * 4; // RGBA

    // Check if dimensions changed (not just total size)
    if (mMagnitudeBuffer.size() != magnitudeSize ||
        mImageWidth != numBins || mImageHeight != numRows)
    {
      // Use assign to fully reinitialize (resize preserves old data)
      mMagnitudeBuffer.assign(magnitudeSize, 0.0f);
      mPixelBuffer.assign(pixelSize, 0);
      mImageWidth = numBins;
      mImageHeight = numRows;
      ResetSpectrogramData();
    }
  }

  void ResetSpectrogramData()
  {
    std::fill(mMagnitudeBuffer.begin(), mMagnitudeBuffer.end(), 0.0f);

    // Initialize pixel buffer with colormap value for 0 (dark blue for jet)
    IColor initColor = ApplyColorMap(0.0f, mColorMap);
    size_t numPixels = mPixelBuffer.size() / 4;
    for (size_t i = 0; i < numPixels; i++)
    {
      mPixelBuffer[i * 4 + 0] = initColor.R;
      mPixelBuffer[i * 4 + 1] = initColor.G;
      mPixelBuffer[i * 4 + 2] = initColor.B;
      mPixelBuffer[i * 4 + 3] = 255;
    }

    mWriteIndex = 0;
    mImageNeedsCreate = true;
  }

  float CalcXNorm(float x, EFrequencyScale scale, float sampleRate) const
  {
    const auto nyquist = NyquistFreq();

    switch (scale)
    {
      case EFrequencyScale::Linear:
        return (mFreqLo + x * (mFreqHi - mFreqLo)) / nyquist;
      case EFrequencyScale::Log:
      {
        const auto logXLo = std::log(mFreqLo / nyquist);
        const auto logXHi = std::log(mFreqHi / nyquist);
        return std::exp(logXLo + x * (logXHi - logXLo));
      }
      case EFrequencyScale::Mel:
      {
        const auto melXLo = HzToMel(mFreqLo);
        const auto melXHi = HzToMel(mFreqHi);
        return (1.0f / nyquist) * MelToHz(melXLo + x * (melXHi - melXLo));
      }
    }
    return x;
  }

  float CalcYNorm(float y) const
  {
    return (std::log(y) - mAmpLo) / (mAmpHi - mAmpLo);
  }

  int NumRows() const
  {
    // Fixed number of rows - visual resolution stays constant
    // Scroll speed varies naturally with FFT size (high FFT = slower scroll)
    float coeff = mSampleRate / 44100.0;
    int numRows = static_cast<int>(std::round(coeff * mNumRows));
    int scrollSpeedCoeff = static_cast<int>(8.0 / mScrollSpeed);
    return numRows * scrollSpeedCoeff;
  }

  float NyquistFreq() const { return static_cast<float>(mSampleRate * 0.5); }
  int NumBins() const { return mFFTSize / 2; }

  double mSampleRate = 44100.0;
  int mFFTSize;
  int mOverlap;
  int mNumRows;
  int mMaxDriftRows = 4;

  float mFreqLo = 20.0f;
  float mFreqHi = 22050.0f;
  float mAmpLo = 0.0f;
  float mAmpHi = 1.0f;

  ITransport mTransport;
  EColorMap mColorMap = EColorMap::Jet;

  bool mScrollEnabled = true;
  float mColorMapRange = 0.5f;
  float mColorMapContrast = 0.5f;

  // Magnitude buffer (normalized 0-1 values)
  std::vector<float> mMagnitudeBuffer;
  // RGBA pixel buffer for rendering
  std::vector<uint8_t> mPixelBuffer;
  int mWriteIndex = 0;

  EDirection mDirection;
  int mScrollSpeed = 8;
  int mQueuedScrollSpeed = 8;
  EFrequencyScale mFreqScale;

  float mCursorFreq = -1.0f;

  std::vector<float> mTmpBuf;
  IPopupMenu mMenu{"Options"};

  int mImageWidth = 0;
  int mImageHeight = 0;
  bool mImageNeedsCreate = true;
  bool mImageNeedsUpdate = false;

  IBitmap mBitmap;
  bool mBitmapValid = false;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
