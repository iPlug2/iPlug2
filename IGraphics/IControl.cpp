/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include <cmath>
#include <cstring>
#define WDL_NO_SUPPORT_UTF8
#include "dirscan.h"

#include "IControl.h"
#include "IPlugParameter.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE
void DefaultAnimationFunc(IControl* pCaller)
{
  if(pCaller->GetAnimationProgress() > 1.)
  {
    pCaller->OnEndAnimation();
    return;
  }
};

void SplashAnimationFunc(IControl* pCaller)
{
  auto progress = pCaller->GetAnimationProgress();
  
  if(progress > 1.) {
    pCaller->OnEndAnimation();
    return;
  }
  
  pCaller->As<IVectorBase>()->SetSplashRadius((float) progress);
  pCaller->SetDirty(false);
};

void EmptyClickActionFunc(IControl* pCaller) { };

void DefaultClickActionFunc(IControl* pCaller) { pCaller->SetAnimation(DefaultAnimationFunc, DEFAULT_ANIMATION_DURATION); };

void SplashClickActionFunc(IControl* pCaller)
{
  float x, y;
  pCaller->GetUI()->GetMouseDownPoint(x, y);
  pCaller->As<IVectorBase>()->SetSplashPoint(x, y);
  pCaller->SetAnimation(SplashAnimationFunc, DEFAULT_ANIMATION_DURATION);
}

void ShowBubbleHorizontalActionFunc(IControl* pCaller)
{
  IGraphics* pGraphics = pCaller->GetUI();
  const IParam* pParam = pCaller->GetParam();
  IRECT bounds = pCaller->GetRECT();
  WDL_String display;
  pParam->GetDisplayWithLabel(display);
  pGraphics->ShowBubbleControl(pCaller, bounds.R, bounds.MH(), display.Get(), EDirection::Horizontal, IRECT(0, 0, 50, 30));
}

void ShowBubbleVerticalActionFunc(IControl* pCaller)
{
  IGraphics* pGraphics = pCaller->GetUI();
  const IParam* pParam = pCaller->GetParam();
  IRECT bounds = pCaller->GetRECT();
  WDL_String display;
  pParam->GetDisplayWithLabel(display);
  pGraphics->ShowBubbleControl(pCaller, bounds.MW(), bounds.T, display.Get(), EDirection::Vertical, IRECT(0, 0, 50, 30));
}

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

using namespace iplug;
using namespace igraphics;

IControl::IControl(const IRECT& bounds, int paramIdx, IActionFunction aF)
: mRECT(bounds)
, mTargetRECT(bounds)
, mActionFunc(aF)
{
  mVals[0].idx = paramIdx;
}

IControl::IControl(const IRECT& bounds, const std::initializer_list<int>& params, IActionFunction aF)
: mRECT(bounds)
, mTargetRECT(bounds)
, mActionFunc(aF)
{
  mVals.clear();
  for (auto& paramIdx : params) {
    mVals.push_back({paramIdx, 0.});
  }
}

IControl::IControl(const IRECT& bounds, IActionFunction aF)
: mRECT(bounds)
, mTargetRECT(bounds)
, mActionFunc(aF)
{
}

int IControl::GetParamIdx(int valIdx) const
{
  assert(valIdx > kNoValIdx && valIdx < NVals());
  return mVals[valIdx].idx;
}

void IControl::SetParamIdx(int paramIdx, int valIdx)
{
  assert(valIdx > kNoValIdx && valIdx < NVals());
  mVals.at(valIdx).idx = paramIdx;
  SetDirty(false);
}

const IParam* IControl::GetParam(int valIdx) const
{
  int paramIdx = GetParamIdx(valIdx);
  
  if(paramIdx > kNoParameter)
    return mDelegate->GetParam(paramIdx);
  else
    return nullptr;
}

int IControl::LinkedToParam(int paramIdx) const
{
  const int nVals = NVals();
  
  for (int v = 0; v < nVals; v++)
  {
    if(mVals[v].idx == paramIdx)
    {
      return v;
    }
  }
  
  return kNoValIdx;
}

void IControl::SetValue(double value, int valIdx)
{
  assert(valIdx > kNoValIdx && valIdx < NVals());
  mVals[valIdx].value = value;
}

