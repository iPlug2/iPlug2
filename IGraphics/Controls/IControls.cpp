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

IVButtonControl::IVButtonControl(const IRECT& bounds, IActionFunction actionFunc,
                                 const char* label,
                                 const IVStyle& style,
                                 bool labelInButton, bool valueInButton, EVShape shape, float angle)
: IButtonControlBase(bounds, actionFunc)
, IVectorBase(style, labelInButton, valueInButton)
, mShape(shape)
, mAngle(angle)
{
  mText = style.valueText;
  AttachIControl(this, label);
  mDblAsSingleClick = true;
}

void IVButtonControl::Draw(IGraphics& g)
{
  DrawBackGround(g, mRECT);
  DrawWidget(g);
  DrawLabel(g);
  DrawValue(g, false);
}

void IVButtonControl::DrawWidget(IGraphics& g)
{
  bool pressed = (bool) GetValue();
  switch (mShape)
  {
    case EVShape::Ellipse:
      DrawPressableEllipse(g, mWidgetBounds, pressed, mMouseIsOver);
      break;
    case EVShape::Rectangle:
      DrawPressableRectangle(g, mWidgetBounds, pressed, mMouseIsOver);
      break;
    case EVShape::Triangle:
      DrawPressableTriangle(g, mWidgetBounds, pressed, mMouseIsOver);
      break;
    case EVShape::EndsRounded:
      DrawPressableRectangle(g, mWidgetBounds, pressed, mMouseIsOver, true, true, false, false);
      break;
    case EVShape::AllRounded:
      DrawPressableRectangle(g, mWidgetBounds, pressed, mMouseIsOver, true, true, true, true);
    default:
      break;
  }
}

void IVButtonControl::OnResize()
{
  SetTargetRECT(CalculateRects(mRECT, true));
  SetDirty(false);
}

bool IVButtonControl::IsHit(float x, float y) const
{
  return mWidgetBounds.Contains(x, y);
}

IVSwitchControl::IVSwitchControl(const IRECT& bounds, int paramIdx, const char* label, const IVStyle& style, bool valueInButton)
  : ISwitchControlBase(bounds, paramIdx, SplashClickActionFunc)
  , IVectorBase(style, false, valueInButton)
{
  AttachIControl(this, label);
  mText = style.valueText;
  
  if(valueInButton)
    mText.mVAlign = mStyle.valueText.mVAlign = EVAlign::Middle;
  
  mDblAsSingleClick = true;
}

IVSwitchControl::IVSwitchControl(const IRECT& bounds, IActionFunction actionFunc, const char* label, const IVStyle& style, int numStates, bool valueInButton)
: ISwitchControlBase(bounds, kNoParameter, actionFunc, numStates)
, IVectorBase(style, false, valueInButton)
{
  AttachIControl(this, label);
  mText = style.valueText;
  
  if(valueInButton)
    mText.mVAlign = mStyle.valueText.mVAlign = EVAlign::Middle;

  mDblAsSingleClick = true;
}

void IVSwitchControl::Draw(IGraphics& g)
{
  DrawBackGround(g, mRECT);
  DrawWidget(g);
  DrawLabel(g);
  DrawValue(g, false);
}

void IVSwitchControl::DrawWidget(IGraphics& g)
{
  DrawPressableRectangle(g, mWidgetBounds, mMouseDown, mMouseIsOver);
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
  SetTargetRECT(CalculateRects(mRECT, true));
  SetDirty(false);
}

bool IVSwitchControl::IsHit(float x, float y) const
{
  return mWidgetBounds.Contains(x, y);
}

IVToggleControl::IVToggleControl(const IRECT& bounds, int paramIdx, const char* label, const IVStyle& style, const char* offText, const char* onText)
: IVSwitchControl(bounds, paramIdx, label, style, true)
, mOnText(onText)
, mOffText(offText)
{
  //TODO: assert boolean?
}

IVToggleControl::IVToggleControl(const IRECT& bounds, IActionFunction actionFunc, const char* label, const IVStyle& style, const char* offText, const char* onText, bool initialState)
: IVSwitchControl(bounds, actionFunc, label, style, 2, true)
, mOnText(onText)
, mOffText(offText)
{
  SetValue((double) initialState);
}

void IVToggleControl::DrawWidget(IGraphics& g)
{
  DrawPressableRectangle(g, mWidgetBounds, GetValue() > 0.5, mMouseIsOver);
}

