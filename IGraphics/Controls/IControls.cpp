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

using namespace iplug;
using namespace igraphics;

#pragma mark - VECTOR CONTROLS

const IColor IVKeyboardControl::DEFAULT_BK_COLOR = IColor(255, 70, 70, 70);
const IColor IVKeyboardControl::DEFAULT_WK_COLOR = IColor(255, 240, 240, 240);
const IColor IVKeyboardControl::DEFAULT_PK_COLOR = IColor(60, 0, 0, 0);
const IColor IVKeyboardControl::DEFAULT_FR_COLOR = COLOR_BLACK;
const IColor IVKeyboardControl::DEFAULT_HK_COLOR = COLOR_ORANGE;

IVLabelControl::IVLabelControl(const IRECT& bounds, const char* label, const IVStyle& style)
: ITextControl(bounds, label)
, IVectorBase(style)
{
  mText = style.valueText;
  AttachIControl(this, label);
}

void IVLabelControl::Draw(IGraphics& g)
{
  DrawBackground(g, mRECT);

  if (mStr.GetLength())
  {
    if (mStyle.drawShadows && !IsDisabled())
      g.DrawText(mStyle.valueText.WithFGColor(GetColor(kSH)), mStr.Get(), mRECT.GetTranslated(mStyle.shadowOffset, mStyle.shadowOffset), &mBlend);

    g.DrawText(mStyle.valueText, mStr.Get(), mRECT, &mBlend);
  }

  if (mStyle.drawFrame)
    g.DrawRect(GetColor(kFR), mRECT, &mBlend, mStyle.frameThickness);
}

IVButtonControl::IVButtonControl(const IRECT& bounds, IActionFunction aF, const char* label, const IVStyle& style, bool labelInButton, bool valueInButton, EVShape shape)
: IButtonControlBase(bounds, aF)
, IVectorBase(style, labelInButton, valueInButton)
{
  mText = style.valueText;
  mShape = shape;
  AttachIControl(this, label);
}

void IVButtonControl::Draw(IGraphics& g)
{
  DrawBackground(g, mRECT);
  DrawWidget(g);
  DrawLabel(g);
  DrawValue(g, false);
}

void IVButtonControl::DrawWidget(IGraphics& g)
{
  bool pressed = (bool)GetValue();
  DrawPressableShape(g, mShape, mWidgetBounds, pressed, mMouseIsOver, IsDisabled());
}

void IVButtonControl::OnResize()
{
  SetTargetRECT(MakeRects(mRECT, true));
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
}

IVSwitchControl::IVSwitchControl(const IRECT& bounds, IActionFunction aF, const char* label, const IVStyle& style, int numStates, bool valueInButton)
: ISwitchControlBase(bounds, kNoParameter, aF, numStates)
, IVectorBase(style, false, valueInButton)
{
  AttachIControl(this, label);
  mText = style.valueText;
  
  if(valueInButton)
    mText.mVAlign = mStyle.valueText.mVAlign = EVAlign::Middle;
}

void IVSwitchControl::Draw(IGraphics& g)
{
  DrawBackground(g, mRECT);
  DrawLabel(g);
  DrawWidget(g);
  DrawValue(g, false);
}

void IVSwitchControl::DrawWidget(IGraphics& g)
{
  DrawPressableShape(g, mShape, mWidgetBounds, mMouseDown, mMouseIsOver, IsDisabled());
}

void IVSwitchControl::SetDirty(bool push, int valIdx)
{
  IControl::SetDirty(push);

  const IParam* pParam = GetParam();

  if(pParam)
    pParam->GetDisplay(mValueStr);
}

void IVSwitchControl::OnResize()
{
  SetTargetRECT(MakeRects(mRECT, true));
  SetDirty(false);
}

bool IVSwitchControl::IsHit(float x, float y) const
{
  return mWidgetBounds.Contains(x, y);
}

void IVSwitchControl::OnInit()
{
  ISwitchControlBase::OnInit();
  
  const IParam* pParam = GetParam();
  
  if(pParam)
  {
    pParam->GetDisplayWithLabel(mValueStr);
  
    if(!mLabelStr.GetLength())
      mLabelStr.Set(pParam->GetName());
  }
}

IVToggleControl::IVToggleControl(const IRECT& bounds, int paramIdx, const char* label, const IVStyle& style, const char* offText, const char* onText)
: IVSwitchControl(bounds, paramIdx, label, style, true)
, mOffText(offText)
, mOnText(onText)
{
  //TODO: assert boolean?
}

IVToggleControl::IVToggleControl(const IRECT& bounds, IActionFunction aF, const char* label, const IVStyle& style, const char* offText, const char* onText, bool initialState)
: IVSwitchControl(bounds, aF, label, style, 2, true)
, mOffText(offText)
, mOnText(onText)
{
  SetValue((double) initialState);
}

void IVToggleControl::DrawWidget(IGraphics& g)
{
  DrawPressableShape(g, mShape, mWidgetBounds, GetValue() > 0.5, mMouseIsOver, IsDisabled());
}

void IVToggleControl::DrawValue(IGraphics& g, bool mouseOver)
{
  if(mouseOver)
    g.FillRect(GetColor(kHL), mValueBounds, &mBlend);
  
  if(GetValue() > 0.5)
    g.DrawText(mStyle.valueText, mOnText.Get(), mValueBounds, &mBlend);
  else
    g.DrawText(mStyle.valueText, mOffText.Get(), mValueBounds, &mBlend);
}

//TODO: Don't Repeat Yourself!
IVSlideSwitchControl::IVSlideSwitchControl(const IRECT& bounds, int paramIdx, const char* label, const IVStyle& style, bool valueInButton, EDirection direction)
: IVSwitchControl(bounds, paramIdx, label, style, valueInButton)
, mDirection(direction)
{
  SetActionFunction([&](IControl* pCaller) {
    SetAnimation([&](IControl* pCaller) {
      auto progress = pCaller->GetAnimationProgress();
      
      mHandleBounds = IRECT::LinearInterpolateBetween(mStartRect, mEndRect, static_cast<float>(progress));

      if(mValueInWidget)
        mValueBounds = mHandleBounds;
      
      if(progress > 1.) {
        pCaller->OnEndAnimation();
        return;
      }
    },
    DEFAULT_ANIMATION_DURATION);
  });
}

IVSlideSwitchControl::IVSlideSwitchControl(const IRECT& bounds, IActionFunction aF, const char* label, const IVStyle& style, bool valueInButton, EDirection direction, int numStates, int initialState)
: IVSwitchControl(bounds, nullptr, label, style, numStates, valueInButton)
, mDirection(direction)
{
  SetValue((double) initialState);
  
  SetActionFunction([&](IControl* pCaller) {
    SetAnimation([&](IControl* pCaller) {
      auto progress = pCaller->GetAnimationProgress();
      
      mHandleBounds = IRECT::LinearInterpolateBetween(mStartRect, mEndRect, static_cast<float>(progress));
      
      if(mValueInWidget)
        mValueBounds = mHandleBounds;
      
      if(progress > 1.) {
        pCaller->OnEndAnimation();
        return;
      }
      
    },
    DEFAULT_ANIMATION_DURATION);
  });
  
  SetAnimationEndActionFunction(aF);
}