double IControl::GetValue(int valIdx) const
{
  assert(valIdx > kNoValIdx && valIdx < NVals());
  return mVals[valIdx].value;
}

void IControl::SetValueFromDelegate(double value, int valIdx)
{
  // Don't update the control from delegate if it is being captured
  // (i.e. if host is automating the control then the mouse is more important)
  
  if (!GetUI()->ControlIsCaptured(this))
  {
    if(GetValue(valIdx) != value)
    {
      SetValue(value, valIdx);
      SetDirty(false);
    }
  }
}

void IControl::SetValueFromUserInput(double value, int valIdx)
{
  if (GetValue(valIdx) != value)
  {
    SetValue(value, valIdx);
    SetDirty(true, valIdx);
  }
}

void IControl::SetValueToDefault(int valIdx)
{
  valIdx = (NVals() == 1) ? 0 : valIdx;

  auto paramDefault = [this](int v)
  {
    const IParam* pParam = GetParam(v);
    if (pParam)
      SetValue(pParam->GetDefault(true), v);
  };
    
  ForValIdx(valIdx, paramDefault);
  SetDirty(true, valIdx);
}

void IControl::SetDirty(bool triggerAction, int valIdx)
{
  valIdx = (NVals() == 1) ? 0 : valIdx;

  auto setValue = [this](int v) { SetValue(Clip(GetValue(v), 0.0, 1.0), v); };
  ForValIdx(valIdx, setValue);
  
  mDirty = true;
  
  if (triggerAction)
  {
    auto paramUpdate = [this](int v)
    {
      if (GetParamIdx(v) > kNoParameter)
      {
        GetDelegate()->SendParameterValueFromUI(GetParamIdx(v), GetValue(v)); //TODO: take tuple
        GetUI()->UpdatePeers(this, v);
      }
    };
      
    ForValIdx(valIdx, paramUpdate);
    
    if (mActionFunc)
      mActionFunc(this);
  }
}

void IControl::Animate()
{
  if (GetAnimationFunction())
    mAnimationFunc(this);
}

bool IControl::IsDirty()
{
  if (GetAnimationFunction())
    return true;
  
  if (!mDirty && mAnimationEndActionFuncQueued)
  {
    mAnimationEndActionFuncQueued(this);
    mAnimationEndActionFuncQueued = nullptr;
  }
  
  return mDirty;
}

void IControl::Hide(bool hide)
{
  mHide = hide;
  SetDirty(false);
}

void IControl::SetDisabled(bool disable)
{
  mBlend.mWeight = (disable ? GRAYED_ALPHA : 1.0f);
  mDisabled = disable;
  SetDirty(false);
}

void IControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  #ifdef PROTOOLS
  if (mod.A)
  {
    SetValueToDefault(GetValIdxForPos(x, y));
  }
  #endif

  if (mod.R)
    PromptUserInput(GetValIdxForPos(x, y));
}

void IControl::OnMouseDblClick(float x, float y, const IMouseMod& mod)
{
  #ifdef PROTOOLS
  PromptUserInput(GetValIdxForPos(x, y));
  #else
  SetValueToDefault(GetValIdxForPos(x, y));
  #endif
}

void IControl::OnMouseOver(float x, float y, const IMouseMod& mod)
{
  bool prev = mMouseIsOver;
  mMouseIsOver = true;
  if (prev == false)
    SetDirty(false);
}

void IControl::OnMouseOut()
{
  bool prev = mMouseIsOver;
  mMouseIsOver = false;
  if (prev == true)
    SetDirty(false);
}

void IControl::OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int valIdx)
{
  if (pSelectedMenu && valIdx > kNoValIdx && GetParamIdx(valIdx) > kNoParameter && !mDisablePrompt)
  {
    SetValueFromUserInput(GetParam()->ToNormalized( (double) pSelectedMenu->GetChosenItemIdx()), valIdx);
  }
}

void IControl::SetPosition(float x, float y)
{
  if (x < 0.f) x = 0.f;
  if (y < 0.f) y = 0.f;
 
  float tX = x + (mTargetRECT.L - mRECT.L);
  float tY = y + (mTargetRECT.T - mRECT.T);
    
  SetRECT({x, y, x + mRECT.W(), y + mRECT.H()});
  SetTargetRECT({tX, tY, tX + mTargetRECT.W(), tY + mTargetRECT.H()});
}