void IVToggleControl::DrawValue(IGraphics& g, bool mouseOver)
{
  if(mouseOver)
    g.FillRect(COLOR_TRANSLUCENT, mValueBounds);
  
  if(GetValue() > 0.5)
    g.DrawText(mStyle.valueText, mOnText.Get(), mValueBounds);
  else
    g.DrawText(mStyle.valueText, mOffText.Get(), mValueBounds);
}

IVTabSwitchControl::IVTabSwitchControl(const IRECT& bounds, int paramIdx, IActionFunction actionFunc,
                                       const char* label, const IVStyle& style, EVShape shape, EDirection direction)
: ISwitchControlBase(bounds, paramIdx, actionFunc)
, IVectorBase(style)
, mShape(shape)
, mDirection(direction)
{
  AttachIControl(this, label);
  mDblAsSingleClick = true;
  mText = style.valueText;
  mText.mAlign = EAlign::Center; //TODO?
  mText.mVAlign = EVAlign::Middle; //TODO?
}

IVTabSwitchControl::IVTabSwitchControl(const IRECT& bounds, IActionFunction actionFunc,
                                       const std::initializer_list<const char*>& options,
                                       const char* label, const IVStyle& style, EVShape shape, EDirection direction)
: ISwitchControlBase(bounds, kNoParameter, actionFunc, static_cast<int>(options.size()))
, IVectorBase(style)
, mShape(shape)
, mDirection(direction)
{
  AttachIControl(this, label);
  mDblAsSingleClick = true;
  mText = style.valueText;
  mText.mAlign = mStyle.valueText.mAlign = EAlign::Center; //TODO?
  mText.mVAlign = mStyle.valueText.mVAlign = EVAlign::Middle; //TODO?
  
  for (auto& option : options) {
    mTabLabels.Add(new WDL_String(option));
  }
}

void IVTabSwitchControl::OnInit()
{
  ISwitchControlBase::OnInit();
  
  const IParam* pParam = GetParam();
  
  if(pParam)
  {
    for (int i = 0; i < mNumStates; i++)
    {
      mTabLabels.Add(new WDL_String(GetParam()->GetDisplayText(i)));
    }
  }
}

void IVTabSwitchControl::Draw(IGraphics& g)
{
  DrawBackGround(g, mRECT);
  DrawLabel(g);
  DrawWidget(g);
}

void IVTabSwitchControl::DrawButton(IGraphics& g, const IRECT& r, bool pressed, bool mouseOver, ETabSegment segment)
{
  switch (mShape)
  {
    case EVShape::Ellipse:
      DrawPressableEllipse(g, r, pressed, mouseOver);
      break;
    case EVShape::Rectangle:
      DrawPressableRectangle(g, r, pressed, mouseOver);
      break;
    case EVShape::Triangle:
      DrawPressableTriangle(g, r, pressed, mouseOver);
      break;
    case EVShape::EndsRounded:
      if(mDirection == EDirection::Horizontal)
        DrawPressableRectangle(g, r, pressed, mouseOver, segment == ETabSegment::Start, segment == ETabSegment::End, false, false);
      else
        DrawPressableRectangle(g, r, pressed, mouseOver, false, segment == ETabSegment::Start, false, segment == ETabSegment::End);
      break;
    case EVShape::AllRounded:
      if(mDirection == EDirection::Horizontal)
        DrawPressableRectangle(g, r, pressed, mouseOver, true, true, false, false);
      else
        DrawPressableRectangle(g, r, pressed, mouseOver, false, true, false, true);
      break;
    default:
      break;
  }
}

void IVTabSwitchControl::DrawWidget(IGraphics& g)
{
  int hit = GetSelectedIdx();
  ETabSegment segment = ETabSegment::Start;

  for (int i = 0; i < mNumStates; i++)
  {
    IRECT r = mButtons.Get()[i];
    
    if(i > 0)
      segment = ETabSegment::Mid;
    
    if(i == mNumStates-1)
      segment = ETabSegment::End;

    DrawButton(g, r, i == hit, mMouseOverButton == i, segment);
    
    if (mTabLabels.Get(i))
    {
      g.DrawText(mText, mTabLabels.Get(i)->Get(), r);
    }
  }
}