void IVSlideSwitchControl::UpdateRects()
{
  mHandleBounds = mStartRect = mWidgetBounds.SubRect(mDirection, mNumStates, GetSelectedIdx());
  mEndRect = mWidgetBounds.SubRect(mDirection, mNumStates, (GetSelectedIdx() + 1) % mNumStates);
  
  if(mValueInWidget)
    mValueBounds = mHandleBounds;
}

void IVSlideSwitchControl::OnResize()
{
  IVSwitchControl::OnResize();
  UpdateRects();
}

void IVSlideSwitchControl::OnEndAnimation()
{
  UpdateRects();

  IControl::OnEndAnimation();
}

void IVSlideSwitchControl::Draw(IGraphics& g)
{
  DrawBackground(g, mRECT);
  DrawWidget(g);
  DrawLabel(g);
  
  if(!GetAnimationFunction())
    DrawValue(g, false);
}

void IVSlideSwitchControl::DrawWidget(IGraphics& g)
{
  DrawTrack(g, mWidgetBounds);
  DrawPressableShape(g, mShape, mHandleBounds, mMouseDown, mMouseIsOver, IsDisabled());
}

void IVSlideSwitchControl::DrawTrack(IGraphics& g, const IRECT& filledArea)
{
  float cR = GetRoundedCornerRadius(mHandleBounds);
  g.FillRoundRect(GetColor(kSH), mWidgetBounds, cR);
}

void IVSlideSwitchControl::SetDirty(bool push, int valIdx)
{
  IVSwitchControl::SetDirty(push, valIdx);
  
  if(!GetAnimationFunction())
    UpdateRects();
}

IVTabSwitchControl::IVTabSwitchControl(const IRECT& bounds, int paramIdx, const std::initializer_list<const char*>& options, const char* label, const IVStyle& style, EVShape shape, EDirection direction)
: ISwitchControlBase(bounds, paramIdx, SplashClickActionFunc, (int) options.size())
, IVectorBase(style)
, mDirection(direction)
{
  AttachIControl(this, label);
  mText = style.valueText;
  mText.mAlign = mStyle.valueText.mAlign = EAlign::Center;
  mText.mVAlign = mStyle.valueText.mVAlign = EVAlign::Middle;
  mShape = shape;

  for (auto& option : options)
  {
    mTabLabels.Add(new WDL_String(option));
  }
}

IVTabSwitchControl::IVTabSwitchControl(const IRECT& bounds, IActionFunction aF, const std::initializer_list<const char*>& options, const char* label, const IVStyle& style, EVShape shape, EDirection direction)
: ISwitchControlBase(bounds, kNoParameter, aF, static_cast<int>(options.size()))
, IVectorBase(style)
, mDirection(direction)
{
  AttachIControl(this, label);
  mText = style.valueText;
  mText.mAlign = mStyle.valueText.mAlign = EAlign::Center;
  mText.mVAlign = mStyle.valueText.mVAlign = EVAlign::Middle;
  mShape = shape;

  for (auto& option : options)
  {
    mTabLabels.Add(new WDL_String(option));
  }
}

void IVTabSwitchControl::OnInit()
{
  ISwitchControlBase::OnInit();
  
  const IParam* pParam = GetParam();
  
  if(pParam && mTabLabels.GetSize() == 0) // don't add param display text based labels if allready added via ctor
  {
    for (int i = 0; i < mNumStates; i++)
    {
      mTabLabels.Add(new WDL_String(GetParam()->GetDisplayText(i)));
    }
    
    if(!mLabelStr.GetLength())
      mLabelStr.Set(pParam->GetName());
  }
}

void IVTabSwitchControl::Draw(IGraphics& g)
{
  DrawBackground(g, mRECT);
  DrawLabel(g);
  DrawWidget(g);
}

void IVTabSwitchControl::DrawButton(IGraphics& g, const IRECT& r, bool pressed, bool mouseOver, ETabSegment segment, bool disabled)
{
  switch (mShape)
  {
    case EVShape::EndsRounded:
      if(mDirection == EDirection::Horizontal)
        DrawPressableRectangle(g, r, pressed, mouseOver, disabled, segment == ETabSegment::Start, segment == ETabSegment::End, false, false);
      else
        DrawPressableRectangle(g, r, pressed, mouseOver, false, disabled, segment == ETabSegment::Start, false, segment == ETabSegment::End);
      break;
    case EVShape::AllRounded:
      if(mDirection == EDirection::Horizontal)
        DrawPressableRectangle(g, r, pressed, mouseOver, disabled, true, true, false, false);
      else
        DrawPressableRectangle(g, r, pressed, mouseOver, disabled, false, true, false, true);
      break;
    default:
      DrawPressableShape(g, mShape, r, pressed, mouseOver, disabled);
      break;
  }
}

void IVTabSwitchControl::DrawWidget(IGraphics& g)
{
  int selected = GetSelectedIdx();
  ETabSegment segment = ETabSegment::Start;

  for (int i = 0; i < mNumStates; i++)
  {
    IRECT r = mButtons.Get()[i];
    
    if(i > 0)
      segment = ETabSegment::Mid;
    
    if(i == mNumStates-1)
      segment = ETabSegment::End;

    DrawButton(g, r, i == selected, mMouseOverButton == i, segment, IsDisabled() || GetStateDisabled(i));
        
    if(mTabLabels.Get(i))
    {
      g.DrawText(mStyle.valueText, mTabLabels.Get(i)->Get(), r, &mBlend);
    }
  }
}

int IVTabSwitchControl::GetButtonForPoint(float x, float y) const
{
  for (int i = 0; i < mNumStates; i++)
  {
    if (mButtons.Get()[i].Contains(x, y))
      return i;
  }
  
  return -1;
}

bool IVTabSwitchControl::IsHit(float x, float y) const
{
  return GetButtonForPoint(x, y) > -1;
}

void IVTabSwitchControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  int index = GetButtonForPoint(x, y);
  if (index > -1)
    SetValue(((double) index * (1./(double) (mNumStates-1))));
  
  SetDirty(true);
}

void IVTabSwitchControl::OnMouseOver(float x, float y, const IMouseMod& mod)
{
  mMouseOverButton = GetButtonForPoint(x, y);
  
  ISwitchControlBase::OnMouseOver(x, y, mod);
  
  SetDirty(false);
}