void IControl::SetSize(float w, float h)
{
  if (w < 0.f) w = 0.f;
  if (h < 0.f) h = 0.f;

  SetTargetAndDrawRECTs({mRECT.L, mRECT.T, mRECT.L + w, mRECT.T + h});
}

IControl* IControl::AttachGestureRecognizer(EGestureType type, IGestureFunc func)
{
  mGestureFuncs.insert(std::make_pair(type, func));
  
  GetUI()->AttachGestureRecognizer(type); // this will crash if called in constructor
  
  return this; //for chaining
}

bool IControl::OnGesture(const IGestureInfo& info)
{
  auto itr = mGestureFuncs.find(info.type);
  
  if(itr != mGestureFuncs.end())
  {
    mLastGesture = info.type;
    itr->second(this, info);
    return true;
  }
  
  return false;
}

void IControl::PromptUserInput(int valIdx)
{
  if (valIdx > kNoValIdx && GetParamIdx(valIdx) > kNoParameter && !mDisablePrompt)
  {
    if (GetParam(valIdx)->NDisplayTexts()) // popup menu
    {
      GetUI()->PromptUserInput(*this, mRECT, valIdx);
    }
    else // text entry
    {
      float cX = mRECT.MW();
      float cY = mRECT.MH();
      float halfW = PARAM_EDIT_W/2.f;
      float halfH = PARAM_EDIT_H/2.f;

      IRECT txtRECT = IRECT(cX - halfW, cY - halfH, cX + halfW,cY + halfH);
      GetUI()->PromptUserInput(*this, txtRECT, valIdx);
    }
    
    SetDirty(false);
  }
}

void IControl::PromptUserInput(const IRECT& bounds, int valIdx)
{
  if (valIdx > kNoValIdx && GetParamIdx(valIdx) > kNoParameter && !mDisablePrompt)
  {
    GetUI()->PromptUserInput(*this, bounds, valIdx);
  }
}

void IControl::SetPTParameterHighlight(bool isHighlighted, int color)
{
  switch (color)
  {
    case 0: //AAX_eHighlightColor_Red
      mPTHighlightColor = COLOR_RED;
      break;
    case 1: //AAX_eHighlightColor_Blue
      mPTHighlightColor = COLOR_BLUE;
      break;
    case 2: //AAX_eHighlightColor_Green
      mPTHighlightColor = COLOR_GREEN;
      break;
    case 3: //AAX_eHighlightColor_Yellow
      mPTHighlightColor = COLOR_YELLOW;
      break;
    default:
      break;
  }

  mPTisHighlighted = isHighlighted;
  SetDirty(false);
}

void IControl::DrawPTHighlight(IGraphics& g)
{
  if (mPTisHighlighted)
  {
    g.FillCircle(mPTHighlightColor, mRECT.R-5, mRECT.T+5, 2);
  }
}

void IControl::SnapToMouse(float x, float y, EDirection direction, const IRECT& bounds, int valIdx, double minClip, double maxClip)
{
  bounds.Constrain(x, y);

  float val;
  
  if(direction == EDirection::Vertical)
    val = 1.f - (y-bounds.T) / bounds.H();
  else
    val = (x-bounds.L) / bounds.W();

  auto valFunc = [&](int valIdx) {
    SetValue(Clip(std::round(val / 0.001 ) * 0.001, minClip, maxClip), valIdx);
  };
  
  ForValIdx(valIdx, valFunc);
  SetDirty(true, valIdx);
}

void IControl::OnEndAnimation()
{
  mAnimationFunc = nullptr;
  SetDirty(false);
  
  if(mAnimationEndActionFunc) // queue for next clean draw
    mAnimationEndActionFuncQueued = mAnimationEndActionFunc;
}

void IControl::StartAnimation(int duration)
{
  mAnimationStartTime = std::chrono::high_resolution_clock::now();
  mAnimationDuration = Milliseconds(duration);
}

double IControl::GetAnimationProgress() const
{
  if(!mAnimationFunc)
    return 0.;
  
  auto elapsed = Milliseconds(std::chrono::high_resolution_clock::now() - mAnimationStartTime);
  return elapsed.count() / mAnimationDuration.count();
}

