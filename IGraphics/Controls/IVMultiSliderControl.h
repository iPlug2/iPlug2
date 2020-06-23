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
   * @param nSteps The number of cross-axis steps for integer quantized grids
   * @param direction The direction of the sliders */
  IVMultiSliderControl(const IRECT& bounds, const char* label, const IVStyle& style = DEFAULT_STYLE, int nSteps = 0, EDirection dir = EDirection::Vertical)
  : IVTrackControlBase(bounds, label, style, MAXNC, nSteps, dir)
  {
    mDrawTrackFrame = false;
    mTrackPadding = 1.f;
  }

  /** Constructs a vector multi slider control that is linked to sequential parameters
   * @param bounds The control's bounds
   * @param label The label for the vector control, leave empty for no label
   * @param style The styling of this vector control \see IVStyle
   * @param loParamIdx The parameter index for the first slider in the multislider. The total number of sliders/parameters covered depends on the template argument, and is contiguous from loParamIdx
   * @param nSteps The number of cross-axis steps for integer quantized grids
   * @param direction The direction of the sliders */
  IVMultiSliderControl(const IRECT& bounds, const char* label, const IVStyle& style, int loParamIdx, int nSteps, EDirection dir)
  : IVTrackControlBase(bounds, label, style, loParamIdx, MAXNC, nSteps, dir)
  {
    mDrawTrackFrame = false;
    mTrackPadding = 1.f;
  }
  
  /** Constructs a vector multi slider control that is linked to a list of parameters that need not be sequential
   * @param bounds The control's bounds
   * @param label The label for the vector control, leave empty for no label
   * @param style The styling of this vector control \see IVStyle
   * @param params List of parameter indexes, the length of which should match the template argument
   * @param nSteps The number of cross-axis steps for integer quantized grids
   * @param direction The direction of the sliders */
  IVMultiSliderControl(const IRECT& bounds, const char* label, const IVStyle& style, const std::initializer_list<int>& params, int nSteps, EDirection dir)
  : IVTrackControlBase(bounds, label, style, params, nSteps, dir)
  {
    mDrawTrackFrame = false;
    mTrackPadding = 1.f;
  }
  
  void Draw(IGraphics& g) override
  {
    DrawBackground(g, mRECT);
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
        
        if(mStepBounds.GetSize() == 1)
          value = 1.f;
        else
          value = step * (1.f/float(mStepBounds.GetSize()-1));
      }
      else
      {
        value = 1.f - (y-bounds.T) / bounds.H();
      }
      
      for(auto i = 0; i < nVals; i++)
      {
        if(mTrackBounds.Get()[i].ContainsEdge(x, mTrackBounds.Get()[i].MH()))
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
        
        if(mStepBounds.GetSize() == 1)
          value = 1.f;
        else
          value = 1.- (step * (1.f/float(mStepBounds.GetSize()-1)));
      }
      else
      {
        value = (x-bounds.L) / bounds.W();
      }
      for(auto i = 0; i < nVals; i++)
      {
        if(mTrackBounds.Get()[i].ContainsEdge(mTrackBounds.Get()[i].MW(), y))
        {
          sliderTest = i;
          break;
        }
      }
    }
        
    if(!GetStepped())
       value = std::round(value / mGrain) * mGrain;
    
    if (sliderTest > -1)
    {
      SetValue(Clip(value, 0., 1.), sliderTest);
      OnNewValue(sliderTest, GetValue(sliderTest));

      mSliderHit = sliderTest;
      mMouseOverTrack = mSliderHit;
      
      if (!GetStepped() && mPrevSliderHit != -1) // LERP disabled when stepped
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

    SetDirty(true); // will send all param vals to delegate
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    // This allows single clicking to remove a step entry when !mZeroValueStepHasBounds
    if(GetStepped() && !mZeroValueStepHasBounds && mPrevSliderHit != -1)
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
            OnNewValue(ch, 0.);
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

  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    if (!IsDisabled() && msgTag == kMsgTagSetHighlight && dataSize == sizeof(int))
    {
      SetHighlightedTrack(*reinterpret_cast<const int*>(pData));
    }
  }

  /** override to do something when an individual slider is dragged */
  virtual void OnNewValue(int trackIdx, double val) {}
  
protected:
  int mPrevSliderHit = -1;
  int mSliderHit = -1;
  double mGrain = 0.001;
};

/** A vectorial multi-toggle control, could be used for a trigger in a step sequencer or tarnce gate
* @ingroup IControls */
template <int MAXNC = 1>
class IVMultiToggleControl : public IVMultiSliderControl<MAXNC>
{
public:
  IVMultiToggleControl(const IRECT& bounds, const char* label, const IVStyle& style = DEFAULT_STYLE, EDirection dir = EDirection::Vertical)
  : IVMultiSliderControl<MAXNC>(bounds, label, style, 1, dir)
  {
    IVMultiSliderControl<MAXNC>::mZeroValueStepHasBounds = false;
    IVMultiSliderControl<MAXNC>::mDblAsSingleClick = true;
  }
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
