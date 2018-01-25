#include "IControls.h"

IButtonControlBase::IButtonControlBase(IPlugBaseGraphics& plug, IRECT rect, int paramIdx,  std::function<void(IControl*)> actionFunc,
                                       uint32_t numStates )
: IControl(plug, rect, paramIdx, actionFunc)
{
  if(paramIdx > -1)
    mNumStates = (uint32_t) mPlug.GetParam(paramIdx)->GetRange() + 1;
  else
    mNumStates = numStates;
  
  assert(mNumStates > 1);
}

void IButtonControlBase::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  if(mNumStates == 2)
    mValue = !mValue;
  else
  {
    const float step = 1.f/float(mNumStates) - 1.;
    mValue += step;
    mValue = fmod(1., mValue);
  }
  
  if (mActionFunc != nullptr)
    mActionFunc(this);
  
  SetDirty();
}

IVSwitchControl::IVSwitchControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx, std::function<void (IControl*)> actionFunc
                                 ,const IColor& fgColor, const IColor &bgColor, uint32_t numStates, EDirection dir)
:IButtonControlBase(plug, rect, paramIdx, actionFunc, numStates)
, mFGColor(fgColor)
, mBGColor(bgColor)
, mDirection(dir)
{
  mStep = 1.f/float(mNumStates) - 1.;
}

void IVSwitchControl::Draw(IGraphics& graphics)
{
  const int state = mValue / mStep;
  
  graphics.FillRect(mBGColor, mRECT, &mBlend);
  
  IRECT handle;
  
  if(mNumStates > 2)
  {
    if(mDirection == kHorizontal)
      handle = mRECT.SubRectHorizontal(mNumStates, state);
    if(mDirection == kVertical)
      handle = mRECT.SubRectVertical(mNumStates, state);
  }
  else
    handle = mRECT;
  
  graphics.FillRect(mFGColor, handle.GetPadded(-10), &mBlend);
}

void IBSwitchControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  if (mBitmap.N > 1)
  {
    mValue += 1.0 / (double) (mBitmap.N - 1);
  }
  else
  {
    mValue += 1.0;
  }
  
  if (mValue > 1.001)
  {
    mValue = 0.0;
  }
  SetDirty();
}

void IKnobControlBase::OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod)
{
  double gearing = mGearing;

#ifdef PROTOOLS
#ifdef OS_WIN
  if (mod.C) gearing *= 10.0;
#else
  if (mod.R) gearing *= 10.0;
#endif
#else
  if (mod.C || mod.S) gearing *= 10.0;
#endif

  if (mDirection == kVertical)
  {
    mValue += (double) dY / (double) (mRECT.T - mRECT.B) / gearing;
  }
  else
  {
    mValue += (double) dX / (double) (mRECT.R - mRECT.L) / gearing;
  }

  SetDirty();
}

void IKnobControlBase::OnMouseWheel(float x, float y, const IMouseMod& mod, float d)
{
#ifdef PROTOOLS
  if (mod.C)
  {
    mValue += 0.001 * d;
  }
#else
  if (mod.C || mod.S)
  {
    mValue += 0.001 * d;
  }
#endif
  else
  {
    mValue += 0.01 * d;
  }

  SetDirty();
}

IVKnobControl::IVKnobControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx,
                             const IColor& fgcolor, const IColor& bgcolor,
                             float rMin, float rMax, float aMin, float aMax, EDirection direction, double gearing)
: IKnobControlBase(plug, rect, paramIdx, direction, gearing)
, mFGColor(fgcolor)
, mBGColor(bgcolor)
, mAngleMin(aMin)
, mAngleMax(aMax)
, mInnerRadius(rMin)
, mOuterRadius(rMax)
{
  if (mOuterRadius == 0.0f)
  {
    mOuterRadius = 0.5f * (float) rect.W();
  }
}

void IVKnobControl::Draw(IGraphics& graphics)
{
  const float v = mAngleMin + (mValue * (mAngleMax - mAngleMin));
  const float cx = mRECT.MW(), cy = mRECT.MH();
  const float radius = (mRECT.W()/2.f) - 2.f;
  graphics.DrawCircle(mFGColor, cx, cy, radius, &BLEND_50);
  graphics.FillArc(mBGColor, cx, cy, radius, mAngleMin, v, &BLEND_50);
  graphics.DrawRadialLine(mFGColor, cx, cy, v, mInnerRadius * radius, mOuterRadius * radius);
}

IBSliderControl::IBSliderControl(IPlugBaseGraphics& plug, float x, float y, int len, int paramIdx, IBitmap& bitmap, EDirection direction, bool onlyHandle)
: IControl(plug, IRECT(), paramIdx)
, mLen(len), mHandleBitmap(bitmap), mDirection(direction), mOnlyHandle(onlyHandle)
{
  if (direction == kVertical)
  {
    mHandleHeadroom = mHandleBitmap.H;
    mRECT = mTargetRECT = IRECT(x, y, x + mHandleBitmap.W, y + len);
  }
  else
  {
    mHandleHeadroom = mHandleBitmap.W;
    mRECT = mTargetRECT = IRECT(x, y, x + len, y + mHandleBitmap.H);
  }
}

IRECT IBSliderControl::GetHandleRECT(double value) const
{
  if (value < 0.0)
  {
    value = mValue;
  }
  IRECT r(mRECT.L, mRECT.T, mRECT.L + mHandleBitmap.W, mRECT.T + mHandleBitmap.H);
  if (mDirection == kVertical)
  {
    int offs = int((1.0 - value) * (double) (mLen - mHandleHeadroom));
    r.T += offs;
    r.B += offs;
  }
  else
  {
    int offs = int(value * (double) (mLen - mHandleHeadroom));
    r.L += offs;
    r.R += offs;
  }
  return r;
}

void IBSliderControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
#ifdef PROTOOLS
  if (mod.A)
  {
    if (mDefaultValue >= 0.0)
    {
      mValue = mDefaultValue;
      SetDirty();
      return;
    }
  }
  else
#endif
    if (mod.R)
    {
      PromptUserInput();
      return;
    }
  
  return SnapToMouse(x, y);
}

void IBSliderControl::OnMouseWheel(float x, float y, const IMouseMod& mod, float d)
{
#ifdef PROTOOLS
  if (mod.C)
  {
    mValue += 0.001 * d;
  }
#else
  if (mod.C || mod.S)
  {
    mValue += 0.001 * d;
  }
#endif
  else
  {
    mValue += 0.01 * d;
  }
  
  SetDirty();
}

void IBSliderControl::SnapToMouse(float x, float y)
{
  if (mDirection == kVertical)
  {
    mValue = 1.0 - (double) (y - mRECT.T - mHandleHeadroom / 2) / (double) (mLen - mHandleHeadroom);
  }
  else
  {
    mValue = (double) (x - mRECT.L - mHandleHeadroom / 2) / (double) (mLen - mHandleHeadroom);
  }
  SetDirty();
}

void IBSliderControl::Draw(IGraphics& graphics)
{
  IRECT r = GetHandleRECT();
  graphics.DrawBitmap(mHandleBitmap, r, 1, &mBlend);
}

bool IBSliderControl::IsHit(float x, float y) const
{
  if(mOnlyHandle)
  {
    IRECT r = GetHandleRECT();
    return r.Contains(x, y);
  }
  else
  {
    return mTargetRECT.Contains(x, y);
  }
}

void IBSliderControl::OnRescale()
{
  mHandleBitmap = GetGUI()->GetScaledBitmap(mHandleBitmap);
}
