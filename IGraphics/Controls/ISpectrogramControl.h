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

#include "IControl.h"
#include "IGraphicsColorMap.h"
#include "ISender.h"

#include <string>

#include "nanovg.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** ISpectrogramControl displays a spectrogram
*/
class ISpectrogramControl : public IControl
{
public:
  static constexpr int MAX_FFT_SIZE = 4096;
  enum class EFrequencyScale { Linear, Log, Mel };
  enum class EAmplitudeScale { Linear, Decibel };

  enum MsgTags
  {
    kMsgTagSampleRate = 1,
    kMsgTagFFTSize,
    kMsgTagOverlap,
    kMsgTagWindowType,
    kMsgTagColorMapRange,
    kMsgTagColorMapContrast
  };
  
  using TDataPacket = std::array<float, MAX_FFT_SIZE>;

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
  , mColorMapList( {}, "MATLAB_jet" )
  {
    SetFreqRange(FirstBinFreq(), NyquistFreq());
    SetDBRange(-90.0f, 0.0f);
    RezizeBuffers();
  }
  
  ~ISpectrogramControl()
  {
    FreeTexture();
  }
  
  void OnResize() override
  {
    RezizeBuffers();
  }
  
  void OnRescale() override
  {
    RezizeBuffers();
  }
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    mMenu.Clear(true);
    mMenu.AddItem("Scale", new IPopupMenu("Scale", {"Linear", "Log", "Mel"}))
      ->GetSubmenu()->CheckItemAlone(int(mFreqScale));
    mMenu.AddItem("Direction", new IPopupMenu("Direction", {"Horizontal", "Vertical"}))
      ->GetSubmenu()->CheckItemAlone(mDirection == EDirection::Horizontal ? 0 : 1);
    mMenu.AddItem("ColorMap", mColorMapList.CreatePopupMenu());
    mMenu.AddItem("FFTSize", new IPopupMenu("FFTSize", { "64", "128", "256", "512", "1024", "2048", "4096"}))
      ->GetSubmenu()->CheckItemWithText(std::to_string(mFFTSize).c_str());
    mMenu.AddItem("Overlap", new IPopupMenu("Overlap", { "2", "4"}))
      ->GetSubmenu()->CheckItemAlone(mOverlap == 2 ? 0 : 1);
    