void IVTabSwitchControl::OnResize()
{
  SetTargetRECT(MakeRects(mRECT));
  
  mButtons.Resize(0);
  
  for (int i = 0; i < mNumStates; i++)
  {
    mButtons.Add(mWidgetBounds.SubRect(mDirection, mNumStates, i));
  }
  
  SetDirty(false);
}

const char* IVTabSwitchControl::GetSelectedLabelStr() const
{
  return mTabLabels.Get(GetSelectedIdx())->Get();
}

IVRadioButtonControl::IVRadioButtonControl(const IRECT& bounds, int paramIdx, const std::initializer_list<const char*>& options, const char* label, const IVStyle& style, EVShape shape, EDirection direction, float buttonSize)
: IVTabSwitchControl(bounds, paramIdx, options, label, style, shape, direction)
, mButtonSize(buttonSize)
{
  mButtonAreaWidth = buttonSize * 3.f;
  mText.mAlign = mStyle.valueText.mAlign = EAlign::Near;
  mText.mVAlign = mStyle.valueText.mVAlign = EVAlign::Middle;
}

IVRadioButtonControl::IVRadioButtonControl(const IRECT& bounds, IActionFunction aF, const std::initializer_list<const char*>& options, const char* label, const IVStyle& style, EVShape shape, EDirection direction, float buttonSize)
: IVTabSwitchControl(bounds, aF, options, label, style, shape, direction)
, mButtonSize(buttonSize)
{
  mButtonAreaWidth = buttonSize * 3.f;
  mText.mAlign = mStyle.valueText.mAlign = EAlign::Near;
  mText.mVAlign = mStyle.valueText.mVAlign = EVAlign::Middle;
}

void IVRadioButtonControl::DrawWidget(IGraphics& g)
{
  int hit = GetSelectedIdx();
  
  for (int i = 0; i < mNumStates; i++)
  {
    IRECT r = mButtons.Get()[i];
    
    DrawButton(g, r.GetFromLeft(mButtonAreaWidth).GetCentredInside(mButtonSize), i == hit, mMouseOverButton == i, ETabSegment::Mid, IsDisabled() || GetStateDisabled(i));
    
    if (mTabLabels.Get(i))
    {
      r = r.GetFromRight(r.W() - mButtonAreaWidth);
      g.DrawText(mStyle.valueText.WithFGColor(i == hit ? GetColor(kON) : GetColor(kX1)), mTabLabels.Get(i)->Get(), r, &mBlend);
    }
  }
}

int IVRadioButtonControl::GetButtonForPoint(float x, float y) const
{
  if (mOnlyButtonsRespondToMouse)
  {
    for (int i = 0; i < mNumStates; i++)
    {
      if (mButtons.Get()[i].GetFromLeft(mButtonAreaWidth).Contains(x, y))
        return i;
    }
    
    return -1;
  }
  else
    return IVTabSwitchControl::GetButtonForPoint(x, y);
}

IVKnobControl::IVKnobControl(const IRECT& bounds, int paramIdx, const char* label, const IVStyle& style, bool valueIsEditable, bool valueInWidget, float a1, float a2, float aAnchor,  EDirection direction, double gearing, float trackSize)
: IKnobControlBase(bounds, paramIdx, direction, gearing)
, IVectorBase(style, false, valueInWidget)
, mAngle1(a1)
, mAngle2(a2)
, mAnchorAngle(aAnchor)
{
  DisablePrompt(!valueIsEditable);
  mText = style.valueText;
  mHideCursorOnDrag = mStyle.hideCursor;
  mShape = EVShape::Ellipse;
  mTrackSize = trackSize;
  AttachIControl(this, label);
}

IVKnobControl::IVKnobControl(const IRECT& bounds, IActionFunction aF, const char* label, const IVStyle& style, bool valueIsEditable, bool valueInWidget,  float a1, float a2, float aAnchor, EDirection direction, double gearing, float trackSize)
: IKnobControlBase(bounds, kNoParameter, direction, gearing)
, IVectorBase(style, false, valueInWidget)
, mAngle1(a1)
, mAngle2(a2)
, mAnchorAngle(aAnchor)
{
  DisablePrompt(!valueIsEditable);
  mText = style.valueText;
  mHideCursorOnDrag = mStyle.hideCursor;
  mShape = EVShape::Ellipse;
  mTrackSize = trackSize;
  SetActionFunction(aF);
  AttachIControl(this, label);
}

void IVKnobControl::Draw(IGraphics& g)
{
  DrawBackground(g, mRECT);
  DrawLabel(g);
  DrawWidget(g);
  DrawValue(g, mValueMouseOver);
}

IRECT IVKnobControl::GetKnobDragBounds()
{
  IRECT r;
  
  if(mWidgetBounds.W() > mWidgetBounds.H())
    r = mWidgetBounds.GetCentredInside(mWidgetBounds.H()/2.f, mWidgetBounds.H());
  else
    r = mWidgetBounds.GetCentredInside(mWidgetBounds.W(), mWidgetBounds.W()/2.f);
  
  return r;
}

void IVKnobControl::DrawWidget(IGraphics& g)
{
  float widgetRadius; // The radius out to the indicator track arc
  
  if(mWidgetBounds.W() > mWidgetBounds.H())
    widgetRadius = (mWidgetBounds.H()/2.f);
  else
    widgetRadius = (mWidgetBounds.W()/2.f);
  
  const float cx = mWidgetBounds.MW(), cy = mWidgetBounds.MH();
  
  widgetRadius -= (mTrackSize/2.f);

  IRECT knobHandleBounds = mWidgetBounds.GetCentredInside((widgetRadius - mTrackToHandleDistance) * 2.f );
  const float angle = mAngle1 + (static_cast<float>(GetValue()) * (mAngle2 - mAngle1));
  DrawIndicatorTrack(g, angle, cx, cy, widgetRadius);
  DrawPressableShape(g, /*mShape*/ EVShape::Ellipse, knobHandleBounds, mMouseDown, mMouseIsOver, IsDisabled());
  DrawPointer(g, angle, cx, cy, knobHandleBounds.W() / 2.f);
}

void IVKnobControl::DrawIndicatorTrack(IGraphics& g, float angle, float cx, float cy, float radius)
{
  if (mTrackSize > 0.f)
  {
    g.DrawArc(GetColor(kX1), cx, cy, radius, angle >= mAnchorAngle ? mAnchorAngle : mAnchorAngle - (mAnchorAngle - angle), angle >= mAnchorAngle ? angle : mAnchorAngle, &mBlend, mTrackSize);
  }
}

void IVKnobControl::DrawPointer(IGraphics& g, float angle, float cx, float cy, float radius)
{
  g.DrawRadialLine(GetColor(kFR), cx, cy, angle, mInnerPointerFrac * radius, mOuterPointerFrac * radius, &mBlend, mPointerThickness);
}

void IVKnobControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  if(mStyle.showValue && mValueBounds.Contains(x, y))
  {
    PromptUserInput(mValueBounds);
  }
  else
  {    
    IKnobControlBase::OnMouseDown(x, y, mod);
  }

  SetDirty(false);
}

void IVKnobControl::OnMouseDblClick(float x, float y, const IMouseMod& mod)
{
  #ifdef AAX_API
  PromptUserInput(mValueBounds);
  #else
  SetValueToDefault(GetValIdxForPos(x, y));
  #endif
}

void IVKnobControl::OnMouseUp(float x, float y, const IMouseMod& mod)
{
  IKnobControlBase::OnMouseUp(x, y, mod);
  SetDirty(true);
}

void IVKnobControl::OnMouseOver(float x, float y, const IMouseMod& mod)
{
  if(mStyle.showValue && !mDisablePrompt)
    mValueMouseOver = mValueBounds.Contains(x,y);
  
  IKnobControlBase::OnMouseOver(x, y, mod);

  SetDirty(false);
}

void IVKnobControl::OnResize()
{
  SetTargetRECT(MakeRects(mRECT));
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
    pParam->GetDisplayWithLabel(mValueStr);
}

void IVKnobControl::OnInit()
{
  const IParam* pParam = GetParam();
  
  if(pParam)
  {
    pParam->GetDisplayWithLabel(mValueStr);
    
    if(!mLabelStr.GetLength())
      mLabelStr.Set(pParam->GetName());
  }
}

IVSliderControl::IVSliderControl(const IRECT& bounds, int paramIdx, const char* label, const IVStyle& style, bool valueIsEditable, EDirection dir, double gearing, float handleSize, float trackSize, bool handleInsideTrack)
: ISliderControlBase(bounds, paramIdx, dir, gearing, handleSize)
, IVectorBase(style)
, mHandleInsideTrack(handleInsideTrack)
{
  DisablePrompt(!valueIsEditable);
  mText = style.valueText;
  mHideCursorOnDrag = style.hideCursor;
  mShape = EVShape::Ellipse;
  mTrackSize = trackSize;
  AttachIControl(this, label);
}

IVSliderControl::IVSliderControl(const IRECT& bounds, IActionFunction aF, const char* label, const IVStyle& style, bool valueIsEditable, EDirection dir, double gearing, float handleSize, float trackSize, bool handleInsideTrack)
: ISliderControlBase(bounds, aF, dir, gearing, handleSize)
, IVectorBase(style)
, mHandleInsideTrack(handleInsideTrack)
{
  DisablePrompt(!valueIsEditable);
  mText = style.valueText;
  mHideCursorOnDrag = style.hideCursor;
  mShape = EVShape::Ellipse;
  mTrackSize = trackSize;
  AttachIControl(this, label);
}

void IVSliderControl::Draw(IGraphics& g)
{
  DrawBackground(g, mRECT);
  DrawLabel(g);
  DrawWidget(g);
  DrawValue(g, mValueMouseOver);
}

void IVSliderControl::DrawTrack(IGraphics& g, const IRECT& filledArea)
{
  const float extra = mHandleInsideTrack ? mHandleSize : 0.f;
  const IRECT adjustedTrackBounds = mDirection == EDirection::Vertical ? mTrackBounds.GetVPadded(extra) : mTrackBounds.GetHPadded(extra);
  const IRECT adjustedFillBounds = mDirection == EDirection::Vertical ? filledArea.GetVPadded(extra) : filledArea.GetHPadded(extra);
  const float cr = GetRoundedCornerRadius(mTrackBounds);
  
  g.FillRoundRect(GetColor(kSH), adjustedTrackBounds, cr, &mBlend);
  g.FillRoundRect(GetColor(kX1), adjustedFillBounds, cr, &mBlend);
  
  if(mStyle.drawFrame)
    g.DrawRoundRect(GetColor(kFR), adjustedTrackBounds, cr, &mBlend, mStyle.frameThickness);
}

void IVSliderControl::DrawWidget(IGraphics& g)
{
  IRECT filledTrack = mTrackBounds.FracRect(mDirection, (float) GetValue());

  if(mTrackSize > 0.f)
    DrawTrack(g, filledTrack);
  
  float cx, cy;
  
  const float offset = (mStyle.drawShadows && mShape != EVShape::Ellipse /* TODO? */) ? mStyle.shadowOffset * 0.5f : 0.f;
  
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
  
  if(mHandleSize > 0.f)
  {
    DrawHandle(g, {cx-mHandleSize, cy-mHandleSize, cx+mHandleSize, cy+mHandleSize});
  }
}

void IVSliderControl::DrawHandle(IGraphics& g, const IRECT& bounds)
{
  DrawPressableShape(g, mShape, bounds, mMouseDown, mMouseIsOver, IsDisabled());
}

void IVSliderControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  if(mStyle.showValue && mValueBounds.Contains(x, y))
  {
    PromptUserInput(mValueBounds);
  }
  else
  { 
    ISliderControlBase::OnMouseDown(x, y, mod);
  }
}

void IVSliderControl::OnMouseDblClick(float x, float y, const IMouseMod& mod)
{
  #ifdef AAX_API
  PromptUserInput(mValueBounds);
  #else
  SetValueToDefault(GetValIdxForPos(x, y));
  #endif
}

void IVSliderControl::OnMouseUp(float x, float y, const IMouseMod& mod)
{
  ISliderControlBase::OnMouseUp(x, y, mod);
  SetDirty(true);
}

void IVSliderControl::OnMouseOver(float x, float y, const IMouseMod& mod)
{
  if(mStyle.showValue && !mDisablePrompt)
    mValueMouseOver = mValueBounds.Contains(x,y);
  
  ISliderControlBase::OnMouseOver(x, y, mod);
  SetDirty(false);
}

void IVSliderControl::OnResize()
{
  SetTargetRECT(MakeRects(mRECT));
  
  if(mDirection == EDirection::Vertical)
    mTrackBounds = mWidgetBounds.GetPadded(-mHandleSize).GetMidHPadded(mTrackSize);
  else
    mTrackBounds = mWidgetBounds.GetPadded(-mHandleSize).GetMidVPadded(mTrackSize);

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
    pParam->GetDisplayWithLabel(mValueStr);
}

void IVSliderControl::OnInit()
{
  const IParam* pParam = GetParam();
  
  if(pParam)
  {
    if(!mLabelStr.GetLength())
      mLabelStr.Set(pParam->GetName());
    
    pParam->GetDisplayWithLabel(mValueStr);
  }
}

IVRangeSliderControl::IVRangeSliderControl(const IRECT& bounds, const std::initializer_list<int>& params, const char* label, const IVStyle& style, EDirection dir, bool onlyHandle, float handleSize, float trackSize)
: IVTrackControlBase(bounds, label, style, params, 0, dir)
, mHandleSize(handleSize)
{
  mTrackSize = trackSize;
}

