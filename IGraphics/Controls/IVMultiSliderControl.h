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

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** A vectorial multi-slider control
 * @ingroup IControls */
template <int MAXNC = 1>
class IVMultiSliderControl : public IVTrackControlBase
{
public:

  /** Constructs a vector multi slider control that is not linked to parameters
     * @param bounds The control's bounds
     * @param label The label for the vector control, leave empty for no label
     * @param style The styling of this vector control \see IVStyle
     * @param direction The direction of the sliders
     * @param minTrackValue Defines the minimum value of each slider
     * @param maxTrackValue Defines the maximum value of each slider */
  IVMultiSliderControl(const IRECT& bounds, const char* label, const IVStyle& style = DEFAULT_STYLE, EDirection dir = EDirection::Vertical, float minTrackValue = 0.f, float maxTrackValue = 1.f)
  : IVTrackControlBase(bounds, label, style, MAXNC, dir, minTrackValue, maxTrackValue)
  {
    mOuterPadding = 0.f;
    mDrawTrackFrame = false;
    mTrackPadding = 1.f;
  }

  /** Constructs a vector multi slider control that is linked to parameters
   * @param bounds The control's bounds
   * @param label The label for the vector control, leave empty for no label
   * @param style The styling of this vector control \see IVStyle
   * @param loParamIdx The parameter index for the first slider in the multislider. The total number of sliders/parameters covered depends on the template argument, and is contiguous from loParamIdx
   * @param direction The direction of the sliders
   * @param minTrackValue Defines the minimum value of each slider
   * @param maxTrackValue Defines the maximum value of each slider */
  IVMultiSliderControl(const IRECT& bounds, const char* label, const IVStyle& style, int loParamIdx, EDirection dir, float minTrackValue, float maxTrackValue) //FIXME: float minTrackValue, float maxTrackValue?
  : IVTrackControlBase(bounds, label, style, loParamIdx, MAXNC, dir, minTrackValue, maxTrackValue)
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

  void SnapToMouse(float x, float y, EDirection direction, const IRECT& bounds, int valIdx = -1 /* TODO:: not used*/, float scalar = 1.f, double minClip = 0., double maxClip = 1.) override
  {
    bounds.Constrain(x, y);
    int nVals = NVals();

    float value = 0.;
    int sliderTest = -1;

    if(direction == EDirection::Vertical)
    {
      value = 1.f - (y-bounds.T) / bounds.H();
      
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
      OnNewValue(sliderTest, static_cast<float>(GetValue(sliderTest)));

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
            SetValue(linearInterp(GetValue(lowBounds), GetValue(highBounds), frac), i);
            OnNewValue(i, static_cast<float>(GetValue(i)));
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
    
private:
    
  inline double linearInterp(double a, double b, double f) const { return ((b - a) * f + a); }
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
