#include "IControls.h"

#pragma mark - VECTOR CONTROLS

IVSwitchControl::IVSwitchControl(IEditorDelegate& dlg, IRECT bounds, int paramIdx, IActionFunction actionFunc
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
  g.FillRect(GetColor(kBG), mRECT);

  const IRECT handleBounds = GetAdjustedHandleBounds(mRECT);
  const float cornerRadius = mRoundness * (handleBounds.W() / 2.);

  if (mValue > 0.5)
  {
    g.FillRoundRect(GetColor(kPR), handleBounds, cornerRadius);

    //inner shadow
    if (mDrawShadows && mEmboss)
    {
      g.PathRect(handleBounds.GetHSliced(mShadowOffset));
      g.PathRect(handleBounds.GetVSliced(mShadowOffset));
      g.PathFill(GetColor(kSH));
    }
  }
  else
  {
    //outer shadow
    if (mDrawShadows && !mEmboss)
      g.FillRoundRect(GetColor(kSH), handleBounds.GetShifted(mShadowOffset, mShadowOffset), cornerRadius);

    g.FillRoundRect(GetColor(kFG), handleBounds, cornerRadius);
  }
  
  if(mMouseIsOver)
    g.FillRoundRect(GetColor(kHL), handleBounds, cornerRadius);
  
  if(GetAnimationFunction())
    DefaultClickAnimation(g);
    
  if(mDrawFrame)
    g.DrawRoundRect(GetColor(kFR), handleBounds, cornerRadius, 0, mFrameThickness);
}

IVKnobControl::IVKnobControl(IEditorDelegate& dlg, IRECT bounds, int paramIdx,
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

  const float v = mAngleMin + ((float)mValue * (mAngleMax - mAngleMin));
  const float cx = handleBounds.MW(), cy = handleBounds.MH();
  const float radius = (handleBounds.W()/2.f);

  g.DrawArc(GetColor(kFR), cx, cy, (mRECT.W()/2.f) - 5.f, mAngleMin, v, 0, 3.f);
  
  if (mDrawShadows && !mEmboss)
    g.FillCircle(GetColor(kSH), cx + mShadowOffset, cy + mShadowOffset, radius);
  
  g.FillCircle(GetColor(kFG), cx, cy, radius);

  g.DrawCircle(GetColor(kON), cx, cy, radius * 0.9, 0, mFrameThickness);

  if(mMouseIsOver)
    g.FillCircle(GetColor(kHL), cx, cy, radius * 0.8f);
  
  g.DrawCircle(GetColor(kFR), cx, cy, radius, 0, mFrameThickness);
  g.DrawRadialLine(GetColor(kFR), cx, cy, v, 0.7f * radius, 0.9f * radius, 0, mFrameThickness);
}

void IVSliderControl::Draw(IGraphics& g)
{
  g.FillRect(GetColor(kBG), mRECT);

  const float halfHandleSize = mHandleSize / 2.f;

  IRECT filledTrack = mTrack.FracRect(mDirection, mValue);

  g.FillRect(GetColor(kFG), mTrack);
  g.FillRect(GetColor(kSH), filledTrack);
  
  g.FillCircle(GetColor(kX1), filledTrack.R, filledTrack.MH(), halfHandleSize);
  
  if(GetMouseIsOver())
    g.FillCircle(GetColor(kSH), filledTrack.R, filledTrack.MH(), halfHandleSize);
}

void IVSliderControl::OnResize()
{
  if(mDirection == kVertical)
    mTrack = mRECT.GetPadded(-mHandleSize).GetMidHPadded(mTrackSize);
  else
    mTrack = mRECT.GetPadded(-mHandleSize).GetMidVPadded(mTrackSize);

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

IBSliderControl::IBSliderControl(IEditorDelegate& dlg, IRECT bounds, int paramIdx, IBitmap& bitmap,
                                 EDirection dir, bool onlyHandle)
: ISliderControlBase(dlg, bounds, paramIdx, dir, onlyHandle)
, IBitmapBase(bitmap)
{
}

IBSliderControl::IBSliderControl(IEditorDelegate& dlg, float x, float y, int len, int paramIdx, IBitmap& bitmap, EDirection direction, bool onlyHandle)
: ISliderControlBase(dlg, IRECT(x, y, x + bitmap.W(), y + len), paramIdx)
, IBitmapBase(bitmap)
{
  if (direction == kVertical)
  {
    mRECT = mTargetRECT = IRECT(x, y, x + bitmap.W(), y + len);
    mTrack = mRECT.GetPadded(0, bitmap.H(), 0, 0);
  }
  else
  {
    mRECT = mTargetRECT = IRECT(x, y, x + len, y + bitmap.H());
    mTrack = mRECT.GetPadded(0, 0, bitmap.W(), 0);
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
    float offs = (1.f - value) * mTrack.H();
    r.T += offs;
    r.B += offs;
  }
  else
  {
    float offs = value * mTrack.W();
    r.L += offs;
    r.R += offs;
  }
  return r;
}