void IVRangeSliderControl::Draw(IGraphics& g)
{
  DrawBackground(g, mRECT);
  DrawLabel(g);
  DrawWidget(g);
//  DrawValue(g, mValueMouseOver);
}

void IVRangeSliderControl::MakeTrackRects(const IRECT& bounds)
{
  for (int ch = 0; ch < NVals(); ch++)
  {
    if(mDirection == EDirection::Vertical)
      mTrackBounds.Get()[ch] = bounds.GetPadded(-mHandleSize).GetMidHPadded(mTrackSize);
    else
      mTrackBounds.Get()[ch] = bounds.GetPadded(-mHandleSize).GetMidVPadded(mTrackSize);
  }
}

void IVRangeSliderControl::DrawTrack(IGraphics& g, const IRECT& r, int chIdx)
{
  bool thisTrack = mMouseOverHandle == chIdx;
  float angle = 0.f;
  
  if(mDirection == EDirection::Horizontal)
    angle = chIdx % 2 ? 180.f : 0.f;
  else
    angle = chIdx % 2 ? 270.f : 90.f;
    
  DrawPressableTriangle(g, GetHandleBounds(chIdx), thisTrack && mMouseIsDown, thisTrack, angle, IsDisabled());
}

IRECT IVRangeSliderControl::GetHandleBounds(int trackIdx)
{
  IRECT filledTrack = mTrackBounds.Get()[trackIdx].FracRect(mDirection, (float) GetValue(trackIdx));
  float cx, cy;
  const float offset = (mStyle.drawShadows && mShape != EVShape::Ellipse /* TODO? */) ? mStyle.shadowOffset * 0.5f : 0.f;
  if(mDirection == EDirection::Vertical)
  {
    cx = filledTrack.MW() + offset;
    cy = filledTrack.T;
    
    if(trackIdx % 2)
      return IRECT(cx+mTrackSize, cy-mHandleSize, cx+(2.f*mHandleSize)+mTrackSize, cy+mHandleSize);
    else
      return IRECT(cx-(2.f*mHandleSize), cy-mHandleSize, cx, cy+mHandleSize);
  }
  else
  {
    cx = filledTrack.R;
    cy = filledTrack.MH() + offset;
    
    if(trackIdx % 2)
      return IRECT(cx-mHandleSize, cy-(2.f*mHandleSize), cx+mHandleSize, cy);
    else
      return IRECT(cx-mHandleSize, cy+mTrackSize, cx+mHandleSize, cy+(2.f*mHandleSize)+mTrackSize);
  }
}

void IVRangeSliderControl::DrawWidget(IGraphics& g)
{
  IRECT r = mTrackBounds.Get()[0];
  
  DrawTrackBackground(g, r, 0);
  
  for(int i=0;i<NVals()-1;i++)
  {
    IRECT filled1 = mTrackBounds.Get()[i].FracRect(mDirection, (float) GetValue(i));
    IRECT filled2 = mTrackBounds.Get()[i+1].FracRect(mDirection, (float) GetValue(i+1));
    
    if(mDirection == EDirection::Vertical)
      g.FillRect(GetColor(kX1), IRECT(filled1.L, filled1.T < filled2.T ? filled1.T : filled2.T, filled1.R, filled1.T > filled2.T ? filled1.T : filled2.T), &mBlend);
    else
      g.FillRect(GetColor(kX1), IRECT(filled1.R < filled2.R ? filled1.R : filled2.R, filled1.T, filled1.R > filled2.R ? filled1.R : filled2.R, filled1.B), &mBlend);
  }
  
  if(mStyle.drawFrame && mDrawTrackFrame)
    g.DrawRect(GetColor(kFR), r, &mBlend, mStyle.frameThickness);
  
  IVTrackControlBase::DrawWidget(g);
}

void IVRangeSliderControl::OnMouseOver(float x, float y, const IMouseMod& mod)
{
  IRECT bounds;
  int hitHandle = -1;
  
  for(int i=0;i<NVals();i++)
  {
    bounds = GetHandleBounds(i);
    if(bounds.Contains(x, y))
    {
      hitHandle = i;
      break;
    }
  }
  
  mMouseOverHandle = hitHandle;
  
  IVTrackControlBase::OnMouseOver(x, y, mod);
  SetDirty(false);
}

void IVRangeSliderControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  mMouseIsDown = true;
  OnMouseDrag(x, y, 0., 0., mod);
}

void IVRangeSliderControl::OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod)
{
  if(mMouseOverHandle == -1)
    return;
  
  auto minClip = mMouseOverHandle == 0 ? 0. : GetValue(mMouseOverHandle-1);
  auto maxClip = mMouseOverHandle == NVals()-1 ? 1. : GetValue(mMouseOverHandle+1);
  SnapToMouse(x, y, mDirection, mWidgetBounds, mMouseOverHandle, minClip, maxClip);
}

IVXYPadControl::IVXYPadControl(const IRECT& bounds, const std::initializer_list<int>& params, const char* label, const IVStyle& style, float handleRadius)
: IControl(bounds, params)
, IVectorBase(style)
, mHandleRadius(handleRadius)
{
  mShape = EVShape::Ellipse;
  AttachIControl(this, label);
}

void IVXYPadControl::Draw(IGraphics& g)
{
  DrawBackground(g, mRECT);
  DrawLabel(g);
  
  if(mStyle.drawFrame)
    g.DrawRect(GetColor(kFR), mWidgetBounds, &mBlend, mStyle.frameThickness);
  
  DrawWidget(g);
}

void IVXYPadControl::DrawWidget(IGraphics& g)
{
  DrawTrack(g);
  
  const IRECT trackBounds = mWidgetBounds.GetPadded(mTrackClipsHandle ? 0 : -mHandleRadius);

  const float xpos = static_cast<float>(GetValue(0)) * trackBounds.W();
  const float ypos = static_cast<float>(GetValue(1)) * trackBounds.H();
  const IRECT handleBounds = IRECT(trackBounds.L + xpos-mHandleRadius, trackBounds.B - ypos-mHandleRadius, trackBounds.L + xpos+mHandleRadius,  trackBounds.B -ypos+mHandleRadius);
  
  DrawHandle(g, trackBounds, handleBounds);
}

void IVXYPadControl::DrawHandle(IGraphics& g, const IRECT& trackBounds, const IRECT& handleBounds)
{
  if(mTrackClipsHandle)
    g.PathClipRegion(trackBounds.GetPadded(-0.5f * mStyle.frameThickness));
  
  DrawPressableShape(g, mShape, handleBounds, mMouseDown, mMouseIsOver, IsDisabled());
}

