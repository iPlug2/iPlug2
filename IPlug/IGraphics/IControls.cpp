#include "IControls.h"

#pragma mark - VECTOR CONTROLS

IVSwitchControl::IVSwitchControl(IDelegate& dlg, IRECT bounds, int paramIdx, std::function<void(IControl*)> actionFunc
  , const IVColorSpec& colorSpec, int numStates, EDirection dir)
  : ISwitchControlBase(dlg, bounds, paramIdx, actionFunc, numStates)
  , IVectorBase(colorSpec)
  , mDirection(dir)
{
  AttachIControl(this);
  mDblAsSingleClick = true;
  
  SetActionFunction(DefaultClickActionFunc);

  mStep = 1.f / float(mNumStates) - 1.f;
}

void IVSwitchControl::Draw(IGraphics& g)
{
  g.FillRect(GetColor(kBG), mRECT);

  const IRECT handleBounds = GetAdjustedHandleBounds(mRECT);
  
  const float cornerRadius = mRoundness * (handleBounds.W() / 2.);

  static const IColor shadowColor = IColor(60, 0, 0, 0);

  if (mValue > 0.5)
  {
    g.FillRoundRect(GetColor(kPR), handleBounds, cornerRadius);

    //inner shadow
    if (mDrawShadows && mEmboss)
    {
      g.PathRect(handleBounds.GetHSliced(mShadowOffset));
      g.PathRect(handleBounds.GetVSliced(mShadowOffset));
      g.PathFill(shadowColor);
    }
  }
  else
  {
    //outer shadow
    if (mDrawShadows && !mEmboss)
      g.FillRoundRect(shadowColor, handleBounds.GetShifted(mShadowOffset, mShadowOffset), cornerRadius);

    g.FillRoundRect(GetColor(kFG), handleBounds, cornerRadius);
  }
  
  if(mMouseIsOver)
    g.FillRoundRect(GetColor(kHL), handleBounds, cornerRadius);
  
  if(GetAnimationFunction())
  {
    float mouseDownX, mouseDownY;
    g.GetMouseDownPoint(mouseDownX, mouseDownY);
    g.FillCircle(GetColor(kHL), mouseDownX, mouseDownY, mFlashCircleRadius);
  }
  
  if(mDrawFrame)
    g.DrawRoundRect(GetColor(kFR), handleBounds, cornerRadius, 0, mStrokeThickness);
}

IVKnobControl::IVKnobControl(IDelegate& dlg, IRECT bounds, int paramIdx,
                             const IVColorSpec& colorSpec,
                             float aMin, float aMax,
                             EDirection direction, double gearing)
: IKnobControlBase(dlg, bounds, paramIdx, direction, gearing)
, IVectorBase(colorSpec)
, mAngleMin(aMin)
, mAngleMax(aMax)
{
  AttachIControl(this);
}

void IVKnobControl::Draw(IGraphics& g)
{
  g.FillRect(GetColor(kBG), mRECT);
  
  IRECT handleBounds = GetAdjustedHandleBounds(mRECT);
  
  handleBounds.ScaleAboutCentre(0.80);

  static const IColor shadowColor = IColor(60, 0, 0, 0);

  const float v = mAngleMin + ((float)mValue * (mAngleMax - mAngleMin));
  const float cx = handleBounds.MW(), cy = handleBounds.MH();
  const float radius = (handleBounds.W()/2.f);

  g.DrawArc(GetColor(kFR), cx, cy, (mRECT.W()/2.f) - 5.f, mAngleMin, v, 0, 3.f);
  
  if (mDrawShadows && !mEmboss)
    g.FillCircle(shadowColor, cx + mShadowOffset, cy + mShadowOffset, radius); 
  
  g.FillCircle(GetColor(kFG), cx, cy, radius);

  g.DrawCircle(GetColor(kON), cx, cy, radius * 0.9, 0, mStrokeThickness);

  if(mMouseIsOver)
    g.FillCircle(GetColor(kHL), cx, cy, radius * 0.8f);
  
  g.DrawCircle(GetColor(kFR), cx, cy, radius, 0, mStrokeThickness);
  g.DrawRadialLine(GetColor(kFR), cx, cy, v, 0.7f * radius, 0.9f * radius, 0, mStrokeThickness);
}

void IVSliderControl::Draw(IGraphics& g)
{
  g.FillRect(GetColor(kBG), mRECT);

  IRECT filledTrack, handle;

  if(mDirection == kVertical)
  {
    const float halfHandleSize = mHandleSize / 2.f;

    const float handleTop = mTrack.B - (mValue * mTrack.H());
    const float handleBottom = handleTop - halfHandleSize;

    const float filledTrackTop = mTrack.B - (mValue * (mTrack.H()));
    const float filledTrackBottom = mTrack.B;

    filledTrack = IRECT(mTrack.L, filledTrackTop, mTrack.R, filledTrackBottom);
    handle = IRECT(mTrack.L, handleTop, mTrack.R, handleBottom);
  }
  else
  {
    //TODO:
  }

  g.FillRect(GetColor(kFG), filledTrack);
}

