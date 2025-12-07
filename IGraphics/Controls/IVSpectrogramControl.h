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
 * @copydoc IVSpectrogramControl
 */

#include "IControl.h"
#include "ISender.h"
#include "IPlugStructs.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** Vectorial spectrogram (waterfall) display control
 * @ingroup IControls
 * Displays a rolling time-frequency visualization with customizable colormaps
 */
template <int MAXNC = 2, int MAX_FFT_SIZE = 4096>
class IVSpectrogramControl : public IControl
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
    kMsgTagWindowType
  };

  /** Available colormaps for amplitude visualization */
  enum class EColormap
  {
    Viridis,
    Magma,
    Plasma,
    Inferno,
    Grayscale,
    Turbo,
    Hot
  };

  enum class EFrequencyScale { Linear, Log };
  enum class EScrollDirection { RightToLeft, LeftToRight, TopToBottom, BottomToTop };
  enum class EChannelMode { Left, Right, Average, Max };

  /** Create a spectrogram control
   * @param bounds The rectangular area that the control occupies
   * @param label A CString to label the control
   * @param style IVStyle for the control
   * @param colormap Initial colormap to use
   * @param freqScale Linear or logarithmic frequency distribution
   * @param scrollDir Direction of time scrolling
   * @param minDB Minimum dB value (maps to darkest color)
   * @param maxDB Maximum dB value (maps to brightest color)
   * @param gamma Contrast curve exponent (1.0 = linear)
   * @param numColumns Number of time columns in the spectrogram history */
  IVSpectrogramControl(const IRECT& bounds,
                       const char* label = "",
                       const IVStyle& style = DEFAULT_STYLE,
                       EColormap colormap = EColormap::Viridis,
                       EFrequencyScale freqScale = EFrequencyScale::Log,
                       EScrollDirection scrollDir = EScrollDirection::RightToLeft,
                       float minDB = -90.f,
                       float maxDB = 0.f,
                       float gamma = 1.f,
                       int numColumns = 256)
  : IControl(bounds)
  , IVectorBase(style)
  , mColormap(colormap)
  , mFreqScale(freqScale)
  , mScrollDirection(scrollDir)
  , mMinDB(minDB)
  , mMaxDB(maxDB)
  , mGamma(gamma)
  , mNumColumns(numColumns)
  {
    AttachIControl(this, label);
    BuildColorLUT();
  }

  ~IVSpectrogramControl()
  {
  }

  void OnResize() override
  {
    SetTargetRECT(MakeRects(mRECT));
    mTextureInvalid = true;
    SetDirty(false);
  }

  void OnRescale() override
  {
    mTextureInvalid = true;
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
      mSampleRate = sr;
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
  }

  void Draw(IGraphics& g) override
  {
    DrawBackground(g, mRECT);

    // Ensure texture is properly sized
    if (mTextureInvalid)
      ResizeTexture();

    DrawSpectrogram(g);
    DrawLabel(g);

    if (mStyle.drawFrame)
      g.DrawRect(GetColor(kFR), mWidgetBounds, &mBlend, mStyle.frameThickness);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if (mod.R)
    {
      mMenu.Clear(true);

      // Colormap submenu
      auto* pColormapMenu = mMenu.AddItem("Colormap", new IPopupMenu("Colormap",
        {"Viridis", "Magma", "Plasma", "Inferno", "Grayscale", "Turbo", "Hot"}))->GetSubmenu();
      pColormapMenu->CheckItem(static_cast<int>(mColormap), true);

      // Frequency scale submenu
      auto* pFreqScaleMenu = mMenu.AddItem("Freq Scale", new IPopupMenu("Freq Scale",
        {"Linear", "Log"}))->GetSubmenu();
      pFreqScaleMenu->CheckItem(mFreqScale == EFrequencyScale::Linear ? 0 : 1, true);

      // Scroll direction submenu
      auto* pScrollMenu = mMenu.AddItem("Scroll Direction", new IPopupMenu("Scroll Direction",
        {"Right to Left", "Left to Right", "Top to Bottom", "Bottom to Top"}))->GetSubmenu();
      pScrollMenu->CheckItem(static_cast<int>(mScrollDirection), true);

      // Channel mode submenu
      auto* pChanMenu = mMenu.AddItem("Channel", new IPopupMenu("Channel",
        {"Left", "Right", "Average", "Max"}))->GetSubmenu();
      pChanMenu->CheckItem(static_cast<int>(mChannelMode), true);

      // dB Range submenu
      auto* pDbRangeMenu = mMenu.AddItem("dB Range", new IPopupMenu("dB Range",
        {"-60 to 0", "-90 to 0", "-120 to 0"}))->GetSubmenu();
      int dbRangeIdx = (mMinDB == -60.f) ? 0 : (mMinDB == -90.f) ? 1 : 2;
      pDbRangeMenu->CheckItem(dbRangeIdx, true);

      // Contrast submenu
      auto* pContrastMenu = mMenu.AddItem("Contrast", new IPopupMenu("Contrast",
        {"Low (0.5)", "Normal (1.0)", "High (2.0)", "Very High (3.0)"}))->GetSubmenu();
      int contrastIdx = (mGamma <= 0.5f) ? 0 : (mGamma <= 1.0f) ? 1 : (mGamma <= 2.0f) ? 2 : 3;
      pContrastMenu->CheckItem(contrastIdx, true);

      // FFT Size submenu
      auto* pFftSizeMenu = mMenu.AddItem("FFT Size", new IPopupMenu("FFT Size",
        {"256", "512", "1024", "2048", "4096"}))->GetSubmenu();
      int fftIdx = 0;
      if (mFFTSize == 256) fftIdx = 0;
      else if (mFFTSize == 512) fftIdx = 1;
      else if (mFFTSize == 1024) fftIdx = 2;
      else if (mFFTSize == 2048) fftIdx = 3;
      else if (mFFTSize == 4096) fftIdx = 4;
      pFftSizeMenu->CheckItem(fftIdx, true);

      // Pause toggle
      mMenu.AddItem(mPaused ? "Resume" : "Pause");

      GetUI()->CreatePopupMenu(*this, mMenu, x, y);
    }
  }

  void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int valIdx) override
  {
    if (!pSelectedMenu)
      return;

    const char* title = pSelectedMenu->GetRootTitle();

    if (strcmp(title, "Colormap") == 0)
    {
      SetColormap(static_cast<EColormap>(pSelectedMenu->GetChosenItemIdx()));
    }
    else if (strcmp(title, "Freq Scale") == 0)
    {
      SetFrequencyScale(pSelectedMenu->GetChosenItemIdx() == 0
        ? EFrequencyScale::Linear : EFrequencyScale::Log);
    }
    else if (strcmp(title, "Scroll Direction") == 0)
    {
      SetScrollDirection(static_cast<EScrollDirection>(pSelectedMenu->GetChosenItemIdx()));
    }
    else if (strcmp(title, "Channel") == 0)
    {
      SetChannelMode(static_cast<EChannelMode>(pSelectedMenu->GetChosenItemIdx()));
    }
    else if (strcmp(title, "dB Range") == 0)
    {
      int idx = pSelectedMenu->GetChosenItemIdx();
      float minDb = (idx == 0) ? -60.f : (idx == 1) ? -90.f : -120.f;
      SetDBRange(minDb, 0.f);
    }
    else if (strcmp(title, "Contrast") == 0)
    {
      int idx = pSelectedMenu->GetChosenItemIdx();
      float gamma = (idx == 0) ? 0.5f : (idx == 1) ? 1.0f : (idx == 2) ? 2.0f : 3.0f;
      SetGamma(gamma);
    }
    else if (strcmp(title, "FFT Size") == 0)
    {
      int fftSize = atoi(pSelectedMenu->GetChosenItem()->GetText());
      GetDelegate()->SendArbitraryMsgFromUI(kMsgTagFFTSize, kNoTag, sizeof(int), &fftSize);
      SetFFTSize(fftSize);
    }
    else if (strcmp(title, "Options") == 0)
    {
      // Handle pause toggle from main menu
      const char* text = pSelectedMenu->GetChosenItem()->GetText();
      if (strcmp(text, "Pause") == 0 || strcmp(text, "Resume") == 0)
      {
        SetPaused(!mPaused);
      }
    }
  }

  // Configuration methods
  void SetColormap(EColormap colormap)
  {
    mColormap = colormap;
    BuildColorLUT();
    RedrawFullTexture();
    SetDirty(false);
  }

  void SetDBRange(float minDB, float maxDB)
  {
    mMinDB = minDB;
    mMaxDB = maxDB;
    RedrawFullTexture();
    SetDirty(false);
  }

  void SetGamma(float gamma)
  {
    mGamma = Clip(gamma, 0.1f, 5.f);
    BuildColorLUT();
    RedrawFullTexture();
    SetDirty(false);
  }

  void SetFrequencyScale(EFrequencyScale scale)
  {
    mFreqScale = scale;
    SetDirty(false);
  }

  void SetScrollDirection(EScrollDirection dir)
  {
    mScrollDirection = dir;
    SetDirty(false);
  }

  void SetChannelMode(EChannelMode mode)
  {
    mChannelMode = mode;
    SetDirty(false);
  }

  void SetPaused(bool paused)
  {
    mPaused = paused;
    SetDirty(false);
  }

  bool IsPaused() const { return mPaused; }

  void SetFFTSize(int fftSize)
  {
    mFFTSize = fftSize;
    mTextureInvalid = true;
    SetDirty(false);
  }

  void SetNumColumns(int numColumns)
  {
    mNumColumns = numColumns;
    mTextureInvalid = true;
    SetDirty(false);
  }

