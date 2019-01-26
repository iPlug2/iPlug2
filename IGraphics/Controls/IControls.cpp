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
                                 const IText& text, const IVColorSpec& colorSpec)
: IButtonControlBase(bounds, actionFunc)
, IVectorBase(colorSpec)
{
  AttachIControl(this, label);
  mDblAsSingleClick = true;
  mText = text;
}

void IVButtonControl::Draw(IGraphics& g)
{
  DrawBackGround(g, mRECT);
  DrawWidget(g);
  DrawTitle(g);
  DrawValue(g);
}

void IVButtonControl::DrawWidget(IGraphics& g)
{
  DrawVectorButton(g, mWidgetBounds, (bool) mValue, mMouseIsOver);
}

void IVButtonControl::OnResize()
{
  SetTargetRECT(CalculateRects(mRECT, mTitleStr.Get(), "", true));
  SetDirty(false);
}

bool IVButtonControl::IsHit(float x, float y) const
{
  return mWidgetBounds.Contains(x, y);
}

IVSwitchControl::IVSwitchControl(IRECT bounds, int paramIdx, const char* label, const IVColorSpec& colorSpec)
  : ISwitchControlBase(bounds, paramIdx, FlashCircleClickActionFunc)
  , IVectorBase(colorSpec)
{
  AttachIControl(this, label);
  mDblAsSingleClick = true;
}

IVSwitchControl::IVSwitchControl(IRECT bounds, IActionFunction actionFunc, const char* label, const IVColorSpec& colorSpec, int numStates)
: ISwitchControlBase(bounds, kNoParameter, actionFunc, numStates)
, IVectorBase(colorSpec)
{
  AttachIControl(this, label);
  mDblAsSingleClick = true;
}

void IVSwitchControl::Draw(IGraphics& g)
{
  DrawBackGround(g, mRECT);
  DrawWidget(g);
  DrawTitle(g);
  DrawValue(g);
}

void IVSwitchControl::DrawWidget(IGraphics& g)
{
  DrawVectorButton(g, mWidgetBounds, (bool) mValue, mMouseIsOver);
}

void IVSwitchControl::SetDirty(bool push)
{
  IControl::SetDirty(push);

  const IParam* pParam = GetParam();

  if(pParam)
    pParam->GetDisplayForHost(mValueStr);
}

void IVSwitchControl::OnResize()
{
  SetTargetRECT(CalculateRects(mRECT, mTitleStr.Get(), "", true));
  SetDirty(false);
}

bool IVSwitchControl::IsHit(float x, float y) const
{
  return mWidgetBounds.Contains(x, y);
}

IVRadioButtonControl::IVRadioButtonControl(IRECT bounds, int paramIdx, IActionFunction actionFunc,
  const IVColorSpec& colorSpec, int numStates, EDirection dir)
: ISwitchControlBase(bounds, paramIdx, actionFunc, numStates)
, IVectorBase(colorSpec)
, mDirection(dir)
{
  AttachIControl(this, "");//TODO
  mDblAsSingleClick = true;
  mText.mAlign = IText::kAlignNear;
  mText.mVAlign = IText::kVAlignMiddle;
  mDrawShadows = false;

  if(GetParam())
  {
    for (int i = 0; i < mNumStates; i++)
    {
      mLabels.Add(new WDL_String(GetParam()->GetDisplayText(i)));
    }
  }
}

void IVRadioButtonControl::Draw(IGraphics& g)
{
  DrawWidget(g);
}

void IVRadioButtonControl::DrawWidget(IGraphics& g)
{
  int hit = int(0.5 + mValue * (double) (mNumStates - 1));
  
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
  mButtons.Resize(0);
  
  for (int i = 0; i < mNumStates; i++)
  {
    mButtons.Add(mRECT.SubRect(mDirection, mNumStates, i));
  }
}

IVKnobControl::IVKnobControl(IRECT bounds, int paramIdx,
                             const char* label,
                             const IVColorSpec& colorSpec,
                             float aMin, float aMax,
                             EDirection direction, double gearing)
: IKnobControlBase(bounds, paramIdx, direction, gearing)
, IVectorBase(colorSpec)
, mAngleMin(aMin)
, mAngleMax(aMax)
{
  //  if(mDisplayParamValue) // TODO: wrong
//    DisablePrompt(false);
  
  AttachIControl(this, label);
}

IVKnobControl::IVKnobControl(IRECT bounds, IActionFunction actionFunction,
                             const char* label,
                             const IVColorSpec& colorSpec,
                             float aMin, float aMax,
                             EDirection direction, double gearing)
: IKnobControlBase(bounds, kNoParameter, direction, gearing)
, IVectorBase(colorSpec)
, mAngleMin(aMin)
, mAngleMax(aMax)
{
//  if(mDisplayParamValue) // TODO: wrong
//    DisablePrompt(false);

  SetActionFunction(actionFunction);
  AttachIControl(this, label);
}