void IVSliderControl::OnResize()
{
  if(mDirection == kVertical)
    mTrack = mRECT.GetPadded(-mHandleSize).GetMidHPadded(5);
  else
    mTrack = mRECT.GetPadded(-mHandleSize).GetMidVPadded(5);

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

IBSliderControl::IBSliderControl(IDelegate& dlg, IRECT bounds, int paramIdx, IBitmap& handleBitmap,
                                 EDirection dir, bool onlyHandle)
: ISliderControlBase(dlg, bounds, paramIdx, dir, onlyHandle)
, mHandleBitmap(handleBitmap)
{
}

void IBSliderControl::Draw(IGraphics& g)
{
//  IRECT r = GetHandleRECT();
  g.DrawBitmap(mHandleBitmap, mRECT, 1);
}

void IBSliderControl::OnRescale()
{
  mHandleBitmap = GetUI()->GetScaledBitmap(mHandleBitmap);
}

void IBSliderControl::OnResize()
{
  if (mDirection == kVertical)
    mTrack = mTargetRECT = mRECT.GetVPadded(-mHandleBitmap.H());
  else
    mTrack = mTargetRECT = mRECT.GetHPadded(-mHandleBitmap.W());

  SetDirty();
}

//IBSliderControl::IBSliderControl(IDelegate& dlg, float x, float y, int len, int paramIdx, IBitmap& bitmap, EDirection direction, bool onlyHandle)
//: IControl(dlg, IRECT(), paramIdx)
//, mLen(len), mHandleBitmap(bitmap), mDirection(direction), mOnlyHandle(onlyHandle)
//{
//  if (direction == kVertical)
//  {
//    mHandleHeadroom = mHandleBitmap.H();
//    mRECT = mTargetRECT = IRECT(x, y, x + mHandleBitmap.W(), y + len);
//  }
//  else
//  {
//    mHandleHeadroom = mHandleBitmap.W();
//    mRECT = mTargetRECT = IRECT(x, y, x + len, y + mHandleBitmap.H());
//  }
//}
//
//IRECT IBSliderControl::GetHandleRECT(double value) const
//{
//  if (value < 0.0)
//  {
//    value = mValue;
//
//  IRECT r(mRECT.L, mRECT.T, mRECT.L + mHandleBitmap.W(), mRECT.T + mHandleBitmap.H());
//
//  if (mDirection == kVertical)
//  {
//    int offs = int((1.0 - value) * (double) (mLen - mHandleHeadroom));
//    r.T += offs;
//    r.B += offs;
//  }
//  else
//  {
//    int offs = int(value * (double) (mLen - mHandleHeadroom));
//    r.L += offs;
//    r.R += offs;
//  }
//  return r;
//}
//
//void IBSliderControl::OnMouseDown(float x, float y, const IMouseMod& mod)
//{
//#ifdef PROTOOLS
//  if (mod.A)
//  {
//    if (mDefaultValue >= 0.0)
//    {
//      mValue = mDefaultValue;
//      SetDirty();
//      return;
//    }
//  }
//  else
//#endif
//    if (mod.R)
//    {
//      PromptUserInput();
//      return;
//    }
//
//  return SnapToMouse(x, y);
//}
//
//void IBSliderControl::OnMouseWheel(float x, float y, const IMouseMod& mod, float d)
//{
//#ifdef PROTOOLS
//  if (mod.C)
//    mValue += 0.001 * d;
//#else
//  if (mod.C || mod.S)
//    mValue += 0.001 * d;
//#endif
//  else
//    mValue += 0.01 * d;
//
//  SetDirty();
//}
//
//void IBSliderControl::SnapToMouse(float x, float y)
//{
//  if (mDirection == kVertical)
//    mValue = 1.0 - (double) (y - mRECT.T - mHandleHeadroom / 2) / (double) (mLen - mHandleHeadroom);
//  else
//    mValue = (double) (x - mRECT.L - mHandleHeadroom / 2) / (double) (mLen - mHandleHeadroom);
//
//  SetDirty();
//}
//
//void IBSliderControl::Draw(IGraphics& g)
//{
//  IRECT r = GetHandleRECT();
//  g.DrawBitmap(mHandleBitmap, r, 1);
//}
//
//bool IBSliderControl::IsHit(float x, float y) const
//{
//  if(mOnlyHandle)
//  {
//    IRECT r = GetHandleRECT();
//    return r.Contains(x, y);
//  }
//  else
//  {
//    return mTargetRECT.Contains(x, y);
//  }
//}
//
//void IBSliderControl::OnRescale()
//{
//  mHandleBitmap = GetUI()->GetScaledBitmap(mHandleBitmap);
//}
//
//void IBSliderControl::OnResize()
//{
//  if (mDirection == kVertical)
//    mTrack = mTargetRECT = mRECT.GetVPadded(-mHandleBitmap.H());
//  else
//    mTrack = mTargetRECT = mRECT.GetHPadded(-mHandleBitmap.W());
//
//  SetDirty(false);
//}


