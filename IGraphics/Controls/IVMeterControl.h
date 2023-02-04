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
 * @copydoc IVMeterControl
 */

#include "IControl.h"
#include "ISender.h"
#include "IPlugStructs.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** Vectorial multi-channel capable meter control, linear or log response
 * @ingroup IControls */
template <int MAXNC = 1>
class IVMeterControl : public IVTrackControlBase
{
public:
  enum class EResponse {
    Linear,
    Log,
  };
  
  IVMeterControl(const IRECT& bounds, const char* label, const IVStyle& style = DEFAULT_STYLE, EDirection dir = EDirection::Vertical, std::initializer_list<const char*> trackNames = {}, int totalNSegs = 0, EResponse response = EResponse::Linear, float lowRangeDB = -72.f, float highRangeDB = 12.f, std::initializer_list<int> markers = {0, -6, -12, -24, -48})
  : IVTrackControlBase(bounds, label, style, MAXNC, totalNSegs, dir, trackNames)
  , mResponse(response)
  , mLowRangeDB(lowRangeDB)
  , mHighRangeDB(highRangeDB)
  , mMarkers(markers)
  {
  }
  
  void SetResponse(EResponse response)
  {
    mResponse = response;
    SetDirty(false);
  }

  void Draw(IGraphics& g) override
  {
    DrawBackground(g, mRECT);
    DrawWidget(g);
    DrawLabel(g);
    
    if (mResponse == EResponse::Log)
    {
      DrawMarkers(g);
    }
    
    if (mStyle.drawFrame)
      g.DrawRect(GetColor(kFR), mWidgetBounds, &mBlend, mStyle.frameThickness);
  }
  
  void DrawPeak(IGraphics& g, const IRECT& r, int chIdx, bool aboveBaseValue) override
  {
    g.FillRect(IVTrackControlBase::GetColor(kX1), r, &mBlend);
  }
  
  void DrawMarkers(IGraphics& g)
  {
    auto lowPointAbs = std::fabs(mLowRangeDB);
    auto rangeDB = std::fabs(mHighRangeDB - mLowRangeDB);
    
    for (auto pt : mMarkers)
    {
      auto linearPos = (pt + lowPointAbs)/rangeDB;

      auto r = mWidgetBounds.FracRect(IVTrackControlBase::mDirection, linearPos);
      
      if (IVTrackControlBase::mDirection == EDirection::Vertical)
      {
        r.B = r.T + 10.f;
        g.DrawLine(IVTrackControlBase::GetColor(kHL), r.L , r.T, r.R, r.T);
      }
      else
      {
        r.L = r.R - 10.f;
        g.DrawLine(IVTrackControlBase::GetColor(kHL), r.MW(), r.T, r.MW(), r.B);
      }
      
      if (mStyle.showValue)
      {
        WDL_String str;
        str.SetFormatted(32, "%i dB", pt);
        g.DrawText(DEFAULT_TEXT, str.Get(), r);
      }
    }
  }

  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    if (!IsDisabled() && msgTag == ISender<>::kUpdateMessage)
    {
      IByteStream stream(pData, dataSize);

      int pos = 0;
      ISenderData<MAXNC> d;
      pos = stream.Get(&d, pos);

      if (mResponse == EResponse::Log)
      {
        auto lowPointAbs = std::fabs(mLowRangeDB);
        auto rangeDB = std::fabs(mHighRangeDB - mLowRangeDB);
        for (auto c = d.chanOffset; c < (d.chanOffset + d.nChans); c++)
        {
          auto ampValue = AmpToDB(static_cast<double>(d.vals[c]));
          auto linearPos = (ampValue + lowPointAbs)/rangeDB;
          SetValue(Clip(linearPos, 0., 1.), c);
        }
      }
      else
      {
        for (auto c = d.chanOffset; c < (d.chanOffset + d.nChans); c++)
        {
          SetValue(Clip(static_cast<double>(d.vals[c]), 0., 1.), c);
        }
      }

      SetDirty(false);
    }
  }
