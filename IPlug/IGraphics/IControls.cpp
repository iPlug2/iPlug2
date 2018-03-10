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

  mStep = 1.f / float(mNumStates) - 1.f;
}

void IVSwitchControl::Draw(IGraphics& g)
{
  const float cornerRadius = mRoundness * (mRECT.W() / 2.);

  g.FillRoundRect(GetColor(kBG), mRECT, mRoundness, &mBlend);

  IRECT handleBounds = GetHandleBounds();
  IColor shadowColor = IColor(60, 0, 0, 0);

  if (mValue > 0.5)
  {
    g.FillRoundRect(GetColor(kPR), handleBounds, cornerRadius, &mBlend);

    if (mDrawShadows && mEmboss)
    {
      g.PathRect(handleBounds.GetHSliced(mShadowOffset));
      g.PathRect(handleBounds.GetVSliced(mShadowOffset));
      g.PathFill(shadowColor);
    }
  }
  else
  {
    if (mDrawShadows && !mEmboss)
      g.FillRoundRect(shadowColor, handleBounds.GetShifted(mShadowOffset, mShadowOffset), cornerRadius, &mBlend);

    g.FillRoundRect(GetColor(kFG), handleBounds, cornerRadius, &mBlend);
  }

 if(mDrawFrame)
   g.DrawRoundRect(GetColor(kFR), handleBounds, cornerRadius, &mBlend);
}

IRECT IVSwitchControl::GetHandleBounds()
{
  IRECT handleBounds = mRECT;

  if (mDrawShadows && !mEmboss)
    handleBounds.GetShifted(0, 0, -mShadowOffset, -mShadowOffset);

  return handleBounds;
}

IVKnobControl::IVKnobControl(IDelegate& dlg, IRECT bounds, int param,
                             const IVColorSpec& colorSpec,
                             float rMin, float rMax, float aMin, float aMax,
                             EDirection direction, double gearing)
: IKnobControlBase(dlg, bounds, param, direction, gearing)
, IVectorBase(colorSpec)
, mAngleMin(aMin)
, mAngleMax(aMax)
, mInnerRadius(rMin)
, mOuterRadius(rMax)
{
  AttachIControl(this);

  if (mOuterRadius == 0.0f)
    mOuterRadius = 0.5f * (float) bounds.W();
}

void IVKnobControl::Draw(IGraphics& g)
{
  const float v = mAngleMin + ((float)mValue * (mAngleMax - mAngleMin));
  const float cx = mRECT.MW(), cy = mRECT.MH();
  const float radius = (mRECT.W()/2.f) - 2.f;
//  g.FillCircle(GetColor(EVColor::kFR), cx, cy, radius+2);
//  g.DrawCircle(GetColor(EVColor::kBG), cx, cy, radius, &BLEND_50, 5.f);
  g.FillArc(GetColor(EVColor::kBG), cx, cy, radius, mAngleMin, v);
  g.DrawRadialLine(GetColor(EVColor::kFG), cx, cy, v, mInnerRadius * radius, mOuterRadius * radius, 0, 5.f);
}

void IVSliderControl::Draw(IGraphics& g)
{
  g.FillRoundRect(GetColor(kBG), mRECT, 5);

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

  g.FillRect(GetColor(kFG), filledTrack, &mBlend);
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
  g.DrawBitmap(mHandleBitmap, mRECT, 1, &mBlend);
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
//  g.DrawBitmap(mHandleBitmap, r, 1, &mBlend);
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