ITextControl::ITextControl(const IRECT& bounds, const char* str, const IText& text, const IColor& BGColor, bool setBoundsBasedOnStr)
: IControl(bounds)
, mStr(str)
, mBGColor(BGColor)
, mSetBoundsBasedOnStr(setBoundsBasedOnStr)
{
  mIgnoreMouse = true;
  IControl::mText = text;
}

void ITextControl::OnInit()
{
  if(mSetBoundsBasedOnStr)
    SetBoundsBasedOnStr();
}

void ITextControl::SetStr(const char* str)
{
  if (strcmp(mStr.Get(), str))
  {
    mStr.Set(str);
    
    if(mSetBoundsBasedOnStr)
      SetBoundsBasedOnStr();
    
    SetDirty(false);
  }
}

void ITextControl::SetStrFmt(int maxlen, const char* fmt, ...)
{
  va_list arglist;
  va_start(arglist, fmt);
  mStr.SetAppendFormattedArgs(false, maxlen, fmt, arglist);
  va_end(arglist);
  
  SetDirty(false);
}

void ITextControl::Draw(IGraphics& g)
{
  g.FillRect(mBGColor, mRECT, &mBlend);
  
  if (mStr.GetLength() && g.GetControlInTextEntry() != this)
    g.DrawText(mText, mStr.Get(), mRECT, &mBlend);
}

void ITextControl::SetBoundsBasedOnStr()
{
  IRECT r;
  GetUI()->MeasureText(mText, mStr.Get(), r);
  SetTargetAndDrawRECTs({mRECT.MW()-(r.W()/2.f), mRECT.MH()-(r.H()/2.f), mRECT.MW()+(r.W()/2.f), mRECT.MH()+(r.H()/2.f)});
}

IURLControl::IURLControl(const IRECT& bounds, const char* str, const char* urlStr, const IText& text, const IColor& BGColor, const IColor& MOColor, const IColor& CLColor)
: ITextControl(bounds, str, text, BGColor)
, mURLStr(urlStr)
, mMOColor(MOColor)
, mCLColor(CLColor)
, mOriginalColor(text.mFGColor)
{
  mIgnoreMouse = false;
  IControl::mText = text;
}

void IURLControl::Draw(IGraphics& g)
{
  g.FillRect(mBGColor, mRECT, &mBlend);
  
  if(mMouseIsOver)
    mText.mFGColor = mMOColor;
  else
    mText.mFGColor = mClicked ? mCLColor : mOriginalColor;
  
  if (mStr.GetLength())
  {
    IRECT textDims;
    g.MeasureText(mText, mStr.Get(), textDims);
    
    float linePosY = 0.f;
    float linePosL = 0.f;
    float linePosR = 0.f;

    if(mText.mVAlign == EVAlign::Middle)
      linePosY = mRECT.MH() + textDims.B;
    else if(mText.mVAlign == EVAlign::Bottom)
      linePosY = mRECT.B;
    else if(mText.mVAlign == EVAlign::Top)
      linePosY = mRECT.T - textDims.H();
    
    if(mText.mAlign == EAlign::Center)
    {
      linePosL = mRECT.MW() + textDims.L;
      linePosR = mRECT.MW() + textDims.R;
    }
    else if(mText.mAlign == EAlign::Near)
    {
      linePosL = mRECT.L;
      linePosR = mRECT.L + textDims.W();
    }
    else if(mText.mAlign == EAlign::Far)
    {
      linePosL = mRECT.R - textDims.W();
      linePosR = mRECT.R;
    }

    g.DrawLine(mText.mFGColor, linePosL, linePosY, linePosR, linePosY, &mBlend);
    g.DrawText(mText, mStr.Get(), mRECT, &mBlend);
  }
}

void IURLControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  GetUI()->OpenURL(mURLStr.Get());
  GetUI()->ReleaseMouseCapture();
  mClicked = true;
}

ITextToggleControl::ITextToggleControl(const IRECT& bounds, int paramIdx, const char* offText, const char* onText, const IText& text, const IColor& bgColor)
: ITextControl(bounds, offText, text, bgColor)
, mOnText(onText)
, mOffText(offText)
{
  SetParamIdx(paramIdx);
  //TODO: assert boolean?
  mIgnoreMouse = false;
  mDblAsSingleClick = true;
}