void IVXYPadControl::DrawTrack(IGraphics& g)
{
  g.DrawVerticalLine(GetColor(kSH), mWidgetBounds, 0.5);
  g.DrawHorizontalLine(GetColor(kSH), mWidgetBounds, 0.5);
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
  SetTargetRECT(MakeRects(mRECT));
  SetDirty(false);
}

IVPlotControl::IVPlotControl(const IRECT& bounds, const std::initializer_list<Plot>& plots, int numPoints, const char* label, const IVStyle& style, float min, float max, bool useLayer)
: IControl(bounds)
, IVectorBase(style)
, mMin(min)
, mMax(max)
, mUseLayer(useLayer)
{
  mPoints.resize(numPoints);

  AttachIControl(this, label);
  
  for(auto plot : plots)
  {
    AddPlotFunc(plot.color, plot.func);
  }
}

void IVPlotControl::Draw(IGraphics& g)
{
  DrawBackground(g, mRECT);
  DrawLabel(g);

  float hdiv = mWidgetBounds.W() / static_cast<float>(mHorizontalDivisions);
  float vdiv = mWidgetBounds.H() / static_cast<float>(mVerticalDivisions + 2);

  IRECT plotsRECT = mWidgetBounds.GetVPadded(-vdiv);

  auto drawFunc = [&](){
    g.DrawGrid(GetColor(kSH), mWidgetBounds, hdiv, vdiv, &mBlend);
        
    for (int p=0; p<mPlots.size(); p++)
    {
      for (int i=0; i<mPoints.size(); i++)
      {
        auto v = mPlots[p].func((static_cast<float>(i)/static_cast<float>(mPoints.size() - 1)));
        v = (v - mMin) / (mMax-mMin);
        mPoints[i] = static_cast<float>(v);
      }
      
      g.DrawData(mPlots[p].color, plotsRECT, mPoints.data(), static_cast<int>(mPoints.size()), nullptr, &mBlend, mTrackSize);
    }

    if (mStyle.drawFrame)
      g.DrawRect(GetColor(kFR), mWidgetBounds, &mBlend, mStyle.frameThickness);
  };
  
  if(mUseLayer)
  {
    if (!g.CheckLayer(mLayer))
    {
      g.StartLayer(this, mRECT);
      drawFunc();
      mLayer = g.EndLayer();
    }
    
    g.DrawLayer(mLayer, &mBlend);
  }
  else
    drawFunc();
}

void IVPlotControl::OnResize()
{
  SetTargetRECT(MakeRects(mRECT));
  SetDirty(false);
}

void IVPlotControl::AddPlotFunc(const IColor& color, const IPlotFunc& func)
{
  mPlots.push_back({color, func});
  
  if(mLayer)
    mLayer->Invalidate();
  SetDirty(false);
}

IVGroupControl::IVGroupControl(const IRECT& bounds, const char* label, float labelOffset, const IVStyle& style)
: IControl(bounds)
, IVectorBase(style)
, mLabelOffset(labelOffset)
{
  AttachIControl(this, label);
  mIgnoreMouse = true;
}

IVGroupControl::IVGroupControl(const char* label, const char* groupName, float padL, float padT, float padR, float padB, const IVStyle& style)
: IControl(IRECT())
, IVectorBase(style)
, mGroupName(groupName)
, mPadL(padL)
, mPadT(padT)
, mPadR(padR)
, mPadB(padB)
{
  AttachIControl(this, label);
  mIgnoreMouse = true;
}

void IVGroupControl::OnInit()
{
  if(mGroupName.GetLength())
  {
    SetBoundsBasedOnGroup(mGroupName.Get(), mPadL, mPadT, mPadR, mPadB);
  }
}

void IVGroupControl::Draw(IGraphics& g)
{
//  const float cr = GetRoundedCornerRadius(mWidgetBounds);
//  g.FillRoundRect(GetColor(kBG), mWidgetBounds, cr);
//  g.FillRect(GetColor(kBG), mLabelBounds);
  DrawLabel(g);
  DrawWidget(g);
}

void IVGroupControl::DrawWidget(IGraphics& g)
{
  const float cr = GetRoundedCornerRadius(mWidgetBounds);
  const float ft = mStyle.frameThickness;
  const float hft = ft/2.f;
  
  int nPaths = /*mStyle.drawShadows ? 2 :*/ 1;
  
  auto b = mWidgetBounds.GetPadded(/*mStyle.drawShadows ? -mStyle.shadowOffset :*/ 0.f);
  
  auto labelR = mLabelBounds.Empty() ? mRECT.MW() : mLabelBounds.R;
  auto labelL = mLabelBounds.Empty() ? mRECT.MW() : mLabelBounds.L;

  for(int i=0; i < nPaths; i++)
  {
    const float offset = i == 0 ? 0.f : mStyle.shadowOffset;
    g.PathClear();
    g.PathMoveTo(labelR, b.T + hft - offset);
    g.PathArc(b.R - cr - hft - offset, b.T + cr + hft - offset, cr, 0.f, 90.f);
    g.PathArc(b.R - cr - hft - offset, b.B - cr - hft - offset, cr, 90.f, 180.f);
    g.PathArc(b.L + cr + hft - offset, b.B - cr - hft - offset, cr, 180.f, 270.f);
    g.PathArc(b.L + cr + hft - offset, b.T + cr + hft - offset, cr, 270.f, 360.f);
    g.PathLineTo(labelL, b.T + hft - offset);
    g.PathStroke(mStyle.drawShadows ? GetColor(i == 0 ? kSH : kFR) : GetColor(kFR), ft);
  }
}

void IVGroupControl::OnResize()
{
  SetTargetRECT(MakeRects(mRECT));
  mLabelBounds.HPad(mLabelPadding);
  mWidgetBounds.Offset(0, -(mLabelBounds.H()/2.f) - (mStyle.frameThickness/2.f), 0, 0);
  const float cr = GetRoundedCornerRadius(mWidgetBounds);
  mLabelBounds.Translate(mRECT.L - mLabelBounds.L + mStyle.frameThickness + mLabelOffset + cr, 0.f);
  SetDirty(false);
}

void IVGroupControl::SetBoundsBasedOnGroup(const char* groupName, float padL, float padT, float padR, float padB)
{
  mGroupName.Set(groupName);
  
  IRECT unionRect;
  GetUI()->ForControlInGroup(mGroupName.Get(), [&unionRect](IControl* pControl) { unionRect = unionRect.Union(pControl->GetRECT()); });
  float halfLabelHeight = mLabelBounds.H()/2.f;
  unionRect.GetVPadded(halfLabelHeight);
  mRECT = unionRect.GetPadded(padL, padT, padR, padB);
  
  OnResize();
}

IVColorSwatchControl::IVColorSwatchControl(const IRECT& bounds, const char* label, ColorChosenFunc func, const IVStyle& style, ECellLayout layout,
  const std::initializer_list<EVColor>& colorIDs, const std::initializer_list<const char*>& labelsForIDs)
