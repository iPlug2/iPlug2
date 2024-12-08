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
 * @copydoc IVChromogramControl
 */

#include "IControl.h"
#include "ISender.h"
#include "Smoothers.h"
#include "IPlugStructs.h"

#include <Eigen/Core>

#include <iostream>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** Control to show the pitch chroma
 * @ingroup IControls
 */
template <int MAXNC = 2, int MAX_FFT_SIZE = 4096>
class IVChromogramControl : public IControl
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
  
  /** Channel configuration types */
  enum class EChannelType { Left = 0, Right, LeftAndRight };
  
  using TDataPacket = std::array<float, MAX_FFT_SIZE>;
  
  /** Constructs an IVChromogramControl
   * @param bounds The rectangular area that the control occupies
   * @param label A CString to label the control
   * @param style, /see IVStyle */
  IVChromogramControl(const IRECT& bounds, const char* label = "", const IVStyle& style = DEFAULT_STYLE)
  : IControl(bounds)
  , IVectorBase(style)
  , mFiltersStorage(12, MAX_FFT_SIZE / 2 + 1)
  {
    AttachIControl(this, label);
    
    Init(12, 513, 440.0, 44100.0);
  }
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if (mMenu != nullptr)
      delete mMenu;
    mMenu = new IPopupMenu("Options");
    mMenu->AddItem("FFTSize", new IPopupMenu("FFTSize", { "1024", "2048", "4096"}));
    mMenu->AddItem("Channels", new IPopupMenu("Channels",
                                              { "L", "R", "L + R"}));
    mMenu->AddItem("Speed",
                   new IPopupMenu("Speed",
                                  { "100%", "75%", "50%", "25%"}));
    
    GetUI()->CreatePopupMenu(*this, *mMenu, x, y);
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
        
        float smoothTime;
        if (strcmp(speedStr, "100%") == 0)
          smoothTime = 0.0;
        else if (strcmp(speedStr, "75%") == 0)
          smoothTime = 1.0;
        else if (strcmp(speedStr, "50%") == 0)
          smoothTime = 2.0;
        else if (strcmp(speedStr, "25%") == 0)
          smoothTime = 5.0;
        SetSmoothTime(smoothTime);
      }
    }
  }

  void OnMouseOver(float x, float y, const IMouseMod& mod) override
  {
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
    for (auto i=0; i<12; i++)
    {
      g.FillRect(IColor::FromHSLA(float(i)/12.f, 0.5f, 0.5f, 1.), mWidgetBounds.SubRectHorizontal(12, i));
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
        CalculateChroma(c, d.vals[c].data(), (mFFTSize/2) + 1);
    }
    else if (msgTag == kMsgTagSampleRate)
    {
      IByteStream stream(pData, dataSize);
      double sr;
      stream.Get(&sr, 0);
      SetSampleRate(sr);
    }
    else if (msgTag == kMsgTagFFTSize)
    {
      IByteStream stream(pData, dataSize);
      int fftSize;
      stream.Get(&fftSize, 0);
      SetFFTSize(fftSize);
    }
  }
  
  void SetFFTSize(int fftSize)
  {
    assert(fftSize > 0);
    assert(fftSize <= MAX_FFT_SIZE);
    mFFTSize = fftSize;
    SetFreqRange((mSampleRate*0.5)/mFFTSize, mSampleRate*0.5, mSampleRate);
    SetSmoothTime(mSmoothTime);
    SetDirty(false);
  }

  void SetSampleRate(double sampleRate)
  {
    Init(12, 513, 0., sampleRate);
    mSampleRate = sampleRate;
    SetFreqRange((mSampleRate*0.5)/mFFTSize, mSampleRate*0.5, mSampleRate);
    
    for (int i = 0; i < MAXNC; i++)
      mDataSmooth[i].SetSampleRate(sampleRate);

    SetDirty(false);
  }
  
  void SetFreqRange(float freqLo, float freqHi, float sampleRate)
  {
    //TODO
    SetDirty(false);
  }
  
  void SetAmpRange(float ampLo, float ampHi)
  {
    mYLo = ampLo;
    mYHi = ampHi;
    
    mDBYLo = AmpToDB(ampLo);
    mDBYHi = AmpToDB(ampHi);

    SetDirty(false);
  }

  void SetSmoothTime(float timeMs)
  {
    mSmoothTime = timeMs;
    
    float fftSizeCoeff = 1024.0/mFFTSize;
    timeMs *= fftSizeCoeff;
    
    for (int i = 0; i < MAXNC; i++)
      mDataSmooth[i].SetSmoothTime(timeMs);
  }