protected:
  float mHighRangeDB;
  float mLowRangeDB;
  EResponse mResponse = EResponse::Linear;
  std::vector<int> mMarkers;
};

/** Vectorial multi-channel capable meter control, with log response, held-peaks and filled-average/rms
 * Requires an IPeakAvgSender
 * @ingroup IControls */
template <int MAXNC = 1>
class IVPeakAvgMeterControl : public IVMeterControl<MAXNC>
{
public:
  IVPeakAvgMeterControl(const IRECT& bounds, const char* label, const IVStyle& style = DEFAULT_STYLE, EDirection dir = EDirection::Vertical,
                        std::initializer_list<const char*> trackNames = {}, int totalNSegs = 0, float lowRangeDB = -60.f, float highRangeDB = 12.f,
                        std::initializer_list<int> markers = {0, -6, -12, -24, -48})
  : IVMeterControl<MAXNC>(bounds, label, style, dir, trackNames, totalNSegs, IVMeterControl<MAXNC>::EResponse::Log, lowRangeDB, highRangeDB, markers)
  {
  }
  
  void DrawPeak(IGraphics& g, const IRECT& r, int chIdx, bool aboveBaseValue) override
  {
    IBlend blend = IVTrackControlBase::GetBlend();
    const float trackPos = mPeakValues[chIdx];
    
    if (trackPos < 0.0001)
      return;
    
    const auto widgetBounds = IVTrackControlBase::mWidgetBounds;
    const auto dir = IVTrackControlBase::mDirection;
    IRECT peakRect = widgetBounds.FracRect(dir, trackPos);
    
    if (dir == EDirection::Vertical)
    {
      peakRect = peakRect.GetFromTop(IVTrackControlBase::mPeakSize);
      peakRect.L = r.L;
      peakRect.R = r.R;
    }
    else
    {
      peakRect = peakRect.GetFromRight(IVTrackControlBase::mPeakSize);
      peakRect.T = r.T;
      peakRect.B = r.B;
    }
    g.FillRect(IVTrackControlBase::GetColor(kX1), peakRect, &blend);
  }

  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    if (!IVTrackControlBase::IsDisabled() && msgTag == ISender<>::kUpdateMessage)
    {
      IByteStream stream(pData, dataSize);

      int pos = 0;
      ISenderData<MAXNC, std::pair<float, float>> d;
      pos = stream.Get(&d, pos);
      
      const auto lowRangeDB = IVPeakAvgMeterControl::mLowRangeDB;
      const auto highRangeDB = IVPeakAvgMeterControl::mHighRangeDB;

      double lowPointAbs = std::fabs(lowRangeDB);
      double rangeDB = std::fabs(highRangeDB - lowRangeDB);
      
      for (auto c = d.chanOffset; c < (d.chanOffset + d.nChans); c++)
      {
        double peakValue = AmpToDB(static_cast<double>(std::get<0>(d.vals[c])));
        double avgValue = AmpToDB(static_cast<double>(std::get<1>(d.vals[c])));
        double linearPeakPos = (peakValue + lowPointAbs)/rangeDB;
        double linearAvgPos = (avgValue + lowPointAbs)/rangeDB;

        IVTrackControlBase::SetValue(Clip(linearAvgPos, 0., 1.), c);
        mPeakValues[c] = static_cast<float>(linearPeakPos);
      }

      IVTrackControlBase::SetDirty(false);
    }
  }
  
protected:
  std::array<float, MAXNC> mPeakValues;
};

const static IColor LED1 = {255, 36, 157, 16};
const static IColor LED2 = {255, 153, 191, 28};
const static IColor LED3 = {255, 215, 222, 37};
const static IColor LED4 = {255, 247, 153, 33};
const static IColor LED5 = COLOR_RED;

