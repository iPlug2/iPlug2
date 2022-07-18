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

#define FFTSIZE_VA_LIST "128", "256", "512", "1024", "2048", "4096"
#define FFTWINDOWS_VA_LIST "Hann", "BlackmanHarris", "Hamming", "Flattop", "Rectangular"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** Vectorial multi-channel capable spectrum analyzer control
 * @ingroup IControls
 * Derived from work by Alex Harker and Matthew Witmer
 */
template <int MAXNC = 1, int MAX_FFT_SIZE = 4096>
class IVSpectrumAnalyzerControl : public IControl
                                , public IVectorBase
{
  using TDataPacket = std::array<float, MAX_FFT_SIZE>;
  
public:
  /** Constructs an IVSpectrumAnalyzerControl
   * @param bounds The rectangular area that the control occupies
   * @param label A CString to label the control
   * @param style, /see IVStyle */
  IVSpectrumAnalyzerControl(const IRECT& bounds, const char* label = "", const IVStyle& style = DEFAULT_STYLE,
                            std::initializer_list<IColor> colors = {COLOR_BLACK})
  : IControl(bounds)
  , IVectorBase(style)
  , mChannelColors(colors)
  {
    AttachIControl(this, label);
 
    SetFreqRange(20.f, 20000.f, 44100.f);
    SetDBRange(-90.f, 0.f);
  }
  
  void Draw(IGraphics& g) override
  {
    DrawBackground(g, mRECT);
    DrawWidget(g);
    DrawLabel(g);
    
    if (mStyle.drawFrame)
      g.DrawRect(GetColor(kFR), mWidgetBounds, &mBlend, mStyle.frameThickness);
  }

  void DrawWidget(IGraphics& g) override
  {
    for (auto c = 0; c < MAXNC; c++)
    {
      g.DrawData(mChannelColors[c], mWidgetBounds, mYPoints[c].data(), NPoints(c), mXPoints[c].data());
    }
  }
  
  void OnResize() override
  {
    SetTargetRECT(MakeRects(mRECT));
    SetDirty(false);
  }
  
  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    if (!IsDisabled() && msgTag == ISender<>::kUpdateMessage)
    {
      IByteStream stream(pData, dataSize);
      
      int pos = 0;
      ISenderData<MAXNC, TDataPacket> d;
      pos = stream.Get(&d, pos);
      
      for (auto c = d.chanOffset; c < (d.chanOffset + d.nChans); c++)
      {
        CalculatePoints(c, d.vals[c].data(), (mFFTSize / 2) + 1);
      }
      
      SetDirty(false);
    }
  }
  
  void SetFFTSize(int fftSize)
  {
    assert(fftSize > 0);
    assert(fftSize <= MAX_FFT_SIZE);
    mFFTSize = fftSize;
    
    for (auto c = 0; c < MAXNC; c++)
    {
      mXPoints[c].clear();
      mYPoints[c].clear();
    }
  }
  
  void SetFreqRange(float freqLo, float freqHi, float sampleRate)
  {
    mLogXLo = std::log(freqLo / (sampleRate / 2.f));
    mLogXHi = std::log(freqHi / (sampleRate / 2.f));
  }
  
  void SetDBRange(float dbLo, float dbHi)
  {
    mLogYLo = std::log(std::pow(10.f, dbLo / 10.0));
    mLogYHi = std::log(std::pow(10.f, dbHi / 10.0));
  }
  