    GetUI()->CreatePopupMenu(*this, mMenu, x, y);
  }
  
  void OnMouseOver(float x, float y, const IMouseMod& mod) override
  {
    const auto xNorm = (mDirection == EDirection::Horizontal) ? (1.0 - y/mRECT.H()) : x/mRECT.W();
    mCursorFreq = CalcXNorm(xNorm, mFreqScale) * NyquistFreq();
  }
  
  void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int valIdx) override
  {
    if (pSelectedMenu)
    {
      const char* title = pSelectedMenu->GetRootTitle();
      
      if (strcmp(title, "Scale") == 0)
      {
        auto freqScale = EFrequencyScale(pSelectedMenu->GetChosenItemIdx());
        SetFreqScale(freqScale);
      }
      else if (strcmp(title, "Direction") == 0)
      {
        SetDirection(pSelectedMenu->GetChosenItemIdx() == 0 ? EDirection::Horizontal :
                                                              EDirection::Vertical);
      }
      else if (strcmp(title, "ColorMap") == 0)
      {
        const char* colormap = pSelectedMenu->GetChosenItem()->GetText();
        SetColorMap(colormap);
      }
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
    }
  }
  
  bool IsDirty() override
  {
    return true;
  }
  
  void Draw(IGraphics& g) override
  {
    g.FillRect(COLOR_BLACK, mRECT);
    UpdateSpectrogramBitmap();
    
#ifdef IGRAPHICS_NANOVG
    int w = NumBins();
    int h = NumRows();
    
    if (mImgID == 0)
    {
      mImgID = nvgCreateImageRGBA((NVGcontext*) g.GetDrawContext(), w, h, 0, mRawBitmap.Get());
    }
    else
    {
      nvgUpdateImage((NVGcontext*) g.GetDrawContext(), mImgID, mRawBitmap.Get());
    }
    
    APIBitmap tempAPIBitmap(mImgID, w, h, 1, 1);
    IBitmap tempBitmap(&tempAPIBitmap, 1, false);
    g.DrawFittedBitmap(tempBitmap, mRECT);
#endif
    
    DrawCursorValues(g);
  }
  
  void DrawCursorValues(IGraphics& g)
  {
    auto mouseOverRect = mRECT.GetFromTLHC(50, 50);

    if (mCursorFreq >= 0.0)
    {
      WDL_String freqLabel;
      freqLabel.SetFormatted(64, "%.1fHz", mCursorFreq);
      g.DrawText(mText, freqLabel.Get(), mouseOverRect.R, mouseOverRect.B + mText.mSize);
    }
  }
  
  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    IByteStream stream(pData, dataSize);

    if (!IsDisabled() && msgTag == ISender<>::kUpdateMessage)
    {
      ISenderData<1, TDataPacket> d;
      stream.Get(&d, 0);
      
      for (auto c = d.chanOffset; c < (d.chanOffset + d.nChans); c++)
      {
        // Process only the left channel
        if (c > 0)
          break;

        ScaleSpectrogramData(c, d.vals[c]);
        UpdateSpectrogram(c, d.vals[c]);
      }
      
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
  
  void SetFFTSizeAndOverlapFromControl(int fftSize, int overlap)
  {
    SetFFTSize(fftSize);
    SetOverlap(overlap);
    ResetSpectrogramData();
  }

  void SetFFTSize(int fftSize)
  {
    assert(fftSize > 0);
    assert(fftSize <= MAX_FFT_SIZE);
    mFFTSize = fftSize;
    RezizeBuffers();
    ResetSpectrogramData();
  }

  void SetOverlap(int overlap)
  {
    assert(overlap > 0);
    mOverlap = overlap;
    RezizeBuffers();
  }

  void SetSampleRate(double sampleRate)
  {
    mSampleRate = sampleRate;
    SetFreqRange(NyquistFreq()/mFFTSize, NyquistFreq());
    ResetSpectrogramData();
  }

  void SetFreqRange(float freqLo, float freqHi)
  {
    mFreqLo = freqLo;
    mFreqHi = freqHi;
  }
  
  void SetFreqScale(EFrequencyScale freqScale)
  {
    mFreqScale = freqScale;
  }
  
  void SetDBRange(float dbLo, float dbHi)
  {
    mAmpLo = std::log(std::pow(10.f, dbLo / 10.0f));
    mAmpHi = std::log(std::pow(10.f, dbHi / 10.0f));
  }

  void SetDirection(EDirection direction)
  {
    mDirection = direction;
  }
  
  void SetColorMap(const char* colormap)
  {
    mColorMapList.SetColorMap(colormap);
  }

  void SetColorMapRange(float range)
  {
    mColorMapRange = range;
  }

  void SetColorMapContrast(float contrast)
  {
    mColorMapContrast = contrast;
  }

#pragma mark -
private:
  void CreateBitmap()
  {
    int w = NumBins();
    int h = NumRows();

    if (w > 0 && h > 0)
    {
      mRawBitmap.Resize(w * h * 4);
    }
  }
  
  void UpdateSpectrogramBitmap()
  {
    uint8_t* pixels = mRawBitmap.Get();
    int w = NumBins();
    int h = NumRows();
    
    auto contrastRange = [](float t, float a, float r) {
      t *= r*2.0; // range
      if (t < 0.0)
        t = 0.0;
      if (t > 1.0)
        t = 1.0;
      // contrast
      float f0 = (1.0/(1.0 - a) - 2.0)*(1.0 - 2.0*t);
      float ga;
      if (t < 0.5)
        ga = t/(f0 + 1.0);
      else
        ga = (f0 - t)/(f0 - 1.0);
      t = ga;
      return t;
    };

    for (int y = 0; y < h; y++)
    {
      for (int x = 0; x < w; x++)
      {
        float val = std::clamp(mTextureBuf[y * w + x], 0.0f, 1.0f);
        val = contrastRange(val, mColorMapContrast, mColorMapRange);
        IColor color = mColorMapList.GetColor(val);

        int idx = (y * w + x) * 4;
        pixels[idx + 0] = color.R; // Red
        pixels[idx + 1] = color.G; // Green
        pixels[idx + 2] = color.B; // Blue
        pixels[idx + 3] = color.A; // Alpha
      }
    }
  }
  
  void ScaleSpectrogramData(int ch, TDataPacket& powerSpectrum)
  {
    auto numBins = NumBins();
    float xRecip = 1.0f / float(numBins-1);
    
    // Process bins from right to left to avoid overwriting values we still need
    for (int i = numBins - 1; i > 0; i--)
    {
      float x = CalcXNorm(i * xRecip, mFreqScale) * float(numBins-1);

      // Bilinear interpolation
      int id0 = (int) x;
      int id1 = (int) x + 1;
      float t = x - (int) x;

      float v0 = powerSpectrum[id0];
      float v1 = powerSpectrum[id1 & (numBins-1)];
      
      powerSpectrum[i] = (1.0f - t) * v0 + t * v1;
    }
    
    // Scale Y
    for (auto i = 0; i < numBins; i++)
    {
      powerSpectrum[i] = CalcYNorm(powerSpectrum[i]);
    }
  }
    
  void UpdateSpectrogram(int ch, TDataPacket& powerSpectrum)
  {
    int numRows = NumRows();
    int rowIndex = mTextureBufWriteIndex++ % numRows;
    std::copy(powerSpectrum.begin(), powerSpectrum.begin() + NumBins(), mTextureBuf.begin() + rowIndex * NumBins());
  }

  void RezizeBuffers()
  {
    auto textureSize = NumRows() * NumBins();
    mTextureBuf.resize(textureSize);
    ResetSpectrogramData();
    CreateBitmap();
    FreeTexture();
  }

  void ResetSpectrogramData()
  {
    std::fill(mTextureBuf.begin(), mTextureBuf.end(), 0.0f);
  }
    
  float CalcXNorm(float x, EFrequencyScale scale) const
  {
    const auto nyquist = NyquistFreq();
    
    switch (scale)
    {
      case EFrequencyScale::Linear:
      {
        return (mFreqLo + x * (mFreqHi - mFreqLo)) / nyquist;
      }
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
        return (1.0 / nyquist) * MelToHz(melXLo + x * (melXHi - melXLo));
      }
    }
  }

  float CalcYNorm(float y) const
  {
    return (std::log(y) - mAmpLo) / (mAmpHi - mAmpLo);
  }

  int NumRows() const
  {
    float coeff = (mSampleRate / 44100.0) * (1024.0 / mFFTSize);
    int numRows = std::round(coeff * mNumRows);
    return numRows;
  }
  