/** Vectorial multi-channel capable meter control with segmented LEDs, log response.
 * Requires an IPeakAvgSender
 * @ingroup IControls */
template <int MAXNC = 1>
class IVLEDMeterControl : public IVPeakAvgMeterControl<MAXNC>
{
public:
  /** LED Range comprises info for a range of LED segments */
  struct LEDRange
  {
    float lowRangeDB; // The lower bounds of the decibel range for this group of LED segments
    float highRangeDB; // The upper bounds of the decibel range for this group of LED segments
    int nSegs; // The number of LED segments
    IColor color; // The color of the LEDs in this range
    
    LEDRange(float lowRangeDB, float highRangeDB, int nSegs, IColor color)
    : lowRangeDB(lowRangeDB)
    , highRangeDB(highRangeDB)
    , nSegs(nSegs)
    , color(color)
    {
    }
  };

  IVLEDMeterControl(const IRECT& bounds, const char* label = "", const IVStyle& style = DEFAULT_STYLE, EDirection dir = EDirection::Vertical,
                    std::initializer_list<const char*> trackNames = {}, int totalNSegs = 13,
                    const std::vector<LEDRange>& ranges = {
                                                            {0., 6., 1, LED5},
                                                            {-18., 0., 3, LED4},
                                                            {-36., -18., 3, LED3},
                                                            {-54., -36., 3, LED2},
                                                            {-72., -54., 3, LED1}
                                                          })
  : IVPeakAvgMeterControl<MAXNC>(bounds, label, style, dir, trackNames, totalNSegs)
  , mLEDRanges(ranges)
  {
    IVMeterControl<MAXNC>::mZeroValueStepHasBounds = false;
    
    float minRange = 0.;
    float maxRange = 0.;
    int nSegs = 0;
    
    for (auto ledRange : ranges)
    {
      if (ledRange.lowRangeDB < minRange)
        minRange = ledRange.lowRangeDB;
      
      if (ledRange.highRangeDB > maxRange)
        maxRange = ledRange.highRangeDB;
      
      nSegs += ledRange.nSegs;
    }
    
    assert(totalNSegs == nSegs); // The ranges argument must contain the same number of segments as totalNSegs
    
    IVMeterControl<MAXNC>::mLowRangeDB = minRange;
    IVMeterControl<MAXNC>::mHighRangeDB = maxRange;
  }
  
  void DrawTrackHandle(IGraphics& g, const IRECT& r, int chIdx, bool aboveBaseValue) override
  {
    /* NO-OP, TODO: could draw peak hold */
  }
  
  void DrawTrackBackground(IGraphics &g, const IRECT &r, int chIdx) override
  {
    const int totalNSegs = IVMeterControl<MAXNC>::mNSteps;
    const EDirection dir = IVMeterControl<MAXNC>::mDirection;
    const float val = IVMeterControl<MAXNC>::GetValue(chIdx);
    const float valPos = (dir == EDirection::Vertical) ? r.B - (val * r.H()) : r.R - ((1.f-val) * r.W()); // threshold position for testing segment
    
    int segIdx = 0; // keep track of how many segments have been drawn

    for (auto ledRange : mLEDRanges)
    {
      for (auto i = 0; i < ledRange.nSegs; i++)
      {
        IRECT segRect;
        if (dir == EDirection::Vertical)
        {
          segRect = r.GetGridCell(segIdx + i, totalNSegs, 1, dir, 1);
        
          if (segRect.MH() > valPos)
            g.FillRect(ledRange.color, segRect.GetPadded(-1.f));
        }
        else
        {
          segRect = r.GetGridCell(totalNSegs - 1 - (segIdx + i), 1, totalNSegs, dir, 1);
        
          if (segRect.MW() < valPos)
            g.FillRect(ledRange.color, segRect.GetPadded(-1.f));
        }
      }
      segIdx += ledRange.nSegs;
    }
  }

private:
  std::vector<LEDRange> mLEDRanges;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