protected:
  std::vector<IColor> mChannelColors;
  int mFFTSize = 1024;

  void CalculatePoints(int ch, const float* powerSpectrum, int size)
  {
    mXPoints[ch].reserve(size);
    mYPoints[ch].reserve(size);
    mXPoints[ch].clear();
    mYPoints[ch].clear();
    
    float xRecip = 1.f / static_cast<float>(size);
    float xAdvance = mOptimiseX / mWidgetBounds.W();
    float xPrev = 0.f;
    
    float ym2 = CalcYNorm(powerSpectrum[1]);
    float ym1 = CalcYNorm(powerSpectrum[0]);
    float yp0 = CalcYNorm(powerSpectrum[1]);
    float yp1 = 0.0;
    
    // Don't use the DC bin
    
    unsigned long i = 1;
    
    // Calculate Curve
    
    auto InterpolateCubic = [](const float& x, const float& y0, const float& y1, const float& y2, const float& y3) {
      // N.B. - this is currently a high-quality cubic hermite
      
      const auto c0 = y1;
      const auto c1 = 0.5f * (y2 - y0);
      const auto c2 = y0 - 2.5f * y1 + y2 + y2 - 0.5f * y3;
      const auto c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
      
      return (((c3 * x + c2) * x + c1) * x + c0);
    };
    
    for (; i < size; i++)
    {
      const auto N = NPoints(ch);
      float x = CalcXNorm(i * xRecip);
      
      // Add cubic smoothing if desired
      
      if (i + 1 < size)
      {
        yp1 = CalcYNorm(powerSpectrum[i+1]);
        
        if (mSmoothX)
        {
          float xInterp = 1.0 / (x - xPrev);
          
          for (float xS = xPrev + xAdvance; xS < x - xAdvance; xS += xAdvance)
            AddPoint(ch, IVec2(xS, InterpolateCubic((xS - xPrev) * xInterp, ym2, ym1, yp0, yp1)));
        }
      }
      
      AddPoint(ch, IVec2(x, yp0));
      
      ym2 = ym1;
      ym1 = yp0;
      yp0 = yp1;
      
      if (N && (XCalc(mXPoints[ch][N]) - XCalc(mXPoints[ch][N - 1])) < mOptimiseX)
        break;
      
      xPrev = x;
    }
    
    while (i < size)
    {
      IVec2 minPoint(CalcXNorm(i * xRecip), powerSpectrum[i]);
      IVec2 maxPoint = minPoint;
      
      float x = XCalc(minPoint.x);
      float xNorm = CalcXNorm(++i * xRecip);
      
      while (((XCalc(xNorm) - x) < mOptimiseX) && i < size)
      {
        if (powerSpectrum[i] < minPoint.y)
          minPoint = IVec2(xNorm, powerSpectrum[i]);
        if (powerSpectrum[i] > maxPoint.y)
          maxPoint = IVec2(xNorm, powerSpectrum[i]);
        xNorm = CalcXNorm(++i * xRecip);
      }
      
      if (minPoint.x < maxPoint.x)
        ConvertAndAddPoints(ch, minPoint, maxPoint);
      else
        ConvertAndAddPoints(ch, maxPoint, minPoint);
    }
  }
  
  void AddPoint(int ch, const IVec2& p)
  {
    mXPoints[ch].push_back(p.x);
    mYPoints[ch].push_back(p.y);
  }
  
  void ConvertAndAddPoints(int ch, IVec2& p1, IVec2& p2)
  {
    p1.y = CalcYNorm(p1.y);
    p2.y = CalcYNorm(p2.y);
    
    AddPoint(ch, p1);
    AddPoint(ch, p2);
  }
    
  float XCalc(float xNorm) const { return mWidgetBounds.L + (mWidgetBounds.W() * xNorm); }
  float YCalc(float yNorm) const { return mWidgetBounds.B - (mWidgetBounds.H() * yNorm); }
  float CalcXNorm(float x) const { return (std::log(x) - mLogXLo) / (mLogXHi - mLogXLo); }
  float CalcYNorm(float y) const { return (std::log(y) - mLogYLo) / (mLogYHi - mLogYLo); }
  int NPoints(int ch) const { return static_cast<int>(mXPoints[ch].size()); }

  float mOptimiseX = 1.f;
  float mSmoothX = 1.f;
  
  float mLogXLo;
  float mLogXHi;
  float mLogYLo;
  float mLogYHi;
  
  std::array<std::vector<float>, MAXNC> mXPoints;
  std::array<std::vector<float>, MAXNC> mYPoints;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
