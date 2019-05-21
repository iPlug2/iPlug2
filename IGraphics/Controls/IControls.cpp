/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

/**
 * @file
 * @brief IControls implementation
 * @ingroup Controls
 */

#include "IControls.h"

#pragma mark - VECTOR CONTROLS

const IColor IVKeyboardControl::DEFAULT_BK_COLOR = IColor(255, 70, 70, 70);
const IColor IVKeyboardControl::DEFAULT_WK_COLOR = IColor(255, 240, 240, 240);
const IColor IVKeyboardControl::DEFAULT_PK_COLOR = IColor(60, 0, 0, 0);
const IColor IVKeyboardControl::DEFAULT_FR_COLOR = DEFAULT_BK_COLOR;

IVButtonControl::IVButtonControl(IRECT bounds, IActionFunction actionFunc,
                                 const char* label,
                                 const IVStyle& style,
                                 bool labelInButton)
: IButtonControlBase(bounds, actionFunc)
, IVectorBase(style, labelInButton)
{
  mText = style.valueText;
  AttachIControl(this, label);
  mDblAsSingleClick = true;
}

void IVButtonControl::Draw(IGraphics& g)
{
  DrawBackGround(g, mRECT);
  DrawWidget(g);
  DrawTitle(g);
  DrawValue(g, false);
}

void IVButtonControl::DrawWidget(IGraphics& g)
{
  DrawVectorButton(g, mWidgetBounds, (bool) GetValue(), mMouseIsOver);
}

void IVButtonControl::OnResize()
{
  SetTargetRECT(CalculateRects(mRECT, mLabelStr.Get(), ""));
  SetDirty(false);
}

bool IVButtonControl::IsHit(float x, float y) const
{
  return mWidgetBounds.Contains(x, y);
}

IVTriangleButtonControl::IVTriangleButtonControl(IRECT bounds, IActionFunction actionFunc,
                                                 const char* label, const IVStyle& style,
                                                 float angle)
: IVButtonControl(bounds, actionFunc, label, style)
, mAngle(angle)
{};

void IVTriangleButtonControl::DrawWidget(IGraphics& g)
{
  DrawVectorTriangleButton(g, mWidgetBounds, mAngle, (bool) GetValue(), mMouseIsOver);
}

IVSwitchControl::IVSwitchControl(IRECT bounds, int paramIdx, const char* label, const IVStyle& style, bool valueInButton)
  : ISwitchControlBase(bounds, paramIdx, SplashClickActionFunc)
  , IVectorBase(style, false, valueInButton)
{
  AttachIControl(this, label);
  mText = style.valueText;
  
  if(valueInButton)
    mText.mVAlign = mStyle.valueText.mVAlign = IText::kVAlignMiddle;
  
  mDblAsSingleClick = true;
}

IVSwitchControl::IVSwitchControl(IRECT bounds, IActionFunction actionFunc, const char* label, const IVStyle& style, int numStates, bool valueInButton)
: ISwitchControlBase(bounds, kNoParameter, actionFunc, numStates)
, IVectorBase(style, false, valueInButton)
{
  AttachIControl(this, label);
  mText = style.valueText;
  
  if(valueInButton)
    mText.mVAlign = mStyle.valueText.mVAlign = IText::kVAlignMiddle;

  mDblAsSingleClick = true;
}

void IVSwitchControl::Draw(IGraphics& g)
{
  DrawBackGround(g, mRECT);
  DrawWidget(g);
  DrawTitle(g);
  DrawValue(g, false);
}

void IVSwitchControl::DrawWidget(IGraphics& g)
{
  DrawVectorButton(g, mWidgetBounds, mMouseDown, mMouseIsOver);
}

void IVSwitchControl::SetDirty(bool push, int valIdx)
{
  IControl::SetDirty(push);

  const IParam* pParam = GetParam();

  if(pParam)
    pParam->GetDisplayForHost(mValueStr);
}