ITextToggleControl::ITextToggleControl(const IRECT& bounds, IActionFunction aF, const char* offText, const char* onText, const IText& text, const IColor& bgColor)
: ITextControl(bounds, offText, text, bgColor)
, mOnText(onText)
, mOffText(offText)
{
  SetActionFunction(aF);
  mDblAsSingleClick = true;
  mIgnoreMouse = false;
}

void ITextToggleControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  if(GetValue() < 0.5)
    SetValue(1.);
  else
    SetValue(0.);
  
  SetDirty(true);
}

void ITextToggleControl::SetDirty(bool push, int valIdx)
{
  if(GetValue() > 0.5)
    SetStr(mOnText.Get());
  else
    SetStr(mOffText.Get());
  
  IControl::SetDirty(push);
}


ICaptionControl::ICaptionControl(const IRECT& bounds, int paramIdx, const IText& text, const IColor& bgColor, bool showParamLabel)
: ITextControl(bounds, "", text, bgColor)
, mShowParamLabel(showParamLabel)
{
  SetParamIdx(paramIdx);
  mDblAsSingleClick = true;
  mDisablePrompt = false;
  mIgnoreMouse = false;
}

void ICaptionControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  if (mod.L || mod.R)
  {
    PromptUserInput(mRECT);
  }
}

void ICaptionControl::Draw(IGraphics& g)
{
  const IParam* pParam = GetParam();

  if(pParam)
  {
    pParam->GetDisplay(mStr);

    if (mShowParamLabel)
    {
      mStr.Append(" ");
      mStr.Append(pParam->GetLabel());
    }
  }

  ITextControl::Draw(g);
  
  if(mTri.W() > 0.f)
  {
    g.FillTriangle(mMouseIsOver ? mTriangleMouseOverColor : mTriangleColor, mTri.L, mTri.T, mTri.R, mTri.T, mTri.MW(), mTri.B, GetMouseIsOver() ? 0 : &BLEND_50);
  }
}

void ICaptionControl::OnResize()
{
  const IParam* pParam = GetParam();
  if(pParam && pParam->Type() == IParam::kTypeEnum)
  {
    mTri = mRECT.FracRectHorizontal(0.2f, true).GetCentredInside(IRECT(0, 0, 8, 5)); //TODO: This seems rubbish
  }
}

PlaceHolder::PlaceHolder(const IRECT& bounds, const char* str)
: ITextControl(bounds, str, IText(20))
{
  mBGColor = COLOR_WHITE;
  mDisablePrompt = false;
  mDblAsSingleClick = false;
  mIgnoreMouse = false;
}

void PlaceHolder::Draw(IGraphics& g)
{
  g.FillRect(mBGColor, mRECT);
  g.DrawLine(COLOR_RED, mRECT.L, mRECT.T, mRECT.R, mRECT.B, &BLEND_50, 2.f);
  g.DrawLine(COLOR_RED, mRECT.L, mRECT.B, mRECT.R, mRECT.T, &BLEND_50, 2.f);
  
  IRECT r = {};
  g.MeasureText(mHeightText, mHeightStr.Get(), r);
  g.FillRect(mBGColor, r.GetTranslated(mRECT.L + mInset, mRECT.MH()), &BLEND_50);
  g.DrawText(mHeightText, mHeightStr.Get(), mRECT.L + mInset, mRECT.MH());
  
  r = {};
  g.MeasureText(mWidthText, mWidthStr.Get(), r);
  g.FillRect(mBGColor, r.GetTranslated(mRECT.MW(), mRECT.T + mInset), &BLEND_75);
  g.DrawText(mWidthText, mWidthStr.Get(), mRECT.MW(), mRECT.T + mInset);
  
  r = {};
  g.MeasureText(mTLGCText, mTLHCStr.Get(), r);
  g.FillRect(mBGColor, r.GetTranslated(mRECT.L + mInset, mRECT.T + mInset), &BLEND_50);
  g.DrawText(mTLGCText, mTLHCStr.Get(), mRECT.L + mInset, mRECT.T + mInset);
  
  if (mStr.GetLength())
  {
    r = mRECT;
    g.MeasureText(mText, mStr.Get(), r);
    g.FillRect(mBGColor, r, &BLEND_75);
    g.DrawText(mText, mStr.Get(), r);
    
    mCentreLabelBounds = r;
  }
}

