/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc IVMultiSliderControl
 */

#include "IControl.h"

#define LERP(a,b,f) ((b-a)*f+a)

/** A vectorial multi-slider control
 * @ingroup IControls */
template <int MAXNC = 1>
class IVMultiSliderControl : public IVTrackControlBase
{
public:

  IVMultiSliderControl(IRECT bounds, const char* label, const IVStyle& style = DEFAULT_STYLE, EDirection dir = kVertical, float minTrackValue = 0.f, float maxTrackValue = 1.f, const char* trackNames = 0, ...)
  : IVTrackControlBase(bounds, label, style, MAXNC, dir, minTrackValue, maxTrackValue, trackNames)
  {
    mOuterPadding = 0.f;
    mDrawTrackFrame = false;
    mTrackPadding = 1.f;
  }

  IVMultiSliderControl(IRECT bounds, const char* label, const IVStyle& style, int loParamIdx, EDirection dir, float minTrackValue, float maxTrackValue, const char* trackNames = 0, ...)
  : IVTrackControlBase(bounds, label, style, loParamIdx, MAXNC, dir, minTrackValue, maxTrackValue, trackNames)
  {
    mOuterPadding = 0.f;
    mDrawTrackFrame = false;
    mTrackPadding = 1.f;
  }
  
  void Draw(IGraphics& g) override
  {
    DrawBackGround(g, mRECT);
    DrawWidget(g);
    DrawLabel(g);
    
    if(mStyle.drawFrame)
      g.DrawRect(GetColor(kFR), mWidgetBounds, nullptr, mStyle.frameThickness);
  }

  int GetValIdxForPos(float x, float y) const override
  {
    int nVals = NVals();
    
    for (auto v = 0; v < nVals; v++)
    {
      if (mTrackBounds.Get()[v].Contains(x, y))
      {
        return v;
      }
    }

    return kNoValIdx;
  }

  void SnapToMouse(float x, float y, EDirection direction, IRECT& bounds, int valIdx = -1 /* TODO:: not used*/, float scalar = 1.) override
  {
    bounds.Constrain(x, y);
    int nVals = NVals();

    float value = 0.;
    int sliderTest = -1;

    if(direction == kVertical)
    {
      value = 1. - (y-bounds.T) / bounds.H();
      
      for(auto i = 0; i < nVals; i++)
      {
        if(mTrackBounds.Get()[i].Contains(x, mTrackBounds.Get()[i].MH()))
        {
          sliderTest = i;
          break;
        }
      }
    }
    else
    {
      value = (x-bounds.L) / bounds.W();
      
      for(auto i = 0; i < nVals; i++)
      {
        if(mTrackBounds.Get()[i].Contains(mTrackBounds.Get()[i].MW(), y))
        {
          sliderTest = i;
          break;
        }
      }
    }
    
    value = std::round( value / mGrain ) * mGrain;
    
    if (sliderTest > -1)
    {
      SetValue(mMinTrackValue + Clip(value, 0.f, 1.f) * (mMaxTrackValue - mMinTrackValue), sliderTest);
      OnNewValue(sliderTest, GetValue(sliderTest));

      mSliderHit = sliderTest;

      if (mPrevSliderHit != -1)
      {
        if (abs(mPrevSliderHit - mSliderHit) > 1 /*|| shiftClicked*/)
        {
          int lowBounds, highBounds;

          if (mPrevSliderHit < mSliderHit)
          {
            lowBounds = mPrevSliderHit;
            highBounds = mSliderHit;
          }
          else
          {
            lowBounds = mSliderHit;
            highBounds = mPrevSliderHit;
          }

          for (auto i = lowBounds; i < highBounds; i++)
          {
            float frac = (float)(i - lowBounds) / float(highBounds-lowBounds);
            SetValue(LERP(GetValue(lowBounds), GetValue(highBounds), frac), i);
            OnNewValue(i, GetValue(i));
          }
        }
      }
      mPrevSliderHit = mSliderHit;
    }
    else
    {
      mSliderHit = -1;
    }

    SetDirty(true); // will send all param vals parameter value to delegate
  }

  void OnResize() override
  {
    SetTargetRECT(CalculateRects(mRECT));
    MakeTrackRects(mWidgetBounds);
    SetDirty(false);
  }
  
  //  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override;

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    IRECT innerBounds = mRECT.GetPadded(-mOuterPadding);

    if (!mod.S)
      mPrevSliderHit = -1;

    SnapToMouse(x, y, mDirection, innerBounds);
  }

  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
  {
    IRECT innerBounds = mRECT.GetPadded(-mOuterPadding);

    SnapToMouse(x, y, mDirection, innerBounds);
  }

  //override to do something when an individual slider is dragged
  virtual void OnNewValue(int trackIdx, float val) {}

protected:
  int mPrevSliderHit = -1;
  int mSliderHit = -1;
  float mGrain = 0.001f;
};
