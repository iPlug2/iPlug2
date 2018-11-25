#include "IControls.h"

#pragma mark - VECTOR CONTROLS

IVButtonControl::IVButtonControl(IGEditorDelegate& dlg, IRECT bounds, IActionFunction actionFunc, const char* str, const IVColorSpec& colorSpec)
: IButtonControlBase(dlg, bounds, actionFunc)
, IVectorBase(colorSpec)
{
  AttachIControl(this);
  mDblAsSingleClick = true;
  mText.mSize = 20; //FIXME: text size
  mStr.Set(str);
}

void IVButtonControl::Draw(IGraphics& g)
{
  IRECT handleBounds = DrawVectorButton(g, mRECT, (bool) mValue, mMouseIsOver);
  
  if(CStringHasContents(mStr.Get()))
    g.DrawText(mText, mStr.Get(), handleBounds);
}

IVSwitchControl::IVSwitchControl(IGEditorDelegate& dlg, IRECT bounds, int paramIdx, IActionFunction actionFunc
  , const char* str, const IVColorSpec& colorSpec, int numStates)
  : ISwitchControlBase(dlg, bounds, paramIdx, actionFunc, numStates)
  , IVectorBase(colorSpec)
{
  AttachIControl(this);
  mDblAsSingleClick = true;
  mText.mSize = 20; //FIXME: text size
  mStr.Set(str);
}

void IVSwitchControl::SetDirty(bool push)
{
  const IParam* pParam = GetParam();

  IControl::SetDirty(push);

  pParam->GetDisplayForHost(mStr);
}

void IVSwitchControl::Draw(IGraphics& g)
{
  IRECT handleBounds = DrawVectorButton(g, mRECT, mMouseDown, mMouseIsOver);
  
  if(CStringHasContents(mStr.Get()))
    g.DrawText(mText, mStr.Get(), handleBounds);
}

IVRadioButtonControl::IVRadioButtonControl(IGEditorDelegate& dlg, IRECT bounds, int paramIdx, IActionFunction actionFunc, const IVColorSpec& colorSpec, int numStates, EDirection dir)
: ISwitchControlBase(dlg, bounds, paramIdx, actionFunc, numStates)
, IVectorBase(colorSpec)
, mDirection(dir)
{
  AttachIControl(this);
  mDblAsSingleClick = true;
  mText.mSize = 20; //FIXME: text size
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
  int hit = int(0.5 + mValue * (double) (mNumStates - 1));
  
  for (int i = 0; i < mNumStates; i++)
  {
    IRECT r = mButtons.Get()[i];
    DrawVectorButton(g, r.FracRectHorizontal(0.25f).GetCentredInside(10.f), i == hit , mMouseIsOver);
    r = r.FracRectHorizontal(0.7f, true);
    i == hit ? mText.mFGColor = COLOR_WHITE : mText.mFGColor = COLOR_BLACK;
    g.DrawText(mText, mLabels.Get(i)->Get(), r);
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

IVKnobControl::IVKnobControl(IGEditorDelegate& dlg, IRECT bounds, int paramIdx,
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

IVKnobControl::IVKnobControl(IGEditorDelegate& dlg, IRECT bounds, IActionFunction actionFunction,
              const IVColorSpec& colorSpec,
              float aMin, float aMax,
              EDirection direction, double gearing)
: IKnobControlBase(dlg, bounds, kNoParameter, direction, gearing)
, IVectorBase(colorSpec)
, mAngleMin(aMin)
, mAngleMax(aMax)
{
  SetActionFunction(actionFunction);
  AttachIControl(this);
}

void IVKnobControl::Draw(IGraphics& g)
{
  g.FillRect(GetColor(kBG), mRECT);
  IRECT handleBounds = GetAdjustedHandleBounds(mRECT);
  handleBounds.ScaleAboutCentre(0.8f);

  const float v = mAngleMin + ((float)mValue * (mAngleMax - mAngleMin));
  const float cx = handleBounds.MW(), cy = handleBounds.MH();
  const float radius = (handleBounds.W()/2.f);

  g.DrawArc(GetColor(kFR), cx, cy, (mRECT.W()/2.f) - 5.f, mAngleMin, v, 0, 3.f);
  
  if (mDrawShadows && !mEmboss)
    g.FillCircle(GetColor(kSH), cx + mShadowOffset, cy + mShadowOffset, radius);
  
  g.FillCircle(GetColor(kFG), cx, cy, radius);

  g.DrawCircle(GetColor(kON), cx, cy, radius * 0.9f, 0, mFrameThickness);

  if(mMouseIsOver)
    g.FillCircle(GetColor(kHL), cx, cy, radius * 0.8f);
  
  g.DrawCircle(GetColor(kFR), cx, cy, radius, 0, mFrameThickness);
  g.DrawRadialLine(GetColor(kFR), cx, cy, v, 0.7f * radius, 0.9f * radius, 0, mFrameThickness);
}

#ifdef IGRAPHICS_NANOVG
#include "nanovg.h"
#endif

void IVSliderControl::Draw(IGraphics& g)
{
  g.FillRect(GetColor(kBG), mRECT);

  const float halfHandleSize = mHandleSize / 2.f;

  IRECT filledTrack = mTrack.FracRect(mDirection, (float) mValue);

  g.FillRect(GetColor(kFG), mTrack);
  g.FillRect(GetColor(kSH), filledTrack);
  
  if(mDirection == kVertical)
    g.FillCircle(GetColor(kFR), filledTrack.MW(), filledTrack.T , halfHandleSize);
  else
    g.FillCircle(GetColor(kFR), filledTrack.R, filledTrack.MH(), halfHandleSize);

  if(GetMouseIsOver())
  {
    if(mDirection == kVertical)
      g.FillCircle(GetColor(kSH), filledTrack.MW(), filledTrack.T , halfHandleSize);
    else
      g.FillCircle(GetColor(kSH), filledTrack.R, filledTrack.MH(), halfHandleSize);
  }
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

IBSliderControl::IBSliderControl(IGEditorDelegate& dlg, IRECT bounds, int paramIdx, IBitmap& bitmap,
                                 EDirection dir, bool onlyHandle)
: ISliderControlBase(dlg, bounds, paramIdx, dir, onlyHandle)
, IBitmapBase(bitmap)
{
  mTrack = bounds; // TODO: check
}

IBSliderControl::IBSliderControl(IGEditorDelegate& dlg, float x, float y, int len, int paramIdx, IBitmap& bitmap, EDirection dir, bool onlyHandle)
: ISliderControlBase(dlg, IRECT(x, y, x + bitmap.W(), y + len), paramIdx)
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