private:
  void DrawWidget(IGraphics& g) override
  {
    DrawSpectrogram(g);
  }

  void DrawSpectrogram(IGraphics& g)
  {
    if (mNumColumns <= 0 || mNumBins <= 0 || mRawBitmap.GetSize() == 0)
      return;

    DrawSpectrogramFallback(g);
  }

  void DrawSpectrogramFallback(IGraphics& g)
  {
    // Fallback FillRect-based rendering for non-NanoVG backends
    // This is slower but works everywhere
    const bool horizontal = IsHorizontalScroll();

    if (horizontal)
    {
      const float colWidth = mWidgetBounds.W() / mNumColumns;

      for (int col = 0; col < mNumColumns; col++)
      {
        int texCol;
        if (mScrollDirection == EScrollDirection::RightToLeft)
          texCol = (mWriteColumn - 1 - col + mNumColumns) % mNumColumns;
        else
          texCol = (mWriteColumn + col) % mNumColumns;

        const float x = mWidgetBounds.L + col * colWidth;

        for (int bin = 0; bin < mNumBins; bin++)
        {
          const int pixelIdx = (bin * mNumColumns + texCol) * 4;
          const IColor color(mRawBitmap.Get()[pixelIdx + 3],
                             mRawBitmap.Get()[pixelIdx + 0],
                             mRawBitmap.Get()[pixelIdx + 1],
                             mRawBitmap.Get()[pixelIdx + 2]);

          // Apply frequency scale for Y position
          const float y0 = FreqBinToYPosition(bin, mNumBins);
          const float y1 = FreqBinToYPosition(bin + 1, mNumBins);

          IRECT rect(x, std::min(y0, y1), x + colWidth + 0.5f, std::max(y0, y1) + 0.5f);
          g.FillRect(color, rect);
        }
      }
    }
    else
    {
      const float rowHeight = mWidgetBounds.H() / mNumColumns;

      for (int row = 0; row < mNumColumns; row++)
      {
        int texCol;
        if (mScrollDirection == EScrollDirection::TopToBottom)
          texCol = (mWriteColumn - 1 - row + mNumColumns) % mNumColumns;
        else
          texCol = (mWriteColumn + row) % mNumColumns;

        const float y = mWidgetBounds.T + row * rowHeight;

        for (int bin = 0; bin < mNumBins; bin++)
        {
          const int pixelIdx = (bin * mNumColumns + texCol) * 4;
          const IColor color(mRawBitmap.Get()[pixelIdx + 3],
                             mRawBitmap.Get()[pixelIdx + 0],
                             mRawBitmap.Get()[pixelIdx + 1],
                             mRawBitmap.Get()[pixelIdx + 2]);

          const float x0 = FreqBinToXPosition(bin, mNumBins);
          const float x1 = FreqBinToXPosition(bin + 1, mNumBins);

          IRECT rect(std::min(x0, x1), y, std::max(x0, x1) + 0.5f, y + rowHeight + 0.5f);
          g.FillRect(color, rect);
        }
      }
    }
  }

  void ProcessSpectrumData(const ISenderData<MAXNC, TDataPacket>& d)
  {
    if (mPaused)
      return;

    if (mTextureInvalid)
      ResizeTexture();

    if (mNumColumns <= 0 || mNumBins <= 0)
      return;

    // Store raw magnitudes for potential redraw
    for (int bin = 0; bin < mNumBins; bin++)
    {
      float mag = 0.f;

      switch (mChannelMode)
      {
        case EChannelMode::Left:
          mag = d.vals[d.chanOffset][bin];
          break;
        case EChannelMode::Right:
          mag = d.vals[d.chanOffset + std::min(1, d.nChans - 1)][bin];
          break;
        case EChannelMode::Average:
          for (int ch = d.chanOffset; ch < d.chanOffset + d.nChans; ch++)
            mag += d.vals[ch][bin];
          mag /= d.nChans;
          break;
        case EChannelMode::Max:
          for (int ch = d.chanOffset; ch < d.chanOffset + d.nChans; ch++)
            mag = std::max(mag, d.vals[ch][bin]);
          break;
      }

      mMagnitudeBuffer[mWriteColumn * mNumBins + bin] = mag;
    }

    // Update texture column with colormapped values
    UpdateTextureColumn(mWriteColumn);

    mWriteColumn = (mWriteColumn + 1) % mNumColumns;
    SetDirty(false);
  }

  void UpdateTextureColumn(int col)
  {
    uint8_t* pixels = mRawBitmap.Get();
    const int stride = mNumColumns * 4;  // Row stride (width * 4 bytes)

    for (int bin = 0; bin < mNumBins; bin++)
    {
      const float mag = mMagnitudeBuffer[col * mNumBins + bin];
      const float val = NormalizeDB(mag);
      const int lutIdx = Clip(static_cast<int>(val * 255.f), 0, 255);
      const IColor& color = mColorLUT[lutIdx];

      // Write to column col, row bin
      // Texture layout: width = mNumColumns, height = mNumBins
      // idx = row * stride + column * 4
      const int idx = bin * stride + col * 4;
      pixels[idx + 0] = color.R;
      pixels[idx + 1] = color.G;
      pixels[idx + 2] = color.B;
      pixels[idx + 3] = color.A;
    }
  }

  void RedrawFullTexture()
  {
    if (mRawBitmap.GetSize() == 0)
      return;

    for (int col = 0; col < mNumColumns; col++)
      UpdateTextureColumn(col);
  }

  float NormalizeDB(float magnitude) const
  {
    float dB = AmpToDB(magnitude + 1e-30f);
    float normalized = (dB - mMinDB) / (mMaxDB - mMinDB);
    return Clip(normalized, 0.f, 1.f);
  }

  float FreqBinToYPosition(int bin, int totalBins) const
  {
    float t = FreqBinToNorm(bin, totalBins);
    return mWidgetBounds.B - t * mWidgetBounds.H();
  }

  float FreqBinToXPosition(int bin, int totalBins) const
  {
    float t = FreqBinToNorm(bin, totalBins);
    return mWidgetBounds.L + t * mWidgetBounds.W();
  }

  float FreqBinToNorm(int bin, int totalBins) const
  {
    if (mFreqScale == EFrequencyScale::Log)
    {
      const float minFreqNorm = 1.f / totalBins;
      const float freqNorm = std::max(static_cast<float>(bin) / totalBins, minFreqNorm);
      const float logMin = std::log(minFreqNorm);
      const float logMax = std::log(1.f);
      return (std::log(freqNorm) - logMin) / (logMax - logMin);
    }
    else
    {
      return static_cast<float>(bin) / totalBins;
    }
  }

  bool IsHorizontalScroll() const
  {
    return mScrollDirection == EScrollDirection::RightToLeft ||
           mScrollDirection == EScrollDirection::LeftToRight;
  }

  void ResizeTexture()
  {
//    FreeTexture();

    mNumBins = mFFTSize / 2;
    const int textureSize = mNumColumns * mNumBins * 4;  // RGBA

    mRawBitmap.Resize(textureSize);
    memset(mRawBitmap.Get(), 0, textureSize);

    mMagnitudeBuffer.resize(mNumColumns * mNumBins, 0.f);

    mWriteColumn = 0;
    mTextureInvalid = false;
  }

  void BuildColorLUT()
  {
    std::vector<IColor> stops;

    switch (mColormap)
    {
      case EColormap::Viridis:
        stops = {
          IColor(255, 68, 1, 84),
          IColor(255, 59, 82, 139),
          IColor(255, 33, 145, 140),
          IColor(255, 94, 201, 98),
          IColor(255, 253, 231, 37)
        };
        break;

      case EColormap::Magma:
        stops = {
          IColor(255, 0, 0, 4),
          IColor(255, 81, 18, 124),
          IColor(255, 183, 55, 121),
          IColor(255, 252, 137, 97),
          IColor(255, 252, 253, 191)
        };
        break;

      case EColormap::Plasma:
        stops = {
          IColor(255, 13, 8, 135),
          IColor(255, 126, 3, 168),
          IColor(255, 204, 71, 120),
          IColor(255, 248, 149, 64),
          IColor(255, 240, 249, 33)
        };
        break;

      case EColormap::Inferno:
        stops = {
          IColor(255, 0, 0, 4),
          IColor(255, 87, 16, 110),
          IColor(255, 188, 55, 84),
          IColor(255, 249, 142, 9),
          IColor(255, 252, 255, 164)
        };
        break;

      case EColormap::Grayscale:
        stops = {
          IColor(255, 0, 0, 0),
          IColor(255, 255, 255, 255)
        };
        break;

      case EColormap::Turbo:
        stops = {
          IColor(255, 48, 18, 59),
          IColor(255, 70, 132, 238),
          IColor(255, 35, 212, 157),
          IColor(255, 178, 234, 53),
          IColor(255, 253, 141, 60),
          IColor(255, 213, 62, 79),
          IColor(255, 122, 4, 3)
        };
        break;

      case EColormap::Hot:
        stops = {
          IColor(255, 0, 0, 0),
          IColor(255, 128, 0, 0),
          IColor(255, 255, 0, 0),
          IColor(255, 255, 128, 0),
          IColor(255, 255, 255, 0),
          IColor(255, 255, 255, 255)
        };
        break;
    }

    const int nStops = static_cast<int>(stops.size());

    for (int i = 0; i < 256; i++)
    {
      float t = i / 255.f;
      t = std::pow(t, mGamma);

      float scaledT = t * (nStops - 1);
      int idx = static_cast<int>(scaledT);
      float frac = scaledT - idx;

      if (idx >= nStops - 1)
        mColorLUT[i] = stops.back();
      else
        mColorLUT[i] = IColor::LinearInterpolateBetween(stops[idx], stops[idx + 1], frac);
    }
  }

private:
  EColormap mColormap = EColormap::Viridis;
  EFrequencyScale mFreqScale = EFrequencyScale::Log;
  EScrollDirection mScrollDirection = EScrollDirection::RightToLeft;
  EChannelMode mChannelMode = EChannelMode::Average;

  float mMinDB = -90.f;
  float mMaxDB = 0.f;
  float mGamma = 1.f;

  int mFFTSize = 1024;
  int mOverlap = 1;
  int mWindowType = 0;
  double mSampleRate = 44100.0;

  bool mPaused = false;
  bool mTextureInvalid = true;

  // Texture dimensions
  int mNumBins = 512;       // FFT bins (texture HEIGHT)
  int mNumColumns = 256;    // Time history (texture WIDTH)
  int mWriteColumn = 0;     // Current write column position

  // Raw pixel buffer (RGBA, row-major)
  RawBitmapData mRawBitmap;

  // Magnitude buffer for potential redraw
  std::vector<float> mMagnitudeBuffer;

  // Colormap lookup table
  std::array<IColor, 256> mColorLUT;

  IPopupMenu mMenu {"Options"};
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