bool IVTabSwitchControl::IsHit(float x, float y) const
{
  bool hit = false;
  
  for (int i = 0; i < mNumStates; i++)
  {
    hit |= mButtons.Get()[i].Contains(x, y);
  }
  
  return hit;
}

void IVTabSwitchControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  int hit = -1;
  
  for (int i = 0; i < mNumStates; i++)
  {
    if(mButtons.Get()[i].Contains(x, y))
    {
      hit = i;
      break;
    }
  }
  
  if(hit > -1)
    SetValue(((double) hit * (1./(double) (mNumStates-1))));
  
  SetDirty(true);
}

void IVTabSwitchControl::OnMouseOver(float x, float y, const IMouseMod& mod)
{
  mMouseOverButton = -1;
  
  for (int i = 0; i < mNumStates; i++)
  {
    if(mButtons.Get()[i].Contains(x, y))
    {
      mMouseOverButton = i;
      break;
    }
  }
  
  ISwitchControlBase::OnMouseOver(x, y, mod);
  
  SetDirty(false);
}

void IVTabSwitchControl::OnResize()
{
  SetTargetRECT(CalculateRects(mRECT));
  
  mButtons.Resize(0);
  
  for (int i = 0; i < mNumStates; i++)
  {
    mButtons.Add(mWidgetBounds.SubRect(mDirection, mNumStates, i));
  }
  
  SetDirty(false);
}

IVRadioButtonControl::IVRadioButtonControl(const IRECT& bounds, int paramIdx, IActionFunction actionFunc,
                                           const char* label, const IVStyle& style, EVShape shape, EDirection direction, float buttonSize)
: IVTabSwitchControl(bounds, paramIdx, actionFunc, label, style, shape, direction)
, mButtonSize(buttonSize)
{
  mText.mAlign = EAlign::Near; //TODO?
}

IVRadioButtonControl::IVRadioButtonControl(const IRECT& bounds, IActionFunction actionFunc,
                                           const std::initializer_list<const char*>& options,
                                           const char* label, const IVStyle& style, EVShape shape, EDirection direction, float buttonSize)
: IVTabSwitchControl(bounds, actionFunc, options, label, style, shape, direction)
, mButtonSize(buttonSize)
{
  mText.mAlign = EAlign::Near; //TODO?
}

void IVRadioButtonControl::DrawWidget(IGraphics& g)
{
  int hit = GetSelectedIdx();
  
  for (int i = 0; i < mNumStates; i++)
  {
    IRECT r = mButtons.Get()[i];
    
    DrawButton(g, r.FracRectHorizontal(0.25).GetCentredInside(mButtonSize), i == hit, mMouseOverButton == i, ETabSegment::Mid);
    
    if (mTabLabels.Get(i))
    {
      r = r.FracRectHorizontal(0.7f, true);
      i == hit ? mText.mFGColor = GetColor(kON) : mText.mFGColor = mStyle.valueText.mFGColor;
      g.DrawText(mText, mTabLabels.Get(i)->Get(), r);
    }
  }
}

bool IVRadioButtonControl::IsHit(float x, float y) const
{
  bool hit = false;
  
  for (int i = 0; i < mNumStates; i++)
  {
    hit |= mButtons.Get()[i].FracRectHorizontal(0.25f).Contains(x, y);
  }
  
  return hit;
}

void IVRadioButtonControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  int hit = -1;
  
  for (int i = 0; i < mNumStates; i++)
  {
    if(mButtons.Get()[i].FracRectHorizontal(0.25f).Contains(x, y))
    {
      hit = i;
      break;
    }
  }
  
  if(hit > -1)
    SetValue(((double) hit * (1./(double) (mNumStates-1))));
  
  SetDirty(true);
}

void IVRadioButtonControl::OnMouseOver(float x, float y, const IMouseMod& mod)
{
  mMouseOverButton = -1;
  
  for (int i = 0; i < mNumStates; i++)
  {
    if(mButtons.Get()[i].FracRectHorizontal(0.25f).Contains(x, y))
    {
      mMouseOverButton = i;
      break;
    }
  }
  
  IVTabSwitchControl::OnMouseOver(x, y, mod);
  
  SetDirty(false);
}

void IVRadioButtonControl::OnResize()
{
  SetTargetRECT(CalculateRects(mRECT));
  
  mButtons.Resize(0);
  
  for (int i = 0; i < mNumStates; i++)
  {
    mButtons.Add(mWidgetBounds.SubRect(mDirection, mNumStates, i));
  }
  
  SetDirty(false);
}

