#include "IControls.h"

#pragma mark - VECTOR CONTROLS

IVSwitchControl::IVSwitchControl(IGEditorDelegate& dlg, IRECT bounds, int paramIdx, IActionFunction actionFunc
  , const IVColorSpec& colorSpec, int numStates, EDirection dir)
  : ISwitchControlBase(dlg, bounds, paramIdx, actionFunc, numStates)
  , IVectorBase(colorSpec)
  , mDirection(dir)
{
  AttachIControl(this);
  mDblAsSingleClick = true;
  mText.mSize = 20;
  mStep = 1.f / float(mNumStates) - 1.f;
}

void IVSwitchControl::Draw(IGraphics& g)
{
  g.FillRect(GetColor(kBG), mRECT);

  IRECT handleBounds = GetAdjustedHandleBounds(mRECT);
  const float cornerRadius = mRoundness * (handleBounds.W() / 2.f);

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
  
  g.DrawText(mText, mStr.Get(), handleBounds);
  
  if(GetAnimationFunction())
    DefaultClickAnimation(g);
    
  if(mDrawFrame)
    g.DrawRoundRect(GetColor(kFR), handleBounds, cornerRadius, 0, mFrameThickness);
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
#ifdef IGRAPHICS_NANOVG
  
  NVGcontext* vg = (NVGcontext*) g.GetDrawContext();
//
  const int h = mTrack.H();
  const int w = mTrack.W();
  const int x = mTrack.L;
  const int y = mTrack.T;
  const float pos = mValue;
//
  NVGpaint bg, knob;
  float cy = y+(int)(h*0.5f);
  float kr = mHandleSize-2.f;

  nvgSave(vg);
  //  nvgClearState(vg);

//   Slot
  bg = nvgBoxGradient(vg, x,cy-2+1, w,4, 2,2, nvgRGBA(0,0,0,32), nvgRGBA(0,0,0,128));
  nvgBeginPath(vg);
  nvgRoundedRect(vg, x,cy-2, w,4, 2);
  nvgFillPaint(vg, bg);
  nvgFill(vg);
  

  // Knob Shadow
  bg = nvgRadialGradient(vg, x+(int)(pos*w),cy+1, kr-3,kr+3, nvgRGBA(0,0,0,64), nvgRGBA(0,0,0,0));
  nvgBeginPath(vg);
  nvgRect(vg, x+(int)(pos*w)-kr-5,cy-kr-5,kr*2+5+5,kr*2+5+5+3);
  nvgCircle(vg, x+(int)(pos*w),cy, kr);
  nvgPathWinding(vg, NVG_HOLE);
  nvgFillPaint(vg, bg);
  nvgFill(vg);

  // Knob
  knob = nvgLinearGradient(vg, x,cy-kr,x,cy+kr, nvgRGBA(255,255,255,16), nvgRGBA(0,0,0,16));
  nvgBeginPath(vg);
  nvgCircle(vg, x+(int)(pos*w),cy, kr-1);
  
  if(GetMouseIsOver())
    nvgFillColor(vg, nvgRGBA(200,200,200,255));
  else
    nvgFillColor(vg, nvgRGBA(255,255,255,255));

  nvgFill(vg);
  nvgFillPaint(vg, knob);
  nvgFill(vg);

  nvgBeginPath(vg);
  nvgCircle(vg, x+(int)(pos*w),cy, kr-0.5f);
  nvgStrokeColor(vg, nvgRGBA(0,0,0,92));
  nvgStroke(vg);

  nvgRestore(vg);
#else
  g.FillRect(GetColor(kBG), mRECT);

  const float halfHandleSize = mHandleSize / 2.f;

  IRECT filledTrack = mTrack.FracRect(mDirection, mValue);

  g.FillRect(GetColor(kFG), mTrack);
  g.FillRect(GetColor(kSH), filledTrack);
  
  if(mDirection == kVertical)
    g.FillCircle(GetColor(kX1), filledTrack.MW(), filledTrack.T , halfHandleSize);
  else
    g.FillCircle(GetColor(kX1), filledTrack.R, filledTrack.MH(), halfHandleSize);

  if(GetMouseIsOver())
  {
    if(mDirection == kVertical)
      g.FillCircle(GetColor(kSH), filledTrack.MW(), filledTrack.T , halfHandleSize);
    else
      g.FillCircle(GetColor(kX1), filledTrack.R, filledTrack.MH(), halfHandleSize);
  }
#endif
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