void IVSwitchControl::OnResize()
{
  SetTargetRECT(CalculateRects(mRECT, mLabelStr.Get(), ""));
  SetDirty(false);
}

bool IVSwitchControl::IsHit(float x, float y) const
{
  return mWidgetBounds.Contains(x, y);
}

IVRadioButtonControl::IVRadioButtonControl(IRECT bounds, int paramIdx, IActionFunction actionFunc,
  const char* label, const IVStyle& style, int numStates, EDirection dir)
: ISwitchControlBase(bounds, paramIdx, actionFunc, numStates)
, IVectorBase(style)
, mDirection(dir)
{
  AttachIControl(this, label);
  mDblAsSingleClick = true;
  mText = style.valueText;
  mText.mAlign = IText::kAlignNear; //TODO?
  mText.mVAlign = IText::kVAlignMiddle; //TODO?
  mStyle.drawShadows = false;  //TODO?

  if(GetParam())
  {
    for (int i = 0; i < mNumStates; i++)
    {
      mLabels.Add(new WDL_String(GetParam()->GetDisplayText(i)));
    }
  }
}

IVRadioButtonControl::IVRadioButtonControl(IRECT bounds, IActionFunction actionFunc,
                                           const std::initializer_list<const char*>& options,
                                           const char* label, const IVStyle& style, EDirection dir)
: ISwitchControlBase(bounds, kNoParameter, actionFunc, options.size())
, IVectorBase(style)
, mDirection(dir)
{
  AttachIControl(this, label);
  mDblAsSingleClick = true;
  mText = style.valueText;
  mText.mAlign = IText::kAlignNear; //TODO?
  mText.mVAlign = IText::kVAlignMiddle; //TODO?
  mStyle.drawShadows = false;  //TODO?
  
  for (auto& option : options) {
    mLabels.Add(new WDL_String(option));
  }
}

void IVRadioButtonControl::Draw(IGraphics& g)
{
  DrawTitle(g);
  DrawWidget(g);
}

void IVRadioButtonControl::DrawWidget(IGraphics& g)
{
  int hit = int(0.5 + GetValue() * (double) (mNumStates - 1));
  
  for (int i = 0; i < mNumStates; i++)
  {
    IRECT r = mButtons.Get()[i];
    DrawVectorButton(g, r.FracRectHorizontal(0.25f).GetCentredInside(10.f), i == hit , mMouseIsOver);
    
    if (mLabels.Get(i))
    {
      r = r.FracRectHorizontal(0.7f, true);
      i == hit ? mText.mFGColor = COLOR_WHITE : mText.mFGColor = COLOR_BLACK;
      g.DrawText(mText, mLabels.Get(i)->Get(), r);
    }
  }
}

//bool IVRadioButtonControl::IsHit(float x, float y) const
//{
//  bool hit = false;
//  
//  for (int i = 0; i < mNumStates; i++)
//  {
//    hit |= mButtons.Get()[i].Contains(x, y);
//  }
//  
//  return hit;
//}

void IVRadioButtonControl::OnResize()
{
  SetTargetRECT(CalculateRects(mRECT, mLabelStr.Get()));
  
  mButtons.Resize(0);
  
  for (int i = 0; i < mNumStates; i++)
  {
    mButtons.Add(mWidgetBounds.SubRect(mDirection, mNumStates, i));
  }
  
  SetDirty(false);
}

IVKnobControl::IVKnobControl(IRECT bounds, int paramIdx,
                             const char* label,
                             const IVStyle& style,
                             bool valueIsEditable,
                             float aMin, float aMax,
                             EDirection direction, double gearing)
: IKnobControlBase(bounds, paramIdx, direction, gearing)
, IVectorBase(style)
, mAngleMin(aMin)
, mAngleMax(aMax)
{
  DisablePrompt(!valueIsEditable);
  mText = style.valueText;
  AttachIControl(this, label);
}

