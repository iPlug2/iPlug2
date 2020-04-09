/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc IVNumberBoxControl
 */

#include "IControl.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** A "meta control" for a number box with an Inc/Dec button
 * It adds several child buttons 
 * @ingroup IControls */
class IVNumberBoxControl : public IControl
                         , public IVectorBase
{
public:
  IVNumberBoxControl(const IRECT& bounds, int paramIdx = kNoParameter, IActionFunction actionFunc = nullptr, const char* label = "", const IVStyle& style = DEFAULT_STYLE, double defaultValue = 50.f, double minValue = 1.f, double maxValue = 100.f, const char* fmtStr = "%0.0f")
  : IControl(bounds, paramIdx, actionFunc)
  , IVectorBase(style.WithDrawShadows(false)
                .WithValueText(style.valueText.WithVAlign(EVAlign::Middle)))
  , mFmtStr(fmtStr)
  , mMinValue(minValue)
  , mMaxValue(maxValue)
  , mRealValue(defaultValue)
  {
    assert(defaultValue >= minValue && defaultValue <= maxValue);
    
    AttachIControl(this, label);
  }
   
  void OnInit() override
  {
    if(GetParam())
    {
      mMinValue = GetParam()->GetMin();
      mMaxValue = GetParam()->GetMax();
      mRealValue = GetParam()->GetDefault();
    }
  }
  
  void Draw(IGraphics& g) override
  {
    DrawLabel(g);
    
    if(mMouseIsOver)
      g.FillRect(GetColor(kHL), mTextReadout->GetRECT());
  }
  
  void OnResize() override
  {
    MakeRects(mRECT, false);
    
    if(mIncButton && mDecButton)
    {
      IRECT sections = mWidgetBounds;
      mTextReadout->SetTargetAndDrawRECTs(sections.ReduceFromLeft(sections.W() * 0.75f));
      sections.Pad(-1.f, 1.f, 0.f, 1.f);
      mIncButton->SetTargetAndDrawRECTs(sections.FracRectVertical(0.5f, true));
      mDecButton->SetTargetAndDrawRECTs(sections.FracRectVertical(0.5f, false));
      SetTargetRECT(mTextReadout->GetRECT());
    }
  }
  
  void OnAttached() override
  {
    IRECT sections = mWidgetBounds;
    GetUI()->AttachControl(mTextReadout = new IVLabelControl(sections.ReduceFromLeft(sections.W() * 0.75f), "0", mStyle.WithDrawFrame(true)));
    
    mTextReadout->SetStrFmt(32, mFmtStr.Get(), mRealValue);
    
    sections.Pad(-1.f, 1.f, 0.f, 1.f);
    GetUI()->AttachControl(mIncButton = new IVButtonControl(sections.FracRectVertical(0.5f, true), SplashClickActionFunc, "+", mStyle))->SetAnimationEndActionFunction(mIncrementFunc);
    GetUI()->AttachControl(mDecButton = new IVButtonControl(sections.FracRectVertical(0.5f, false), SplashClickActionFunc, "-", mStyle))->SetAnimationEndActionFunction(mDecrementFunc);
  }
  
  void OnMouseDown(float x, float y, const IMouseMod &mod) override
  {
    if (mHideCursorOnDrag)
      GetUI()->HideMouseCursor(true, true);

    if(GetParam())
      GetDelegate()->BeginInformHostOfParamChangeFromUI(GetParamIdx());
  }
  
  void OnMouseUp(float x, float y, const IMouseMod &mod) override
  {
    if (mHideCursorOnDrag)
      GetUI()->HideMouseCursor(false);
    
    if(GetParam())
      GetDelegate()->EndInformHostOfParamChangeFromUI(GetParamIdx());
  }
  
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod &mod) override
  {
    double gearing = IsFineControl(mod, true) ? mSmallIncrement : mLargeIncrement;
    mRealValue -= (double(dY) * gearing);
    OnValueChanged();
  }
  
  void OnMouseDblClick(float x, float y, const IMouseMod &mod) override
  {
    if(mTextReadout->GetRECT().Contains(x, y))
      GetUI()->CreateTextEntry(*this, mText, mTextReadout->GetRECT(), mTextReadout->GetStr());
  }
  
  void OnTextEntryCompletion(const char* str, int valIdx) override
  {
    mRealValue = atof(str);
    OnValueChanged();
  }
  
  void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override
  {
    double gearing = IsFineControl(mod, true) ? mSmallIncrement : mLargeIncrement;
    double inc = (d > 0.f ? 1. : -1.) * gearing;
    mRealValue += inc;
    OnValueChanged();
  }
  
  void SetValueFromDelegate(double value, int valIdx = 0) override
  {
    if(GetParam())
    {
      mRealValue = GetParam()->FromNormalized(value);
      OnValueChanged(true);
    }
    
    IControl::SetValueFromDelegate(value, valIdx);
  }
  
  void SetValueFromUserInput(double value, int valIdx = 0) override
  {
    if(GetParam())
    {
      mRealValue = GetParam()->FromNormalized(value);
      OnValueChanged(true);
    }
    
    IControl::SetValueFromUserInput(value, valIdx);
  }
  
  void SetStyle(const IVStyle& style) override
  {
    IVectorBase::SetStyle(style);
    mTextReadout->SetStyle(style);
    mIncButton->SetStyle(style);
    mDecButton->SetStyle(style);
  }
  
  bool IsFineControl(const IMouseMod& mod, bool wheel) const
  {
    #ifdef PROTOOLS
    #ifdef OS_WIN
      return mod.C;
    #else
      return wheel ? mod.C : mod.R;
    #endif
    #else
      return (mod.C || mod.S);
    #endif
  }
  
  void OnValueChanged(bool preventAction = false)
  {
    mRealValue = Clip(mRealValue, mMinValue, mMaxValue);
    
    mTextReadout->SetStrFmt(32, mFmtStr.Get(), mRealValue);
    
    if(!preventAction && GetParam())
      SetValue(GetParam()->ToNormalized(mRealValue));
    
    SetDirty(!preventAction);
  }
  
  double GetRealValue() const { return mRealValue; }
  
protected:
  
  IActionFunction mIncrementFunc = [this](IControl* pCaller) { mRealValue += mLargeIncrement; OnValueChanged(); };
  IActionFunction mDecrementFunc = [this](IControl* pCaller) { mRealValue -= mLargeIncrement; OnValueChanged(); };
  IVLabelControl* mTextReadout = nullptr;
  IVButtonControl* mIncButton = nullptr;
  IVButtonControl* mDecButton = nullptr;
  WDL_String mFmtStr;
  double mLargeIncrement = 1.f;
  double mSmallIncrement = 0.1f;
  double mMinValue;
  double mMaxValue;
  double mRealValue = 0.f;
  bool mHideCursorOnDrag = true;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