void PlaceHolder::OnResize()
{
  mTLHCStr.SetFormatted(32, "%0.1f, %0.1f", mRECT.L, mRECT.T);
  mWidthStr.SetFormatted(32, "%0.1f", mRECT.W());
  mHeightStr.SetFormatted(32, "%0.1f", mRECT.H());
}

IButtonControlBase::IButtonControlBase(const IRECT& bounds, IActionFunction aF)
: IControl(bounds, kNoParameter, aF)
{
  mDblAsSingleClick = true;
}

void IButtonControlBase::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  SetValue(1.);
  SetDirty(true);
}

void IButtonControlBase::OnEndAnimation()
{
  SetValue(0.);
  IControl::OnEndAnimation();
}

ISwitchControlBase::ISwitchControlBase(const IRECT& bounds, int paramIdx, IActionFunction aF, int numStates)
: IControl(bounds, paramIdx, aF)
, mNumStates(numStates)
{
  assert(mNumStates > 1);
  mDisabledState.Resize(numStates);
  SetAllStatesDisabled(false);
  mDblAsSingleClick = true;
}

void ISwitchControlBase::SetAllStatesDisabled(bool disabled)
{
  for(int i=0; i<mNumStates; i++)
  {
    SetStateDisabled(i, disabled);
  }
  SetDirty(false);
}

void ISwitchControlBase::SetStateDisabled(int stateIdx, bool disabled)
{
  if(stateIdx >= 0 && stateIdx < mNumStates)
    mDisabledState.Get()[stateIdx] = disabled;
  
  SetDirty(false);
}

bool ISwitchControlBase::GetStateDisabled(int stateIdx) const
{
  if(stateIdx >= 0 && stateIdx < mNumStates)
    return mDisabledState.Get()[stateIdx];
  return false;
}

void ISwitchControlBase::OnInit()
{
  if (GetParamIdx() > kNoParameter)
    mNumStates = (int) GetParam()->GetRange() + 1;
 
  assert(mNumStates > 1);
}

void ISwitchControlBase::OnMouseDown(float x, float y, const IMouseMod& mod)
{  
  if (mNumStates == 2)
    SetValue(!GetValue());
  else
  {
    const double step = 1. / (double(mNumStates) - 1.);
    double val = GetValue();
    val += step;
    if(val > 1.)
      val = 0.;
    SetValue(val);
  }
  
  mMouseDown = true;
  SetDirty(true);
}

void ISwitchControlBase::OnMouseUp(float x, float y, const IMouseMod& mod)
{
  mMouseDown = false;
  SetDirty(false);
}

bool IKnobControlBase::IsFineControl(const IMouseMod& mod, bool wheel) const
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

void IKnobControlBase::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  mMouseDown = true;
  mMouseDragValue = GetValue();

  if (mHideCursorOnDrag)
    GetUI()->HideMouseCursor(true, true);

  IControl::OnMouseDown(x, y, mod);
}

void IKnobControlBase::OnMouseUp(float x, float y, const IMouseMod& mod)
{
  if (mHideCursorOnDrag)
    GetUI()->HideMouseCursor(false);
  
  mMouseDown = false;
  SetDirty(false);
}

void IKnobControlBase::OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod)
{
  double gearing = IsFineControl(mod, false) ? mGearing * 10.0 : mGearing;
  
  IRECT dragBounds = GetKnobDragBounds();

  if (mDirection == EDirection::Vertical)
    mMouseDragValue += static_cast<double>(dY / static_cast<double>(dragBounds.T - dragBounds.B) / gearing);
  else
    mMouseDragValue += static_cast<double>(dX / static_cast<double>(dragBounds.R - dragBounds.L) / gearing);

  mMouseDragValue = Clip(mMouseDragValue, 0., 1.);

  double v = mMouseDragValue;
  const IParam* pParam = GetParam();
  
  if (pParam && pParam->GetStepped() && pParam->GetStep() > 0)
  {
    const double range = pParam->GetRange();
    
    if (range > 0.)
    {
      double l, h;
      pParam->GetBounds(l, h);

      v = l + mMouseDragValue * range;
      v = v - std::fmod(v, pParam->GetStep());
      v -= l;
      v /= range;
    }
  }

  SetValue(v);
  SetDirty();
}