: IControl(bounds)
, IVectorBase(style)
, mColorChosenFunc(func)
, mLayout(layout)
, mColorIdForCells(colorIDs)
{
  assert(colorIDs.size() == labelsForIDs.size());
  
  AttachIControl(this, label);
  mCellRects.Resize(static_cast<int>(mColorIdForCells.size()));
  mText.mAlign = mStyle.valueText.mAlign = EAlign::Far;

  for (int i=0;i<colorIDs.size();i++)
  {
    mLabels.Add(new WDL_String(labelsForIDs.begin()[i]));
  }
}

void IVColorSwatchControl::Draw(IGraphics& g)
{
  DrawWidget(g);
  DrawLabel(g);
}

void IVColorSwatchControl::DrawWidget(IGraphics& g)
{  
  for (int i=0; i< mColorIdForCells.size(); i++)
  {
    WDL_String* pStr = mLabels.Get(i);
    IRECT r = mCellRects.Get()[i];
    IRECT buttonBounds = r.FracRectHorizontal(pStr->GetLength() ? 0.25f : 1.f, true);
    g.FillRect(GetColor(mColorIdForCells[i]), buttonBounds, &mBlend);
    g.DrawRect(i == mCellOver ? COLOR_GRAY : COLOR_DARK_GRAY, buttonBounds.GetPadded(0.5f), &mBlend);
    
    if(pStr->GetLength())
      g.DrawText(mStyle.valueText, mLabels.Get(i)->Get(), r.FracRectHorizontal(0.7f, false), &mBlend);
  }
}

void IVColorSwatchControl::OnResize()
{
  SetTargetRECT(MakeRects(mRECT, true));

  int rows = 3;
  int columns = 3;
  
  if(mLayout == ECellLayout::kGrid)
  {
    rows = 3;
    columns = 3;
  }
  else if (mLayout == ECellLayout::kHorizontal)
  {
    rows = 1;
    columns = static_cast<int>(mColorIdForCells.size());
  }
  else if (mLayout == ECellLayout::kVertical)
  {
    rows = static_cast<int>(mColorIdForCells.size());
    columns = 1;
  }
  
  for (int i=0; i< mColorIdForCells.size(); i++)
  {
    mCellRects.Get()[i] = mWidgetBounds.GetGridCell(i, rows, columns).GetPadded(-2);
  }

  SetDirty(false);
}

void IVColorSwatchControl::OnMouseOver(float x, float y, const IMouseMod& mod)
{
  for (int i=0; i<mColorIdForCells.size(); i++)
  {
    if(mCellRects.Get()[i].Contains(x, y))
    {
      mCellOver = i;
      SetDirty();
      return;
    }
  }
  
  mCellOver = -1;
  SetDirty();
}

void IVColorSwatchControl::OnMouseOut()
{
  mCellOver = -1;
  SetDirty();
}

void IVColorSwatchControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  int cellClicked=-1;
  
  for (int i=0; i<mColorIdForCells.size(); i++)
  {
    if(mCellRects.Get()[i].Contains(x, y))
    {
      cellClicked = i;
      break;
    }
  }
  
  if(cellClicked > -1)
  {
    EVColor vColorClicked = mColorIdForCells[cellClicked];
    IColor color = GetColor(vColorClicked);
    GetUI()->PromptForColor(color, "Choose a color", [this, cellClicked, vColorClicked](IColor result) {
      SetColor(vColorClicked, result);
      if(mColorChosenFunc)
        mColorChosenFunc(cellClicked, result);
    });
  }
}

#pragma mark - SVG CONTROLS

ISVGButtonControl::ISVGButtonControl(const IRECT& bounds, IActionFunction aF, const ISVG& offImage, const ISVG& onImage)
: IButtonControlBase(bounds, aF)
, mOffSVG(offImage)
, mOnSVG(onImage)
{
}

void ISVGButtonControl::Draw(IGraphics& g)
{
  if (GetValue() > 0.5)
    g.DrawSVG(mOnSVG, mRECT, &mBlend);
  else
    g.DrawSVG(mOffSVG, mRECT, &mBlend);
}

ISVGKnobControl::ISVGKnobControl(const IRECT& bounds, const ISVG& svg, int paramIdx)
: IKnobControlBase(bounds, paramIdx)
, mSVG(svg)
{
}

void ISVGKnobControl::Draw(IGraphics& g)
{
  g.DrawRotatedSVG(mSVG, mRECT.MW(), mRECT.MH(), mRECT.W(), mRECT.H(), mStartAngle + GetValue() * (mEndAngle - mStartAngle), &mBlend);
}

void ISVGKnobControl::SetSVG(ISVG& svg)
{
  mSVG = svg;
  SetDirty(false);
}

ISVGSwitchControl::ISVGSwitchControl(const IRECT& bounds, const std::initializer_list<ISVG>& svgs, int paramIdx, IActionFunction aF)
: ISwitchControlBase(bounds, paramIdx, aF, static_cast<int>(svgs.size()))
, mSVGs(svgs)
{
}

void ISVGSwitchControl::Draw(IGraphics& g)
{
  g.DrawSVG(mSVGs[GetSelectedIdx()], mRECT, &mBlend);
}

ISVGSliderControl::ISVGSliderControl(const IRECT& bounds, const ISVG& handleSVG, const ISVG& trackSVG, int paramIdx, EDirection dir, double gearing)
: ISliderControlBase(bounds, paramIdx, dir, gearing)
, mHandleSVG(handleSVG)
, mTrackSVG(trackSVG)
{
}

void ISVGSliderControl::Draw(IGraphics& g)
{
  g.DrawSVG(mTrackSVG, mTrackSVGBounds, &mBlend);
  g.DrawSVG(mHandleSVG, GetHandleBounds(GetValue()), &mBlend);
}

void ISVGSliderControl::OnResize()
{
  auto trackAspectRatio = mTrackSVG.W() / mTrackSVG.H();
  auto handleAspectRatio = mHandleSVG.W() / mHandleSVG.H();
  auto handleOverTrackHeight = mHandleSVG.H() / mTrackSVG.H();

  IRECT handleBoundsAtMidPoint;
  
  if (mDirection == EDirection::Vertical)
  {
    mTrackSVGBounds = mRECT.GetCentredInside(mRECT.H() * trackAspectRatio, mRECT.H());

    handleBoundsAtMidPoint = mRECT.GetCentredInside(mRECT.H() * handleAspectRatio * handleOverTrackHeight, mRECT.H() * handleOverTrackHeight);
    mHandleBoundsAtMax = { handleBoundsAtMidPoint.L, mTrackSVGBounds.T, handleBoundsAtMidPoint.R, mTrackSVGBounds.T + handleBoundsAtMidPoint.H() };
    mTrackBounds = mTrackSVGBounds.GetPadded(0, -handleBoundsAtMidPoint.H(), 0, 0);
  }
  else
  {
    mTrackSVGBounds = mRECT.GetCentredInside(mRECT.W(), mRECT.W() / trackAspectRatio);
    auto handleHeight = mTrackSVGBounds.H() * handleOverTrackHeight;
    handleBoundsAtMidPoint = mRECT.GetCentredInside(handleHeight * handleAspectRatio, handleHeight);
    auto halfHeight = handleBoundsAtMidPoint.H() / 2.f;
    mHandleBoundsAtMax = { mTrackSVGBounds.R - handleBoundsAtMidPoint.W(), mTrackSVGBounds.MH() - halfHeight, mTrackSVGBounds.R, mTrackSVGBounds.MH() + halfHeight };
    mTrackBounds = mTrackSVGBounds.GetPadded(-handleBoundsAtMidPoint.W(), 0, 0, 0);
  }

  SetDirty(false);
}