#pragma mark -
private:
  float FirstBinFreq() const { return NyquistFreq()/mFFTSize; }
  float NyquistFreq() const { return mSampleRate * 0.5; }
  int NumBins() const { return mFFTSize / 2; }
  
  void FreeTexture()
  {
    if (mImgID > 0)
    {
      nvgDeleteImage((NVGcontext*) GetUI()->GetDrawContext(), mImgID);
    }
    mImgID = 0;
  }
  
  template<typename T=double>
  static inline T HzToMel(T freq) { return T(2595.0) * std::log10(T(1.0) + freq / T(700.0)); }

  template<typename T=double>
  static inline T MelToHz(T mel) { return T(700.0) * (std::pow(T(10.0), (mel/T(2595.0))) - T(1.0)); }
  
  double mSampleRate = 44100.0;
  int mFFTSize;
  int mOverlap;
  int mNumRows;
  // Linear
  float mFreqLo = 20.0;
  float mFreqHi = 22050.0;
  float mAmpLo = 0.0;
  float mAmpHi = 1.0;

  IColorMapList mColorMapList;

  std::vector<float> mTextureBuf;
  int mTextureBufWriteIndex = 0;
  RawBitmapData mRawBitmap;

  EDirection mDirection;
  EFrequencyScale mFreqScale;

  float mCursorFreq = -1.0;

  float mColorMapRange = 0.5;
  float mColorMapContrast = 0.5;
  
  int mImgID = 0;

  IPopupMenu mMenu {"Options"};
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