void IVKnobControl::Draw(IGraphics& g)
{
  DrawBackGround(g, mRECT);
  DrawWidget(g);
  DrawTitle(g);
  DrawValue(g);
}

void IVKnobControl::DrawWidget(IGraphics& g)
{
  const float radius = (mWidgetBounds.H()/2.f);
  const float cx = mWidgetBounds.MW(), cy = mWidgetBounds.MH();
  
  if(!IsGrayed())
  {
    const float v = mAngleMin + ((float)mValue * (mAngleMax - mAngleMin));
    
    g.DrawArc(GetColor(kFR), cx, cy, radius + 5.f, mAngleMin, v, 0, 3.f);
    
    if(mDrawShadows && !mEmboss)
      g.FillCircle(GetColor(kSH), cx + mShadowOffset, cy + mShadowOffset, radius);
    
    g.FillCircle(GetColor(kFG), cx, cy, radius);
    
    g.DrawCircle(GetColor(kON), cx, cy, radius * 0.9f, 0, mFrameThickness);
    
    if(mMouseIsOver)
      g.FillCircle(GetColor(kHL), cx, cy, radius * 0.8f);
    
    g.DrawCircle(GetColor(kFR), cx, cy, radius, 0, mFrameThickness);
    g.DrawRadialLine(GetColor(kFR), cx, cy, v, 0.7f * radius, 0.9f * radius, 0, mFrameThickness);
  }
  else
  {
    g.FillCircle(GetColor(kOFF), cx, cy, radius);
  }
}

void IVKnobControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  if(mDisplayParamValue && mValueBounds.Contains(x, y))
  {
    PromptUserInput(mValueBounds);
  }
  else
    IKnobControlBase::OnMouseDown(x, y, mod);
}

void IVKnobControl::OnResize()
{
  SetTargetRECT(CalculateRects(mRECT, mTitleStr.Get()));
  SetDirty(false);
}

bool IVKnobControl::IsHit(float x, float y) const
{
  return mWidgetBounds.Contains(x, y);
}

IVSliderControl::IVSliderControl(IRECT bounds, int paramIdx,
                const char* label,
                const IVColorSpec& colorSpec,
                EDirection dir, bool onlyHandle, float handleSize, float trackSize)
: ISliderControlBase(bounds, paramIdx, dir, onlyHandle, handleSize)
, IVectorBase(colorSpec)
, mTrackSize(trackSize)
{
  AttachIControl(this, label);
}

IVSliderControl::IVSliderControl(IRECT bounds, IActionFunction aF,
                const char* label,
                const IVColorSpec& colorSpec,
                EDirection dir, bool onlyHandle, float handleSize, float trackSize)
: ISliderControlBase(bounds, aF, dir, onlyHandle, handleSize)
, IVectorBase(colorSpec)
, mTrackSize(trackSize)
{
  AttachIControl(this, label);
}

void IVSliderControl::Draw(IGraphics& g)
{
  DrawBackGround(g, mRECT);
  DrawWidget(g);
  DrawTitle(g);
  DrawValue(g);
}

void IVSliderControl::DrawWidget(IGraphics& g)
{
  const float halfHandleSize = mHandleSize / 2.f;

  //track
  IRECT filledTrack = mTrack.FracRect(mDirection, (float) mValue);
  
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
  if(mDrawShadows && !mEmboss)
    g.FillCircle(GetColor(kSH), cx + mShadowOffset, cy + mShadowOffset, halfHandleSize);
  
  g.FillCircle(GetColor(kFG), cx, cy, halfHandleSize);
  
  if(GetMouseIsOver())
    g.FillCircle(GetColor(kHL), cx, cy, halfHandleSize);
  
  g.DrawCircle(GetColor(kFR), cx, cy, halfHandleSize, 0, mFrameThickness);
  g.DrawCircle(GetColor(kON), cx, cy, halfHandleSize * 0.7f, 0, mFrameThickness);
}

void IVSliderControl::OnResize()
{
  CalculateRects(mRECT, mTitleStr.Get());
  
  if(mDirection == kVertical)
    mTrack = mWidgetBounds.GetPadded(-mHandleSize).GetMidHPadded(mTrackSize);
  else
    mTrack = mWidgetBounds.GetPadded(-mHandleSize).GetMidVPadded(mTrackSize);

  SetDirty(false);
}

#pragma mark - BITMAP CONTROLS

void IBSwitchControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  if (mBitmap.N() > 1)
    mValue += 1.0 / (double)(mBitmap.N() - 1);
  else
    mValue += 1.0;

  if (mValue > 1.001)
    mValue = 0.0;

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
    mTrack = mRECT.GetPadded(0, (float) bitmap.H(), 0, 0);
  }
  else
  {
    mRECT = mTargetRECT = IRECT(x, y, x + len, y + bitmap.H());
    mTrack = mRECT.GetPadded(0, 0, (float) bitmap.W(), 0);
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
    value = mValue;
  
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