IVKnobControl::IVKnobControl(IRECT bounds, IActionFunction actionFunction,
                             const char* label,
                             const IVStyle& style,
                             bool valueIsEditable,
                             float aMin, float aMax,
                             EDirection direction, double gearing)
: IKnobControlBase(bounds, kNoParameter, direction, gearing)
, IVectorBase(style)
, mAngleMin(aMin)
, mAngleMax(aMax)
{
  DisablePrompt(!valueIsEditable);
  mText = style.valueText;
  SetActionFunction(actionFunction);
  AttachIControl(this, label);
}

void IVKnobControl::Draw(IGraphics& g)
{
  DrawBackGround(g, mRECT);
  DrawWidget(g);
  DrawTitle(g);
  DrawValue(g, mValueMouseOver);
}

void IVKnobControl::DrawWidget(IGraphics& g)
{
  float radius;
  if(mWidgetBounds.W() > mWidgetBounds.H())
    radius = (mWidgetBounds.H()/2.f);
  else
    radius = (mWidgetBounds.W()/2.f);
  
  const float cx = mWidgetBounds.MW(), cy = mWidgetBounds.MH();
  
  if(!IsGrayed())
  {
    const float v = mAngleMin + ((float) GetValue() * (mAngleMax - mAngleMin));
    
    g.DrawArc(GetColor(kFR), cx, cy, radius + 5.f, mAngleMin, v, 0, 3.f);
    
    if(mStyle.drawShadows && !mStyle.emboss)
      g.FillCircle(GetColor(kSH), cx + mStyle.shadowOffset, cy + mStyle.shadowOffset, radius);
    
    g.FillCircle(GetColor(kFG), cx, cy, radius);
    
    g.DrawCircle(GetColor(kON), cx, cy, radius * 0.9f, 0, mStyle.frameThickness);
    
    if(mMouseIsOver && !mValueMouseOver)
      g.FillCircle(GetColor(kHL), cx, cy, radius * 0.8f);
    
    g.DrawCircle(GetColor(kFR), cx, cy, radius, 0, mStyle.frameThickness);
    g.DrawRadialLine(GetColor(kFR), cx, cy, v, 0.7f * radius, 0.9f * radius, 0, mStyle.frameThickness);
  }
  else
  {
    g.FillCircle(GetColor(kOFF), cx, cy, radius);
  }
}

void IVKnobControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  if(mStyle.showValue && mValueBounds.Contains(x, y))
  {
    PromptUserInput(mValueBounds);
  }
  else
    IKnobControlBase::OnMouseDown(x, y, mod);
}

void IVKnobControl::OnMouseOver(float x, float y, const IMouseMod& mod)
{
  if(mStyle.showValue && !mDisablePrompt)
    mValueMouseOver = mValueBounds.Contains(x,y);
  
  IKnobControlBase::OnMouseOver(x, y, mod);
}

void IVKnobControl::OnResize()
{
  SetTargetRECT(CalculateRects(mRECT, mLabelStr.Get()));
  SetDirty(false);
}

bool IVKnobControl::IsHit(float x, float y) const
{
  if(!mDisablePrompt)
  {
    if(mValueBounds.Contains(x,y))
      return true;
  }
  
  return mWidgetBounds.Contains(x, y);
}

IVSliderControl::IVSliderControl(IRECT bounds, int paramIdx,
                const char* label,
                const IVStyle& style,
                bool valueIsEditable,
                EDirection dir, bool onlyHandle, float handleSize, float trackSize)
: ISliderControlBase(bounds, paramIdx, dir, onlyHandle, handleSize)
, IVectorBase(style)
, mTrackSize(trackSize)
{
  DisablePrompt(!valueIsEditable);
  mText = style.valueText;
  AttachIControl(this, label);
}

IVSliderControl::IVSliderControl(IRECT bounds, IActionFunction aF,
                const char* label,
                const IVStyle& style,
                bool valueIsEditable,
                EDirection dir, bool onlyHandle, float handleSize, float trackSize)
