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

#include <string>

#include "INanoVGShaderControl.h"
#include "IGraphicsColorMap.h"
#include "ITransport.h"
#include "ISender.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** ISpectrogramControl displays a spectrogram that scrolls
    as it is fed by input data
*/

class ISpectrogramControl : public INanoVGShaderControl
{
public:
  static constexpr int MAX_FFT_SIZE = 4096;
  
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

  ISpectrogramControl(const IRECT& bounds, int fftSize = 1024,
                      int overlap = 2, int numRows = 128,
                      EDirection direction = EDirection::Horizontal,
                      EFrequencyScale freqScale = EFrequencyScale::Mel)
  : INanoVGShaderControl(bounds)
  , mFFTSize(fftSize)
  , mOverlap(overlap)
  , mNumRows(numRows)
  , mDirection(direction)
  , mFreqScale(freqScale)
  , mColorMapList( {}, "MATLAB_jet" )
  {
    SetVertexShaderStr(
#if defined IGRAPHICS_GL
    R"(
      attribute vec4 apos;
      attribute vec2 atexcoord;
      uniform float dscale;
      uniform int dir;
      varying vec2 texcoord;
      void main() {
        texcoord = atexcoord;
        if (dir == 0)
          gl_Position = vec4(apos.x, apos.y * dscale, apos.z, apos.w);
        else
          gl_Position = vec4(apos.x * dscale, apos.y, apos.z, apos.w);
      }
    )"
#elif defined IGRAPHICS_METAL
    R"(
    )"
#endif
    );

    SetupFragmentShader();
    SetAxes();

    SetFreqRange(NyquistFreq()/mFFTSize, NyquistFreq());
    SetDBRange(-90.0f, 0.0f);
    
    mTransport.SetSampleRate(mSampleRate);
    mTransport.SetOverlap(mOverlap);
    
    IText txt(DEFAULT_TEXT_SIZE, IColor(255, 255, 255, 255));
    SetText(txt);
    
    CheckSpectrogramDataSize();
    UpdateTimeAxis();
  }
  
  void SetAxes()
  {
    // mTimeAxis = std::make_unique<ITimeAxis>(mRECT, mText, 0.0, 1.0,
    //            (mDirection == EDirection::Vertical) ?
    //                       EDirection::Vertical : EDirection::Horizontal);
    // std::vector<float> freqs = { 100.0, 500.0, 1000.0, 5000.0, 10000.0, 20000.0 };
    // mFreqAxis = std::make_unique<IFrequencyAxis>(mRECT, mText,
    //                                              freqs,
    //             20.0, 20000.0, mFreqScale,
    //             (mDirection == EDirection::Vertical) ?
    //                            EDirection::Horizontal : EDirection::Vertical);
  }
  
  void OnResize() override
  {
    SetAxes();
    CheckSpectrogramDataSize();
    INanoVGShaderControl::OnResize();
  }
  
  void OnRescale() override
  {
    SetAxes();
    CheckSpectrogramDataSize();
    INanoVGShaderControl::OnRescale();
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
    mMenu.AddItem("ScrollSpeed", new IPopupMenu("ScrollSpeed", { "1", "2", "4", "8"}))
      ->GetSubmenu()->CheckItemWithText(std::to_string(mScrollSpeed).c_str());
    
    GetUI()->CreatePopupMenu(*this, mMenu, x, y);
  }
  
  void OnMouseOver(float x, float y, const IMouseMod& mod) override
  {
    // Compute frequency under pointer
    const auto xNorm = (mDirection == EDirection::Horizontal) ? (1.0 - y/mRECT.H()) : x/mRECT.W();
    mCursorFreq = CalcXNorm(xNorm, mFreqScale, mSampleRate) * NyquistFreq();
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
      else if (strcmp(title, "ScrollSpeed") == 0)
      {
        int scrollSpeed = atoi(pSelectedMenu->GetChosenItem()->GetText());
        SetScrollSpeed(scrollSpeed);
      }
    }
  }
  
  bool IsDirty() override
  {
    // We need to refresh continuously even if there is no new data,
    // to get smooth scrolling
    return true;
  }
  
  void PreDraw(IGraphics& g) override
  {
    g.FillRect(COLOR_BLACK, mRECT);
  }
  
  void PostDraw(IGraphics& g) override
  {
    UpdateTimeAxis();
//    auto timeAxisClipRect = mRECT.GetReducedFromRight(50);
    // mTimeAxis->Draw(g, timeAxisClipRect);
    // mFreqAxis->Draw(g, mRECT);
    
    auto mouseOverRect = mRECT.GetFromTLHC(50, 50);

    // Draw frequency value under pointer
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
      if (!mScrollEnabled)
        return;

      ISenderData<1, TDataPacket> d;
      stream.Get(&d, 0);
      
      for (auto c = d.chanOffset; c < (d.chanOffset + d.nChans); c++)
      {
        // Process only the left channel
        if (c > 0)
          break;

        ScaleSpectrogramData(c, d.vals[c]);
        UpdateSpectrogram(c, d.vals[c]);
        mTransport.OnNewData(mFFTSize);
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
  
  void SetText(const IText& txt) override
  {
    IControl::SetText(txt);
    // mTimeAxis->SetText(txt);
    // mFreqAxis->SetText(txt);
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

    CheckSpectrogramDataSize();
    ResetSpectrogramData();
    UpdateTimeAxis();
  }

  void SetOverlap(int overlap)
  {
    assert(overlap > 0);
    mOverlap = overlap;

    mTransport.SetOverlap(mOverlap);

    CheckSpectrogramDataSize();
    UpdateTimeAxis();
  }

  void SetSampleRate(double sampleRate)
  {
    mSampleRate = sampleRate;
    mTransport.SetSampleRate(sampleRate);
//    mFreqAxis->SetRangeHi(NyquistFreq());
    SetFreqRange(NyquistFreq()/mFFTSize, NyquistFreq());
    ResetSpectrogramData();
  }

  void SetFreqRange(float freqLo, float freqHi)
  {
    mFreqLo = freqLo;
    mFreqHi = freqHi;
//    mFreqAxis->SetRange(freqLo, freqHi);
  }
  
  void SetFreqScale(EFrequencyScale freqScale)
  {
    mFreqScale = freqScale;
//    mFreqAxis->SetScale(freqScale);
  }
  
  void SetDBRange(float dbLo, float dbHi)
  {
    mAmpLo = std::log(std::pow(10.f, dbLo / 10.0));
    mAmpHi = std::log(std::pow(10.f, dbHi / 10.0));
  }

  void SetDirection(EDirection direction)
  {
    mDirection = direction;
    // mTimeAxis->SetDirection((direction == EDirection::Vertical) ?
    //                        EDirection::Vertical : EDirection::Horizontal);
    // mFreqAxis->SetDirection((direction == EDirection::Horizontal) ?
    //                        EDirection::Vertical : EDirection::Horizontal);
  }
  
  void SetScrollEnabled(bool enabled)
  {
    if (mScrollEnabled && !enabled)
    {
      mTransport.Reset();
      UpdateTimeAxis();
    }
    
    mScrollEnabled = enabled;

    if (!mScrollEnabled)
      mTransport.Reset();
  }

  void SetScrollSpeed(int scrollSpeed)
  {
    mQueuedScrollSpeed = scrollSpeed;
  }
  
  void SetColorMap(const char* colormap)
  {
    mColorMapList.SetColorMap(colormap);
    mNeedSetupShader = true;
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
  void SetupFragmentShader()
  {
    WDL_String fragmentShader(
#if defined IGRAPHICS_GL
    R"(
      uniform sampler2D texid;
      uniform float texorigin;
      uniform int dir;
      uniform float soffset;
      uniform float ccontrast;
      uniform float crange;
      varying vec2 texcoord;

      float contrastRange(float x, float contrast, float range);
      vec4 colormap(float x);

      void main() {
        vec2 tc;
        if (dir == 0)
        {
          float t = texcoord.y + texorigin;
          if (t > 1.0) t -= 1.0;
          tc = vec2(texcoord.x, t + soffset);
        }
        else
        {
          float t = texcoord.x + texorigin;
          if (t > 1.0) t -= 1.0;
          tc = vec2(texcoord.y, t + soffset);
        }
        vec4 col = texture2D(texid, tc);
        float x = col.r;
        x = contrastRange(x, ccontrast, crange);
        gl_FragColor = colormap(x);
      }
    
      // Schlick sigmoid, see:
      //
      // https://dept-info.labri.u-bordeaux.fr/~schlick/DOC/gem2.ps.gz
      //
      // a included in [0, 1]
      // a = 0.5 -> gives a line
      float contrastRange(float t, float a, float r)
      {
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
      }

    )"
#elif defined IGRAPHICS_METAL
    R"(
    )"
#endif
    );
    
    mColorMapList.AppendToShader(fragmentShader);
    SetFragmentShaderStr(fragmentShader.Get());
  }
  
  void ScaleSpectrogramData(int ch, TDataPacket& powerSpectrum)
  {
    auto numBins = NumBins();
    
    // Scale X
    if (mTmpBuf.size() != numBins)
    {
      mTmpBuf.resize(numBins);
    }
    
    std::copy(powerSpectrum.begin(), powerSpectrum.begin() + numBins, mTmpBuf.begin());
    
    float xRecip = 1.0f / float(numBins-1);
    // Start at 1, don't use the DC bin
    for (auto i = 1; i < numBins; i++)
    {
      // Possible improvement: interpolation with the 2 closest values
      float x = CalcXNorm(i * xRecip, mFreqScale, mSampleRate) * float(numBins-1);

      // Bilinear interpolation
      int id0 = (int) x;
      int id1 = (int) x + 1;
      float t = x - (int) x;

      float v0 = mTmpBuf[id0];
      float v1 = mTmpBuf[id1];
      
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
    mNeedUpdateTexture = true;
  }

  void CheckSpectrogramDataSize()
  {
    auto textureSize = NumRows() * NumBins();
    
    if (mTextureBuf.size() != textureSize)
    {
      mTextureBuf.resize(textureSize);
      ResetSpectrogramData();
    }
  }

  void ResetSpectrogramData()
  {
    std::fill(mTextureBuf.begin(), mTextureBuf.end(), 0.0f);
    mNeedUpdateTexture = true;
  }
  
  void UpdateTimeAxis()
  {
    // if (!mScrollEnabled)
    //   return;

    // const auto numRows = NumRows();
    // auto spectrogramDuration = mTransport.ComputeDataDuration(numRows*mFFTSize);
    // auto dscaleInv = ((float)(numRows - mMaxDriftRows*2.0))/numRows;
    // spectrogramDuration *= dscaleInv;

    // double now = mTransport.GetElapsedTime();
    // mTimeAxis->SetRange(now*0.001, (now + spectrogramDuration)*0.001);
  }
    
  float CalcXNorm(float x, EFrequencyScale scale, float sampleRate) const
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

    int scrollSpeedCoeff = (int)(8.0 / mScrollSpeed);
    numRows *= scrollSpeedCoeff;
    
    return numRows;
  }
  
  float NyquistFreq() const { return mSampleRate * 0.5; }
  int NumBins() const { return mFFTSize / 2; }
  
#pragma mark -
  void DrawToFBO(int w, int h) override;
  void CreateTexture();
  
private:
  double mSampleRate = 44100.0;
  
  int mFFTSize;
  int mOverlap;
  int mNumRows;
  
  // For smooth scrolling
  int mMaxDriftRows = 4;

  // Linear
  float mFreqLo = 20.0;
  float mFreqHi = 22050.0;
  float mAmpLo = 0.0;
  float mAmpHi = 1.0;

  ITransport mTransport;
  
  // std::unique_ptr<ITimeAxis> mTimeAxis;
  // std::unique_ptr<IFrequencyAxis> mFreqAxis;

  IColorMapList mColorMapList;

  bool mScrollEnabled = true;
  
  float mColorMapRange = 0.5;
  float mColorMapContrast = 0.5;
  
  // Raw buffer used to generate the texture
  // (kind of circular buffer)
  std::vector<float> mTextureBuf;
  int mTextureBufWriteIndex = 0;
  
  EDirection mDirection;

  int mScrollSpeed = 8;
  int mQueuedScrollSpeed = 8;
  
  EFrequencyScale mFreqScale;

  float mCursorFreq = -1.0;
  
  bool mNeedUpdateTexture = true;
  bool mTextureGenerated = false;

  unsigned int mTexId;

  std::vector<float> mTmpBuf;

  IPopupMenu mMenu {"Options"};

  bool mNeedSetupShader = true;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
