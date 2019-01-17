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

  IVMultiSliderControl(IRECT bounds, float minTrackValue = 0.f, float maxTrackValue = 1.f, const char* trackNames = 0, ...)
  : IVTrackControlBase(bounds, MAXNC, minTrackValue, maxTrackValue, trackNames)
  {
    mOuterPadding = 0.f;
    mDrawTrackFrame = false;
    mTrackPadding = 1.f;
    SetColor(kFG, COLOR_BLACK);
  }

  IVMultiSliderControl(IRECT bounds, int loParamIdx, float minTrackValue = 0.f, float maxTrackValue = 1.f, const char* trackNames = 0, ...)
    : IVTrackControlBase(bounds, loParamIdx, MAXNC, minTrackValue, maxTrackValue, trackNames)
  {
    mOuterPadding = 0.f;
    mDrawTrackFrame = false;
    mTrackPadding = 1.f;
    SetColor(kFG, COLOR_BLACK);
  }

  virtual int GetValIdxForPos(float x, float y) const override // TODO fixed for horizontal
  {
    for (auto i = 0; i < MaxNTracks(); i++)
    {
      if (mTrackBounds.Get()[i].Contains(x, mTrackBounds.Get()[i].MH()))
      {
        return i;
      }
    }

    return kNoValIdx;
  }

  void SnapToMouse(float x, float y, EDirection direction, IRECT& bounds, int valIdx = -1 /* TODO:: not used*/, float scalar = 1.) override //TODO: fixed for horizontal
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
      SetValue(mMinTrackValue + (1.f - Clip(yValue, 0.f, 1.f)) * (mMaxTrackValue - mMinTrackValue), sliderTest);
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