: ISliderControlBase(bounds, aF, dir, onlyHandle, handleSize)
, IVectorBase(style)
, mTrackSize(trackSize)
{
  DisablePrompt(!valueIsEditable);
  mText = style.valueText;
  AttachIControl(this, label);
}

void IVSliderControl::Draw(IGraphics& g)
{
  DrawBackGround(g, mRECT);
  DrawWidget(g);
  DrawTitle(g);
  DrawValue(g, mValueMouseOver);
}

void IVSliderControl::DrawWidget(IGraphics& g)
{
  const float halfHandleSize = mHandleSize / 2.f;

  //track
  IRECT filledTrack = mTrack.FracRect(mDirection, (float) GetValue());

  g.FillRect(GetColor(kFR), mTrack);
  g.FillRect(GetColor(kFG), filledTrack);
  
  g.DrawRect(GetColor(kFR), mTrack);
  
  float cx, cy;
  
  if(mDirection == kVertical)
  {
    cx = filledTrack.MW();
    cy = filledTrack.T;
  }
  else
  {
    cx = filledTrack.R;
    cy = filledTrack.MH();
  }
  
  //Handle
  if(mStyle.drawShadows && !mStyle.emboss)
    g.FillCircle(GetColor(kSH), cx + mStyle.shadowOffset, cy + mStyle.shadowOffset, halfHandleSize);
  
  g.FillCircle(GetColor(kFG), cx, cy, halfHandleSize);
  
  if(GetMouseIsOver())
    g.FillCircle(GetColor(kHL), cx, cy, halfHandleSize);
  
  g.DrawCircle(GetColor(kFR), cx, cy, halfHandleSize, 0, mStyle.frameThickness);
  g.DrawCircle(GetColor(kON), cx, cy, halfHandleSize * 0.7f, 0, mStyle.frameThickness);
}

void IVSliderControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  if(mStyle.showValue && mValueBounds.Contains(x, y))
  {
    PromptUserInput(mValueBounds);
  }
  else
    ISliderControlBase::OnMouseDown(x, y, mod);
}

void IVSliderControl::OnMouseOver(float x, float y, const IMouseMod& mod)
{
  if(mStyle.showValue && !mDisablePrompt)
    mValueMouseOver = mValueBounds.Contains(x,y);
  
  ISliderControlBase::OnMouseOver(x, y, mod);
}

void IVSliderControl::OnResize()
{
  SetTargetRECT(CalculateRects(mRECT, mLabelStr.Get()));
  
  if(mDirection == kVertical)
    mTrack = mWidgetBounds.GetPadded(-mHandleSize).GetMidHPadded(mTrackSize);
  else
    mTrack = mWidgetBounds.GetPadded(-mHandleSize).GetMidVPadded(mTrackSize);

  SetDirty(false);
}

bool IVSliderControl::IsHit(float x, float y) const
{
  if(!mDisablePrompt)
  {
    if(mValueBounds.Contains(x,y))
    {
      return true;
    }
  }
  
  return mWidgetBounds.Contains(x, y);
}


IVRangeSliderControl::IVRangeSliderControl(IRECT bounds, int paramIdxLo, int paramIdxHi)
: IVSliderControl(bounds)
{
  SetNVals(2);
  SetParamIdx(paramIdxLo, 0);
  SetParamIdx(paramIdxHi, 1);

  mTrackSize = bounds.W();
}

