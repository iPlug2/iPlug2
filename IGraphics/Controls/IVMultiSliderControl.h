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
  static constexpr int kMsgTagSetHighlight = 0;
  
  /** Constructs a vector multi slider control that is not linked to parameters
     * @param bounds The control's bounds
     * @param label The label for the vector control, leave empty for no label
     * @param style The styling of this vector control \see IVStyle
     * @param direction The direction of the sliders */
  IVMultiSliderControl(const IRECT& bounds, const char* label, const IVStyle& style = DEFAULT_STYLE, int nSteps = 0, EDirection dir = EDirection::Vertical)
  : IVTrackControlBase(bounds, label, style, MAXNC, nSteps, dir)
  {
    mDrawTrackFrame = false;
    mTrackPadding = 1.f;
  }

  /** Constructs a vector multi slider control that is linked to parameters
   * @param bounds The control's bounds
   * @param label The label for the vector control, leave empty for no label
   * @param style The styling of this vector control \see IVStyle
   * @param loParamIdx The parameter index for the first slider in the multislider. The total number of sliders/parameters covered depends on the template argument, and is contiguous from loParamIdx
   * @param direction The direction of the sliders */
  IVMultiSliderControl(const IRECT& bounds, const char* label, const IVStyle& style, int loParamIdx, int nSteps, EDirection dir)
  : IVTrackControlBase(bounds, label, style, loParamIdx, MAXNC, nSteps, dir)
  {
    mDrawTrackFrame = false;
    mTrackPadding = 1.f;
  }
  
  IVMultiSliderControl(const IRECT& bounds, const char* label, const IVStyle& style, const std::initializer_list<int>& params, int nSteps, EDirection dir)
  : IVTrackControlBase(bounds, label, style, params, nSteps, dir)
  {
    mDrawTrackFrame = false;
    mTrackPadding = 1.f;
  }
  
  void Draw(IGraphics& g) override
  {
    DrawBackGround(g, mRECT);
    DrawWidget(g);
    DrawLabel(g);
    
    if(mStyle.drawFrame)
      g.DrawRect(GetColor(kFR), mWidgetBounds, &mBlend, mStyle.frameThickness);
  }

  void SnapToMouse(float x, float y, EDirection direction, const IRECT& bounds, int valIdx = -1 /* TODO:: not used*/, double minClip = 0., double maxClip = 1.) override
  {
    bounds.Constrain(x, y);
    int nVals = NVals();

    double value = 0.;
    int sliderTest = -1;
    
    int step = GetStepIdxForPos(x, y);
        
    if(direction == EDirection::Vertical)
    {
      if(step > -1)
      {
        y = mStepBounds.Get()[step].T;
      }
      
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
      if(step > -1)
      {
        x = mStepBounds.Get()[step].L;
      }
      
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
    
    const bool noSteps = mStepBounds.GetSize() == 0;
    
    if(noSteps)
       value = std::round(value / mGrain) * mGrain;
    
    if (sliderTest > -1)
    {
      SetValue(Clip(value, 0., 1.), sliderTest);
      OnNewValue(sliderTest, GetValue(sliderTest));

      mSliderHit = sliderTest;
      mMouseOverTrack = mSliderHit;
      
      if (noSteps && mPrevSliderHit != -1) // disable LERP when steps
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
            double frac = (double)(i - lowBounds) / double(highBounds-lowBounds);
            SetValue(iplug::Lerp(GetValue(lowBounds), GetValue(highBounds), frac), i);
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

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if(mStepBounds.GetSize() && mPrevSliderHit != -1)
    {
      int ch = GetValIdxForPos(x, y);
      
      if(ch > -1)
      {
        int step = GetStepIdxForPos(x, y);
        
        if(step > -1)
        {
          float y = mStepBounds.Get()[step].T;
          double valueAtStep = 1.f - (y-mWidgetBounds.T) / mWidgetBounds.H();

          if(GetValue(ch) == valueAtStep)
          {
            SetValue(0., ch);
            SetDirty(true);
            return;
          }
        }
      }
    }

    if (!mod.S)
      mPrevSliderHit = -1;
      
    SnapToMouse(x, y, mDirection, mWidgetBounds);    
  }

  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
  {
    SnapToMouse(x, y, mDirection, mWidgetBounds);
  }

  //override to do something when an individual slider is dragged
  virtual void OnNewValue(int trackIdx, double val) {}

  void DrawTrackHandle(IGraphics& g, const IRECT& r, int chIdx, bool aboveBaseValue) override
  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    if (!IsDisabled() && msgTag == kMsgTagSetHighlight && dataSize == sizeof(int))
    {
      SetHighlightedTrack(*reinterpret_cast<const int*>(pData));
    }
  }
  
protected:
  int mPrevSliderHit = -1;
  int mSliderHit = -1;
  double mGrain = 0.001;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