IRECT ISVGSliderControl::GetHandleBounds(double value) const
{
  if (value < 0.0)
    value = GetValue();

  IRECT r = mHandleBoundsAtMax;

  if (mDirection == EDirection::Vertical)
  {
    float offs = (1.f - (float) value) * mTrackBounds.H();
    r.T += offs;
    r.B += offs;
  }
  else
  {
    float offs = (1.f - (float) value) * mTrackBounds.W();
    r.L -= offs;
    r.R -= offs;
  }

  return r;
}

#pragma mark - BITMAP CONTROLS

IBButtonControl::IBButtonControl(float x, float y, const IBitmap& bitmap, IActionFunction aF)
  : IButtonControlBase(IRECT(x, y, bitmap), aF)
  , IBitmapBase(bitmap)
{
  AttachIControl(this);
}

IBButtonControl::IBButtonControl(const IRECT& bounds, const IBitmap& bitmap, IActionFunction aF)
  : IButtonControlBase(bounds.GetCentredInside(bitmap), aF)
  , IBitmapBase(bitmap)
{
  AttachIControl(this);
}

IBSwitchControl::IBSwitchControl(float x, float y, const IBitmap& bitmap, int paramIdx)
: ISwitchControlBase(IRECT(x, y, bitmap), paramIdx)
, IBitmapBase(bitmap)
{
  AttachIControl(this);
}

IBSwitchControl::IBSwitchControl(const IRECT& bounds, const IBitmap& bitmap, int paramIdx)
: ISwitchControlBase(bounds.GetCentredInside(bitmap), paramIdx)
, IBitmapBase(bitmap)
{
  AttachIControl(this);
}

void IBSwitchControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  if (mBitmap.N() > 1)
    SetValue(GetValue() + 1.0 / static_cast<double>(mBitmap.N() - 1));
  else
    SetValue(GetValue() + 1.0);

  if (GetValue() > 1.001)
    SetValue(0.);

  SetDirty();
}

IBSliderControl::IBSliderControl(float x, float y, float trackLength, const IBitmap& handleBitmap, const IBitmap& trackBitmap, int paramIdx, EDirection dir, double gearing)
: ISliderControlBase(IRECT::MakeXYWH(x, y,
                                     dir == EDirection::Vertical ? handleBitmap.W() : trackLength,
                                     dir == EDirection::Vertical ? trackLength : handleBitmap.H()),
                     paramIdx, dir, gearing,
                     float(dir == EDirection::Vertical ? handleBitmap.H() : handleBitmap.W()))
, IBitmapBase(handleBitmap)
, mTrackBitmap(trackBitmap)
{
  AttachIControl(this);
}

IBSliderControl::IBSliderControl(const IRECT& bounds, const IBitmap& handleBitmap, const IBitmap& trackBitmap, int paramIdx, EDirection dir, double gearing)
: ISliderControlBase(bounds, paramIdx, dir, gearing, float(dir == EDirection::Vertical ? handleBitmap.H() : handleBitmap.W()))
, IBitmapBase(handleBitmap)
, mTrackBitmap(trackBitmap)
{
  AttachIControl(this);
}

void IBSliderControl::Draw(IGraphics& g)
{
  if(mTrackBitmap.IsValid())
    g.DrawBitmap(mTrackBitmap, mRECT.GetCentredInside(IRECT(0, 0, mTrackBitmap)), 0, 0, &mBlend);
  
  g.DrawBitmap(mBitmap, GetHandleBounds(), 1, &mBlend);
}

IRECT IBSliderControl::GetHandleBounds(double value) const
{
  if (value < 0.0)
    value = GetValue();
  
  IRECT r(mTrackBounds.L, mTrackBounds.T, mBitmap);

  if (mDirection == EDirection::Vertical)
    r.Translate(0.f, (1.f - static_cast<float>(value)) * (mTrackBounds.H() - static_cast<float>(mBitmap.H())));
  else
    r.Translate(static_cast<float>(value) * (mTrackBounds.W() - static_cast<float>(mBitmap.W())), 0.f);
  
  return r;
}

void IBSliderControl::OnResize()
{
  if (mDirection == EDirection::Vertical)
  {
    if(mTrackBitmap.IsValid())
      mTrackBounds = mRECT.GetCentredInside(IRECT(0, 0, mTrackBitmap));
    else
    {
      const float halfWidth = static_cast<float>(mBitmap.W()) / 2.f;
      mTrackBounds = mRECT.GetMidHPadded(halfWidth);
    }
  }
  else
  {
    if(mTrackBitmap.IsValid())
      mTrackBounds = mRECT.GetCentredInside(IRECT(0, 0, mTrackBitmap));
    else
    {
      const float halfHeight = static_cast<float>(mBitmap.H()) / 2.f;
      mTrackBounds = mRECT.GetMidVPadded(halfHeight);
    }
  }

  SetDirty(false);
}

void IBKnobRotaterControl::Draw(IGraphics& g)
{
  const double angle = -130.0 + GetValue() * 260.0;
  g.DrawRotatedBitmap(mBitmap, mRECT.MW(), mRECT.MH(), angle, &mBlend);
}

IBTextControl::IBTextControl(const IRECT& bounds, const IBitmap& bitmap, const IText& text, const char* str, int charWidth, int charHeight, int charOffset, bool multiLine, bool vCenter, EBlend blend)
: ITextControl(bounds, str, text)
, IBitmapBase(bitmap)
, mCharWidth(charWidth)
, mCharHeight(charHeight)
, mCharOffset(charOffset)
, mMultiLine(multiLine)
, mVCentre(vCenter)
{
  mBlend = blend;
}

void IBTextControl::Draw(IGraphics& g)
{
  g.DrawBitmapedText(mBitmap, mRECT, mText, &mBlend, mStr.Get(), mVCentre, mMultiLine, mCharWidth, mCharHeight, mCharOffset);
}