void IKnobControlBase::OnMouseWheel(float x, float y, const IMouseMod& mod, float d)
{
  double gearing = IsFineControl(mod, true) ? 0.001 : 0.01;

  double v = GetValue() + gearing * d;
  const IParam* pParam = GetParam();
  
  if (pParam && pParam->GetStepped() && pParam->GetStep() > 0)
  {
    const double range = pParam->GetRange();

    if (range > 0. && d != 0.f)
    {
      double l, h;
      pParam->GetBounds(l,h);
      v = l + GetValue() * range;
      const double step = pParam->GetStep();
      v += d > 0 ? step : -step;
      v = Clip(v, l, h);
      v = v - std::fmod(v, step);
      v -= l;
      v /= range;
    }
  }

  SetValue(v);
  SetDirty();
}

ISliderControlBase::ISliderControlBase(const IRECT& bounds, int paramIdx, EDirection dir, double gearing, float handleSize)
: IControl(bounds, paramIdx)
, mDirection(dir)
, mHandleSize(handleSize)
, mGearing(gearing)
{
}

 ISliderControlBase::ISliderControlBase(const IRECT& bounds, IActionFunction aF, EDirection dir, double gearing, float handleSize)
: IControl(bounds, aF)
, mDirection(dir)
, mHandleSize(handleSize)
, mGearing(gearing)
{
}

void ISliderControlBase::OnResize()
{
  SetTargetRECT(mRECT);
  mTrackBounds = mRECT;
  SetDirty(false);
}

void ISliderControlBase::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  mMouseDown = true;
  
  if(GetParam())
  {
    if(!GetParam()->GetStepped())
      SnapToMouse(x, y, mDirection, mTrackBounds);
  }
  else
    SnapToMouse(x, y, mDirection, mTrackBounds);

  mMouseDragValue = GetValue();

  if (mHideCursorOnDrag)
    GetUI()->HideMouseCursor(true, true);

  IControl::OnMouseDown(x, y, mod);
}

void ISliderControlBase::OnMouseUp(float x, float y, const IMouseMod& mod)
{
  if (mHideCursorOnDrag)
    GetUI()->HideMouseCursor(false);
  
  mMouseDown = false;
  SetDirty(false);
}

void ISliderControlBase::OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod)
{
  const IParam* pParam = GetParam();

  if(mod.touchID || !mHideCursorOnDrag)
  {
    if(pParam)
    {
      if(!pParam->GetStepped())
      {
        SnapToMouse(x, y, mDirection, mTrackBounds);
        return;
      }
      // non-stepped parameter, fall through
    }
    else
    {
      SnapToMouse(x, y, mDirection, mTrackBounds);
      return;
    }
  }
  
  double gearing = IsFineControl(mod, false) ? mGearing * 10.0 : mGearing;

  if (mDirection == EDirection::Vertical)
    mMouseDragValue += static_cast<double>(dY / static_cast<double>(mTrackBounds.T - mTrackBounds.B) / gearing);
  else
    mMouseDragValue += static_cast<double>(dX / static_cast<double>(mTrackBounds.R - mTrackBounds.L) / gearing);
  
  mMouseDragValue = Clip(mMouseDragValue, 0., 1.);

  double v = mMouseDragValue;
  
  if (pParam && pParam->GetStepped() && pParam->GetStep() > 0)
  {
    const double range = pParam->GetRange();
    
    if (range > 0.)
    {
      double l, h;
      pParam->GetBounds(l,h);

      v = l + mMouseDragValue * range;
      v = v - std::fmod(v, pParam->GetStep());
      v -= l;
      v /= range;
    }
  }

  SetValue(v);
  SetDirty(true);
}

void ISliderControlBase::OnMouseWheel(float x, float y, const IMouseMod& mod, float d)
{
  double gearing = IsFineControl(mod, true) ? 0.001 : 0.01;

  double v = GetValue() + gearing * d;
  const IParam* pParam = GetParam();
  
  if (pParam && pParam->GetStepped() && pParam->GetStep() > 0)
  {
    const double range = pParam->GetRange();

    if (range > 0. && d != 0.f)
    {
      double l, h;
      pParam->GetBounds(l,h);
      v = l + GetValue() * range;
      const double step = pParam->GetStep();
      v += d > 0 ? step : -step;
      v = Clip(v, l, h);
      v = v - std::fmod(v, step);
      v -= l;
      v /= range;
    }
  }

  SetValue(v);
  SetDirty();
}