protected:
  void Init(int nChroma, int nBins, float ref, float sampleRate)
  {
    using namespace Eigen;
    const float log2E = 1.44269504088896340736;
    auto fftSize = 2 * (nBins - 1);
    ArrayXf freqs = ArrayXf::LinSpaced(fftSize, 0, sampleRate);
    freqs = nChroma * (freqs / (ref / 16)).log() * log2E;
    freqs[0] = freqs[1] - 1.5 * nChroma;

    ArrayXf widths =  ArrayXf::Ones(fftSize);
    widths.segment(0, fftSize - 1) = freqs.segment(1, fftSize - 1) - freqs.segment(0, fftSize - 1);
    widths = widths.max(1.0);
    ArrayXXf diffs = freqs.replicate(1, nChroma).transpose() - ArrayXf::LinSpaced(nChroma, 0, nChroma - 1).transpose().replicate(fftSize, 1).transpose();
    auto halfChroma = std::lrint(nChroma/2);
    ArrayXXf remainder = diffs.unaryExpr([&](const float x){
        return std::fmodf(x + 10* nChroma + halfChroma, nChroma) - halfChroma;
    });
    MatrixXf filters = (-0.5 * (2 * remainder / widths.replicate(1, nChroma)
    .transpose())
    .square())
    .exp()
    .block(0, 0, nChroma, nBins);

    filters.colwise().normalize();

    mFiltersStorage.setZero();
    mFiltersStorage.block(0, 0, nChroma, nBins) = filters;
    mNChroma = nChroma;
    mNBins = nBins;
    mScale = 2.0 / (fftSize * mNChroma);
    mSampleRate = sampleRate;
  }
  
  void CalculateChroma(int ch, float* powerSpectrum, int size)
  {
    if (size < 2)
      return;
    
    using namespace Eigen;
    using namespace std;
    ArrayXf frame = ArrayXf::Zero(size).transpose();
    
    for(auto i=0;i<size;i++)
    {
      frame(i) = powerSpectrum[i];
    }
    
    Eigen::Ref<Eigen::MatrixXf> filters = mFiltersStorage.block(0, 0, mNChroma, mNBins);

//    if (minFreq != 0 || maxFreq != -1){
//      maxFreq = (maxFreq == -1) ? (mSampleRate / 2) : min(maxFreq, mSampleRate / 2);
//      double  binHz = mSampleRate / ((mNBins - 1) * 2.);
//      index   minBin =
//          minFreq == 0 ? 0 : static_cast<index>(ceil(minFreq / binHz));
//      index   maxBin =
//          min(static_cast<index>(floorl(maxFreq / binHz)), (mNBins - 1));
//      frame.segment(0, minBin).setZero();
//      frame.segment(maxBin, frame.size() - maxBin).setZero();
//    }

    ArrayXf result = mScale * (filters * frame.square().matrix()).array();

//    if (normalize > 0) {
//      double norm = normalize == 1? result.sum() : result.maxCoeff();
//      result = result / std::max(norm, epsilon);
//    }
    
    // use result
  }
  
  double mSampleRate = 44100.0;
  
  int mFFTSize = 1024;

  // Frequencies
//  double minFreq = 0;
//  double maxFreq = -1;
  
  // Amp
  float mYLo;
  float mYHi;
  
  float mDBYLo;
  float mDBYHi;
  
  float mPointerAmp = 0.0;
  float mPointerFreq = -1.0;

  EChannelType mChanType = EChannelType::LeftAndRight;

  LogDataSmooth<float> mDataSmooth[MAXNC];
  float mSmoothTime = 2.0;
  
  IPopupMenu* mMenu = nullptr;
  
  int mNChroma;
  int mNBins;
  double mScale;
  Eigen::MatrixXf mFiltersStorage;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
