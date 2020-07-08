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

/** Vectorial multi-channel capable meter control
 * @ingroup IControls */
template <int MAXNC = 1>
class IVMeterControl : public IVTrackControlBase
{
public:
  IVMeterControl(const IRECT& bounds, const char* label, const IVStyle& style = DEFAULT_STYLE, EDirection dir = EDirection::Vertical, std::initializer_list<const char*> trackNames = {}, int totalNSegs = 0, float lowRangeDB = -72.f, float highRangeDB = 12.f)
  : IVTrackControlBase(bounds, label, style, MAXNC, totalNSegs, dir, trackNames)
  , mLowRangeDB(lowRangeDB)
  , mHighRangeDB(highRangeDB)
  {
  }

  void Draw(IGraphics& g) override
  {
    DrawBackground(g, mRECT);
    DrawWidget(g);
    DrawLabel(g);

    if(mStyle.drawFrame)
      g.DrawRect(GetColor(kFR), mWidgetBounds, &mBlend, mStyle.frameThickness);
  }

  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    if (!IsDisabled() && msgTag == ISender<>::kUpdateMessage)
    {
      IByteStream stream(pData, dataSize);

      int pos = 0;
      ISenderData<MAXNC> d;
      pos = stream.Get(&d, pos);

      double lowPointAbs = std::fabs(mLowRangeDB);
      double rangeDB = std::fabs(mHighRangeDB - mLowRangeDB);
      for (auto c = d.chanOffset; c < (d.chanOffset + d.nChans); c++)
      {
        double ampValue = AmpToDB(static_cast<double>(d.vals[c]));
        double linearPos = (ampValue + lowPointAbs)/rangeDB;
        SetValue(Clip(linearPos, 0., 1.), c);
      }

      SetDirty(false);
    }
  }
protected:
  float mHighRangeDB;
  float mLowRangeDB;
};

const static IColor LED1 = {255, 36, 157, 16};
const static IColor LED2 = {255, 153, 191, 28};
const static IColor LED3 = {255, 215, 222, 37};
const static IColor LED4 = {255, 247, 153, 33};
const static IColor LED5 = COLOR_RED;

template <int MAXNC = 1>
class IVLEDMeterControl : public IVMeterControl<MAXNC>
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

  IVLEDMeterControl(const IRECT& bounds, int totalNSegs = 13, const std::vector<LEDRange>& ranges = {{0., 6., 1, LED5},{-18., 0., 3, LED4}, {-36., -18., 3, LED3}, {-54., -36., 3, LED2}, {-72., -54., 3, LED1}}, const char* label = "", const IVStyle& style = DEFAULT_STYLE, EDirection dir = EDirection::Vertical, std::initializer_list<const char*> trackNames = {})
  : IVMeterControl<MAXNC>(bounds, label, style, dir, trackNames, totalNSegs)
  , mLEDRanges(ranges)
  {
    IVMeterControl<MAXNC>::mZeroValueStepHasBounds = false;
    
    float minRange = 0.;
    float maxRange = 0.;
    int nSegs = 0;
    
    for (auto ledRange : ranges)
    {
      if(ledRange.lowRangeDB < minRange)
        minRange = ledRange.lowRangeDB;
      
      if(ledRange.highRangeDB > maxRange)
        maxRange = ledRange.highRangeDB;
      
      nSegs += ledRange.nSegs;
    }
    
    assert(totalNSegs == nSegs); // The ranges argument must contain the same number of segments as totalNSegs
    
    IVMeterControl<MAXNC>::mLowRangeDB = minRange;
    IVMeterControl<MAXNC>::mHighRangeDB = maxRange;
  }
  
  void Draw(IGraphics& g) override
  {
    IVMeterControl<MAXNC>::DrawBackground(g, IVMeterControl<MAXNC>::mRECT);
    IVMeterControl<MAXNC>::DrawLabel(g);
    IVMeterControl<MAXNC>::DrawWidget(g);
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
        if(dir == EDirection::Vertical)
        {
          segRect = r.GetGridCell(segIdx + i, totalNSegs, 1, dir, 1);
        
          if(segRect.MH() > valPos)
            g.FillRect(ledRange.color, segRect.GetPadded(-1.f));
        }
        else
        {
          segRect = r.GetGridCell(totalNSegs - 1 - (segIdx + i), 1, totalNSegs, dir, 1);
        
          if(segRect.MW() < valPos)
            g.FillRect(ledRange.color, segRect.GetPadded(-1.f));
        }
      }
      segIdx += ledRange.nSegs;
    }
  }
  
  void DrawTrackHandle(IGraphics &g, const IRECT &r, int chIdx, bool aboveBaseValue) override
  {
    /* NO-OP */
  }

private:
  std::vector<LEDRange> mLEDRanges;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