void IVRangeSliderControl::Draw(IGraphics & g)
{
  g.FillRect(GetColor(kBG), mRECT);

  //const float halfHandleSize = mHandleSize / 2.f;

  //track
  const float minVal = (float) std::min(GetValue(0), GetValue(1));
  const float maxVal = (float) std::max(GetValue(0), GetValue(1));

  IRECT filledTrack = { mTrack.L, mTrack.B - (maxVal * mTrack.H()), mTrack.R, mTrack.B - (minVal * mTrack.H()) };

  g.FillRect(GetColor(kFR), mTrack);
  g.FillRect(GetColor(kFG), filledTrack);
  g.DrawRect(GetColor(kFR), mTrack);

  float cx[2];
  float cy[2];

  if (mDirection == kVertical)
  {
    cx[0] = cx[1] = filledTrack.MW();
    cy[0] = filledTrack.T;
    cy[1] = filledTrack.B;
  }
  else
  {
    cx[0] = filledTrack.L;
    cx[1] = filledTrack.R;
    cy[0] = cy[1] = filledTrack.MH();
  }

  ////Handles
  //for (int i = 0; i < 2; i++)
  //{
  //  if (mDrawShadows && !mEmboss)
  //    g.FillCircle(GetColor(kSH), cx[i] + mShadowOffset, cy[i] + mShadowOffset, halfHandleSize);

  //  g.FillCircle(GetColor(kFG), cx[i], cy[i], halfHandleSize);

  //  if (GetMouseIsOver())
  //    g.FillCircle(GetColor(kHL), cx[i], cy[i], halfHandleSize);

  //  g.DrawCircle(GetColor(kFR), cx[i], cy[i], halfHandleSize, 0, mFrameThickness);
  //  g.DrawCircle(GetColor(kON), cx[i], cy[i], halfHandleSize * 0.7f, 0, mFrameThickness);
  //}
}

void IVRangeSliderControl::OnMouseDown(float x, float y, const IMouseMod & mod)
{
  SnapToMouse(x, y, mDirection, mTrack);
}

void IVRangeSliderControl::OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod & mod)
{
  SnapToMouse(x, y, mDirection, mTrack);
}

#pragma mark - BITMAP CONTROLS

void IBSwitchControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  if (mBitmap.N() > 1)
    SetValue(GetValue() + 1.0 / (double)(mBitmap.N() - 1));
  else
    SetValue(GetValue() + 1.0);

  if (GetValue() > 1.001)
    SetValue(0.);

  SetDirty();
}

IBSliderControl::IBSliderControl(IRECT bounds, int paramIdx, const IBitmap& bitmap,
                                 EDirection dir, bool onlyHandle)
: ISliderControlBase(bounds, paramIdx, dir, onlyHandle)
, IBitmapBase(bitmap)
{
  mTrack = bounds; // TODO: check
}

IBSliderControl::IBSliderControl(float x, float y, int len, int paramIdx, const IBitmap& bitmap, EDirection dir, bool onlyHandle)
: ISliderControlBase(IRECT(x, y, x + bitmap.W(), y + len), paramIdx)
, IBitmapBase(bitmap)
{
  if (dir == kVertical)
  {
    mRECT = mTargetRECT = IRECT(x, y, x + bitmap.W(), y + len);
    mTrack = mRECT.GetPadded(0, -(float) bitmap.H(), 0, 0);
  }
  else
  {
    mRECT = mTargetRECT = IRECT(x, y, x + len, y + bitmap.H());
    mTrack = mRECT.GetPadded(0, 0, -(float) bitmap.W(), 0);
  }
}

void IBSliderControl::Draw(IGraphics& g)
{
  IRECT r = GetHandleBounds();
  g.DrawBitmap(mBitmap, r, 1, &mBlend);
}

void IBSliderControl::OnRescale()
{
  mBitmap = GetUI()->GetScaledBitmap(mBitmap);
}

void IBSliderControl::OnResize()
{
  SetDirty(false);
}

IRECT IBSliderControl::GetHandleBounds(double value) const
{
  if (value < 0.0)
    value = GetValue();
  
  IRECT r(mRECT.L, mRECT.T, mRECT.L + mBitmap.W(), mRECT.T + mBitmap.H());

  if (mDirection == kVertical)
  {
    float offs = (1.f - (float) value) * mTrack.H();
    r.T += offs;
    r.B += offs;
  }
  else
  {
    float offs = (float) value * mTrack.W();
    r.L += offs;
    r.R += offs;
  }
  return r;
}