bool ISliderControlBase::IsFineControl(const IMouseMod& mod, bool wheel) const
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


IDirBrowseControlBase::~IDirBrowseControlBase()
{
  mFiles.Empty(true);
  mPaths.Empty(true);
  mPathLabels.Empty(true);
  mItems.Empty(false);
}

int IDirBrowseControlBase::NItems()
{
  return mItems.GetSize();
}

void IDirBrowseControlBase::AddPath(const char* path, const char* label)
{
  assert(strlen(path));

  mPaths.Add(new WDL_String(path));
  mPathLabels.Add(new WDL_String(label));
}

void IDirBrowseControlBase::CollectSortedItems(IPopupMenu* pMenu)
{
  int nItems = pMenu->NItems();
  
  for (int i = 0; i < nItems; i++)
  {
    IPopupMenu::Item* pItem = pMenu->GetItem(i);
    
    if(pItem->GetSubmenu())
      CollectSortedItems(pItem->GetSubmenu());
    else
      mItems.Add(pItem);
  }
}

void IDirBrowseControlBase::SetupMenu()
{
  mFiles.Empty(true);
  mItems.Empty(false);
  
  mMainMenu.Clear();
  mSelectedIndex = -1;

  int idx = 0;

  if (mPaths.GetSize() == 1)
  {
    ScanDirectory(mPaths.Get(0)->Get(), mMainMenu);
  }
  else
  {
    for (int p = 0; p<mPaths.GetSize(); p++)
    {
      IPopupMenu* pNewMenu = new IPopupMenu();
      mMainMenu.AddItem(mPathLabels.Get(p)->Get(), idx++, pNewMenu);
      ScanDirectory(mPaths.Get(p)->Get(), *pNewMenu);
    }
  }
  
  CollectSortedItems(&mMainMenu);
}

//void IDirBrowseControlBase::GetSelectedItemLabel(WDL_String& label)
//{
//  if (mSelectedMenu != nullptr) {
//    if(mSelectedIndex > -1)
//      label.Set(mSelectedMenu->GetItem(mSelectedIndex)->GetText());
//  }
//  else
//    label.Set("");
//}
//
//void IDirBrowseControlBase::GetSelectedItemPath(WDL_String& path)
//{
//  if (mSelectedMenu != nullptr) {
//    if(mSelectedIndex > -1) {
//      path.Set(mPaths.Get(0)->Get()); //TODO: what about multiple paths
//      path.AppendFormatted(1024, "/%s", mSelectedMenu->GetItem(mSelectedIndex)->GetText());
//      path.Append(mExtension.Get());
//    }
//  }
//  else
//    path.Set("");
//}

void IDirBrowseControlBase::ScanDirectory(const char* path, IPopupMenu& menuToAddTo)
{
  WDL_DirScan d;

  if (!d.First(path))
  {
    do
    {
      const char* f = d.GetCurrentFN();
      if (f && f[0] != '.')
      {
        if (d.GetCurrentIsDirectory())
        {
          WDL_String subdir;
          d.GetCurrentFullFN(&subdir);
          IPopupMenu* pNewMenu = new IPopupMenu();
          menuToAddTo.AddItem(d.GetCurrentFN(), pNewMenu, -2);
          ScanDirectory(subdir.Get(), *pNewMenu);
        }
        else
        {
          const char* a = strstr(f, mExtension.Get());
          if (a && a > f && strlen(a) == strlen(mExtension.Get()))
          {
            WDL_String menuEntry {f};
            
            if(!mShowFileExtensions)
              menuEntry.Set(f, (int) (a - f));
            
            IPopupMenu::Item* pItem = new IPopupMenu::Item(menuEntry.Get(), IPopupMenu::Item::kNoFlags, mFiles.GetSize());
            menuToAddTo.AddItem(pItem, -2 /* sort alphabetically */);
            WDL_String* pFullPath = new WDL_String("");
            d.GetCurrentFullFN(pFullPath);
            mFiles.Add(pFullPath);
          }
        }
      }
    } while (!d.Next());
  }
  
  if(!mShowEmptySubmenus)
    menuToAddTo.RemoveEmptySubmenus();
}
