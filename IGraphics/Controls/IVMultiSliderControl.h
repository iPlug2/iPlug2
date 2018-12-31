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

  IVMultiSliderControl(IGEditorDelegate& dlg, IRECT bounds, float minTrackValue = 0.f, float maxTrackValue = 1.f, const char* trackNames = 0, ...)
  : IVTrackControlBase(dlg, bounds, MAXNC, minTrackValue, maxTrackValue, trackNames)
  {
    mOuterPadding = 0.f;
    mDrawTrackFrame = false;
    mTrackPadding = 1.f;
    SetColor(kFG, COLOR_BLACK);
  }

  void SnapToMouse(float x, float y, EDirection direction, IRECT& bounds, float scalar = 1.) override //TODO: fixed for horizontal
  {
    bounds.Constrain(x, y);

    float yValue = (y-bounds.T) / bounds.H();

    yValue = std::round( yValue / mGrain ) * mGrain;

    int sliderTest = -1;

    for(auto i = 0; i < MaxNTracks(); i++)
    {
      if(mTrackBounds.Get()[i].Contains(x, mTrackBounds.Get()[i].MH()))
      {
        sliderTest = i;
        break;
      }
    }

    if (sliderTest > -1)
    {
      float* trackValue = GetTrackData(sliderTest);
      *trackValue = mMinTrackValue + (1.f - Clip(yValue, 0.f, 1.f)) * (mMaxTrackValue - mMinTrackValue);
      OnNewValue(sliderTest, *trackValue);

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
            trackValue = GetTrackData(i);
            float frac = (float)(i - lowBounds) / float(highBounds-lowBounds);
            *trackValue = LERP(*GetTrackData(lowBounds), *GetTrackData(highBounds), frac);
            OnNewValue(i, *trackValue);
          }
        }
      }
      mPrevSliderHit = mSliderHit;
    }
    else
    {
      mSliderHit = -1;
    }

    SetDirty();
  }

  //  void OnResize() override;
  //  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override;

  virtual void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    IRECT innerBounds = mRECT.GetPadded(-mOuterPadding);

    if (!mod.S)
      mPrevSliderHit = -1;

    SnapToMouse(x, y, mDirection, innerBounds);
  }

  virtual void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
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