IVKnobControl::IVKnobControl(const IRECT& bounds, int paramIdx,
                             const char* label,
                             const IVStyle& style,
                             bool valueIsEditable,
                             float aMin, float aMax, float aAnchor,
                             EDirection direction, double gearing)
: IKnobControlBase(bounds, paramIdx, direction, gearing)
, IVectorBase(style)
, mAngleMin(aMin)
, mAngleMax(aMax)
, mAnchorAngle(aAnchor)
{
  DisablePrompt(!valueIsEditable);
  mText = style.valueText;
  AttachIControl(this, label);
}

IVKnobControl::IVKnobControl(const IRECT& bounds, IActionFunction actionFunction,
                             const char* label,
                             const IVStyle& style,
                             bool valueIsEditable,
                             float aMin, float aMax, float aAnchor,
                             EDirection direction, double gearing)
: IKnobControlBase(bounds, kNoParameter, direction, gearing)
, IVectorBase(style)
, mAngleMin(aMin)
, mAngleMax(aMax)
, mAnchorAngle(aAnchor)
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
  DrawLabel(g);
  DrawValue(g, mValueMouseOver);
}

void IVKnobControl::DrawWidget(IGraphics& g)
{
  float radius;
  
  if(mWidgetBounds.W() > mWidgetBounds.H())
    radius = (mWidgetBounds.H()/2.f /*TODO: fix bodge*/);
  else
    radius = (mWidgetBounds.W()/2.f /*TODO: fix bodge*/);
  
  const float cx = mWidgetBounds.MW(), cy = mWidgetBounds.MH();
  
  if(!IsGrayed())
  {
    /*TODO: constants! */
    const float v = mAngleMin + ((float) GetValue() * (mAngleMax - mAngleMin));
    
    g.DrawArc(GetColor(kX1), cx, cy, (radius * 0.8f) + 3.f, v >= mAnchorAngle ? mAnchorAngle : mAnchorAngle - (mAnchorAngle-v), v >= mAnchorAngle ? v : mAnchorAngle, 0, 2.f);
    
    DrawPressableCircle(g, mWidgetBounds, radius * 0.8f, false/*mMouseDown*/, mMouseIsOver & !mValueMouseOver);
    
    g.DrawCircle(GetColor(kHL), cx, cy, radius * 0.7f);

    if(mMouseDown)
      g.FillCircle(GetColor(kON), cx, cy, radius * 0.7f);
    
    g.DrawRadialLine(GetColor(kFR), cx, cy, v, 0.6f * radius, 0.8f * radius, 0, mStyle.frameThickness >= 1.f ? mStyle.frameThickness : 1.f);
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
  {
    if(mStyle.hideCursor)
      GetUI()->HideMouseCursor(true, true);
    
    IKnobControlBase::OnMouseDown(x, y, mod);
  }
}

void IVKnobControl::OnMouseUp(float x, float y, const IMouseMod& mod)
{
  if(mStyle.hideCursor)
    GetUI()->HideMouseCursor(false);

  IKnobControlBase::OnMouseUp(x, y, mod);

  SetDirty(true);
}

void IVKnobControl::OnMouseOver(float x, float y, const IMouseMod& mod)
{
  if(mStyle.showValue && !mDisablePrompt)
    mValueMouseOver = mValueBounds.Contains(x,y);
  
  IKnobControlBase::OnMouseOver(x, y, mod);
}

void IVKnobControl::OnResize()
{
  SetTargetRECT(CalculateRects(mRECT));
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

void IVKnobControl::SetDirty(bool push, int valIdx)
{
  IKnobControlBase::SetDirty(push);
  
  const IParam* pParam = GetParam();
  
  if(pParam)
    pParam->GetDisplayForHostWithLabel(mValueStr);
}

void IVKnobControl::OnInit()
{
  const IParam* pParam = GetParam();
  
  if(pParam)
    pParam->GetDisplayForHostWithLabel(mValueStr);
}

IVSliderControl::IVSliderControl(const IRECT& bounds, int paramIdx,
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

IVSliderControl::IVSliderControl(const IRECT& bounds, IActionFunction aF,
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
  DrawLabel(g);
  DrawValue(g, mValueMouseOver);
}

void IVSliderControl::DrawTrack(IGraphics& g, const IRECT& filledArea)
{
  g.FillRect(GetColor(kSH), mTrack);
  g.FillRect(GetColor(kX1), filledArea);
  
  if(mStyle.drawFrame)
    g.DrawRect(GetColor(kFR), mTrack, nullptr, mStyle.frameThickness);
}

void IVSliderControl::DrawHandle(IGraphics& g, const IRECT& handleBounds)
{
  switch (mShape)
  {
    case EVShape::Ellipse:
      DrawPressableEllipse(g, handleBounds, mMouseDown, mMouseIsOver);
      break;
    case EVShape::Rectangle:
      DrawPressableRectangle(g, handleBounds, mMouseDown, mMouseIsOver);
      break;
    case EVShape::Triangle:
      DrawPressableTriangle(g, handleBounds, mMouseDown, mMouseIsOver);
      break;
    case EVShape::EndsRounded:
      DrawPressableRectangle(g, handleBounds, mMouseDown, mMouseIsOver, true, true, false, false);
      break;
    case EVShape::AllRounded:
      DrawPressableRectangle(g, handleBounds, mMouseDown, mMouseIsOver, true, true, true, true);
    default:
      break;
  }
}

void IVSliderControl::DrawWidget(IGraphics& g)
{
  IRECT filledTrack = mTrack.FracRect(mDirection, (float) GetValue());

  if(!mOnlyHandle)
    DrawTrack(g, filledTrack);
  
  float cx, cy;
  
  const float offset = (mStyle.drawShadows && mShape != EVShape::Ellipse /* TODO? */) ? mStyle.shadowOffset * 0.5f : 0.;
  
  if(mDirection == EDirection::Vertical)
  {
    cx = filledTrack.MW() + offset;
    cy = filledTrack.T;
  }
  else
  {
    cx = filledTrack.R;
    cy = filledTrack.MH() + offset;
  }
  
  IRECT handleBounds = IRECT(cx-mHandleSize, cy-mHandleSize, cx+mHandleSize, cy+mHandleSize);

  DrawHandle(g, handleBounds);
}

void IVSliderControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  if(mStyle.showValue && mValueBounds.Contains(x, y))
  {
    PromptUserInput(mValueBounds);
  }
  else
  {
    if(mStyle.hideCursor)
      GetUI()->HideMouseCursor(true, false);
    
    ISliderControlBase::OnMouseDown(x, y, mod);
  }
}

void IVSliderControl::OnMouseUp(float x, float y, const IMouseMod& mod)
{
  if(mStyle.hideCursor)
    GetUI()->HideMouseCursor(false);
  
  ISliderControlBase::OnMouseUp(x, y, mod);

  SetDirty(true);
}

void IVSliderControl::OnMouseOver(float x, float y, const IMouseMod& mod)
{
  if(mStyle.showValue && !mDisablePrompt)
    mValueMouseOver = mValueBounds.Contains(x,y);
  
  ISliderControlBase::OnMouseOver(x, y, mod);
}

void IVSliderControl::OnResize()
{
  SetTargetRECT(CalculateRects(mRECT));
  
  if(mDirection == EDirection::Vertical)
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

void IVSliderControl::SetDirty(bool push, int valIdx)
{
  ISliderControlBase::SetDirty(push);
  
  const IParam* pParam = GetParam();
  
  if(pParam)
    pParam->GetDisplayForHostWithLabel(mValueStr);
}

void IVSliderControl::OnInit()
{
  const IParam* pParam = GetParam();
  
  if(pParam)
    pParam->GetDisplayForHostWithLabel(mValueStr);
}

IVRangeSliderControl::IVRangeSliderControl(const IRECT& bounds, int paramIdxLo, int paramIdxHi,
                                           const char* label,
                                           const IVStyle& style,
//                                           bool valueIsEditable = false,
                                           EDirection dir, bool onlyHandle, float handleSize, float trackSize)
: IVSliderControl(bounds, paramIdxLo, label, style, false, dir, onlyHandle, handleSize, trackSize)
{
  SetNVals(2);
  SetParamIdx(paramIdxLo, 0);
  SetParamIdx(paramIdxHi, 1);
}

void IVRangeSliderControl::DrawWidget(IGraphics & g)
{
  g.FillRect(GetColor(kBG), mWidgetBounds);

  //const float halfHandleSize = mHandleSize / 2.f;

  //track
  const float minVal = (float) std::min(GetValue(0), GetValue(1));
  const float maxVal = (float) std::max(GetValue(0), GetValue(1));

  IRECT filledTrack = { mTrack.L, mTrack.B - (maxVal * mTrack.H()), mTrack.R, mTrack.B - (minVal * mTrack.H()) };

  g.FillRect(GetColor(kSH), mTrack);
  g.FillRect(GetColor(kFG), filledTrack);
  g.DrawRect(GetColor(kFR), mTrack);

  float cx[2];
  float cy[2];

  if (mDirection == EDirection::Vertical)
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
  if(mDirection == EDirection::Vertical)
    mMouseDownVal = 1.f - (y-mRECT.T) / mRECT.H();
  else
    mMouseDownVal = (x-mRECT.L) / mRECT.W();
}

void IVRangeSliderControl::OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod & mod)
{
  SnapToMouse(x, y, mDirection, mTrack);
}

void IVRangeSliderControl::SnapToMouse(float x, float y, EDirection direction, IRECT& bounds, int valIdx, float scalar /* TODO: scalar! */)
{
  bounds.Constrain(x, y);
  
  double newVal;
  
  if(direction == EDirection::Vertical)
    newVal = 1.f - (y-bounds.T) / bounds.H();
  else
    newVal = (x-bounds.L) / bounds.W();
  
  double* pHighVal;
  double* pLowVal;
  
  if(mMouseDownVal > newVal)
  {
    pHighVal = &mMouseDownVal;
    pLowVal = &newVal;
  }
  else
  {
    pHighVal = &newVal;
    pLowVal = &mMouseDownVal;
  }
  
  SetValue(std::round(*pHighVal / 0.001 ) * 0.001, 0);
  SetValue(std::round(*pLowVal / 0.001 ) * 0.001, 1);

  SetDirty(true, valIdx);
}

IVXYPadControl::IVXYPadControl(const IRECT& bounds, const std::initializer_list<int>& params,
               const char* label,
               const IVStyle& style,
               float handleRadius)
: IControl(bounds, params)
, IVectorBase(style)
, mHandleRadius(handleRadius)
{
  AttachIControl(this, label);
}

void IVXYPadControl::Draw(IGraphics& g)
{
  DrawBackGround(g, mRECT);
  DrawLabel(g);
  
  if(mStyle.drawFrame)
    g.DrawRect(GetColor(kFR), mWidgetBounds, nullptr, mStyle.frameThickness);
  
  DrawWidget(g);
}

void IVXYPadControl::DrawWidget(IGraphics& g)
{
  float xpos = GetValue(0) * mWidgetBounds.W();
  float ypos = GetValue(1) * mWidgetBounds.H();
  
  g.DrawVerticalLine(GetColor(kSH), mWidgetBounds, 0.5);
  g.DrawHorizontalLine(GetColor(kSH), mWidgetBounds, 0.5);
  g.PathClipRegion(mWidgetBounds.GetPadded(-0.5 * mStyle.frameThickness));
  DrawPressableEllipse(g, IRECT(mWidgetBounds.L + xpos-mHandleRadius, mWidgetBounds.B - ypos-mHandleRadius, mWidgetBounds.L + xpos+mHandleRadius,  mWidgetBounds.B -ypos+mHandleRadius), mMouseDown, mMouseIsOver);
}

void IVXYPadControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  mMouseDown = true;
  if(mStyle.hideCursor)
    GetUI()->HideMouseCursor(true, false);

  OnMouseDrag(x, y, 0., 0., mod);
}

void IVXYPadControl::OnMouseUp(float x, float y, const IMouseMod& mod)
{
  if(mStyle.hideCursor)
    GetUI()->HideMouseCursor(false);

  mMouseDown = false;
  SetDirty(true);
}

void IVXYPadControl::OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod)
{
  mRECT.Constrain(x, y);
  float xn = (x - mRECT.L) / mRECT.W();
  float yn = 1.f - ((y - mRECT.T) / mRECT.H());
  SetValue(xn, 0);
  SetValue(yn, 1);
  SetDirty(true);
}

void IVXYPadControl::OnResize()
{
  SetTargetRECT(CalculateRects(mRECT));

  SetDirty(false);
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

IBSliderControl::IBSliderControl(const IRECT& bounds, int paramIdx, const IBitmap& bitmap,
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
  if (dir == EDirection::Vertical)
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

  if (mDirection == EDirection::Vertical)
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
