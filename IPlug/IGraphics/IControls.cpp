#include "IControls.h"

#pragma mark - VECTOR CONTROLS

IVSwitchControl::IVSwitchControl(IDelegate& dlg, IRECT bounds, int paramIdx, std::function<void(IControl*)> actionFunc
  , const IVColorSpec& colorSpec, uint32_t numStates, EDirection dir)
  : ISwitchControlBase(dlg, bounds, paramIdx, actionFunc, numStates)
  , IVectorBase(colorSpec)
  , mDirection(dir)
{
  mStep = 1.f / float(mNumStates) - 1.f;
}

void IVSwitchControl::Draw(IGraphics& g)
{
//  const int state = (int)std::round(mValue / mStep);

  g.FillRoundRect(GetColor(EVColor::kBG), mRECT, mRoundness, &mBlend);

//
  IRECT handle;
//
//  if (mNumStates > 2)
//  {
//    if (mDirection == kHorizontal)
//      handle = mRECT.SubRectHorizontal(mNumStates, state);
//    if (mDirection == kVertical)
//      handle = mRECT.SubRectVertical(mNumStates, state);
//  }
//  else
    handle = mRECT;
//
 // g.FillRect(GetColor(EVColor::kFG), handle.GetPadded(-10), &mBlend);
//  g.FillCircle(GetColor(EVColor::kFG), handle.MW(), handle.MH(), handle.W()/2., &mBlend);

  g.FillEllipse(GetColor(EVColor::kFG), mRECT, &mBlend);
  //g.DrawRect(GetColor(EVColor::kFR), mRECT.GetPadded(-5), &mBlend);
//  g.FillCircle(GetColor(EVColor::kFR), handle.MW(), handle.MH(), (handle.W()/2.)-2, GetMouseIsOver() ? &BLEND_25 : &BLEND_10);
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


IVButtonControl::IVButtonControl(IDelegate& dlg, IRECT rect, int param,
                                 const char *txtOff, const char *txtOn)
  : IControl(dlg, rect, param),
    IVectorBase(&DEFAULT_BG_COLOR, &DEFAULT_TXT_COLOR, &DEFAULT_FR_COLOR, &DEFAULT_PR_COLOR)
{
  mText.mFGColor = GetColor(bTXT);
  SetTexts(txtOff, txtOn);
  mDblAsSingleClick = true;
};

const IColor IVButtonControl::DEFAULT_BG_COLOR = IColor(255, 200, 200, 200);
const IColor IVButtonControl::DEFAULT_FR_COLOR = IColor(255, 70, 70, 70);
const IColor IVButtonControl::DEFAULT_TXT_COLOR = DEFAULT_FR_COLOR;
const IColor IVButtonControl::DEFAULT_PR_COLOR = IColor(255, 240, 240, 240);

void IVButtonControl::Draw(IGraphics& g)
{
  auto btnRect = GetButtonRect();
  auto shadowColor = IColor(60, 0, 0, 0);

  if (mValue > 0.5)
  {
    g.FillRect(GetColor(bPR), btnRect);

    if (mDrawShadows && mEmboss)
      DrawInnerShadowForRect(btnRect, shadowColor, g);

    if (mTxtOn.GetLength())
    {
      auto textR = GetRectToAlignTextIn(btnRect, 1);
      g.DrawText(mText, mTxtOn.Get(), textR);
    }
  }
  else
  {
    if (mDrawShadows && !mEmboss)
      DrawOuterShadowForRect(btnRect, shadowColor, g);

    g.FillRect(GetColor(bBG), btnRect);

    if (mTxtOff.GetLength())
    {
      auto textR = GetRectToAlignTextIn(btnRect, 0);
      g.DrawText(mText, mTxtOff.Get(), textR);
    }
  }

  if(mDrawBorders)
    g.DrawRect(GetColor(bFR), btnRect);
}

IRECT IVButtonControl::GetRectToAlignTextIn(IRECT r, int state)
{
  // this rect is not precise, it serves as a horizontal level
  auto tr = r;
  tr.T += 0.5f * (tr.H() - mText.mSize * mTxtH[state]) - 1.0f; // -1 looks better with small text
  tr.B = tr.T + 0.1f;
  return tr;
}

IRECT IVButtonControl::GetButtonRect()
{
  auto br = mRECT;
  if (mDrawShadows && !mEmboss)
  {
    br.R -= mShadowOffset;
    br.B -= mShadowOffset;
  }
  return br;
}

void IVButtonControl::DrawInnerShadowForRect(IRECT r, IColor shadowColor, IGraphics& g)
{
  auto& o = mShadowOffset;
  auto slr = r;
  slr.R = slr.L + o;
  auto str = r;
  str.L += o;
  str.B = str.T + o;
  g.FillRect(shadowColor, slr);
  g.FillRect(shadowColor, str);
}

void IVButtonControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  if (mValue > 0.5) mValue = 0.0;
  else mValue = 1.0;
  SetDirty();
}

void IVButtonControl::SetTexts(const char *txtOff, const char *txtOn, bool fitToText, float pad)
{
  mTxtOff.Set(txtOff);
  mTxtOn.Set(txtOn);
  BasicTextMeasure(mTxtOff.Get(), mTxtH[0], mTxtW[0]);
  BasicTextMeasure(mTxtOn.Get(), mTxtH[1], mTxtW[1]);
  if (fitToText)
  {
    auto h = mTxtH[0];
    if (mTxtH[1] > h)
      h = mTxtH[1];
    auto w = mTxtW[0];
    if (mTxtW[1] > w)
      w = mTxtW[1];

    mRECT = mTargetRECT = GetRectToFitTextIn(mRECT, (float) mText.mSize, w, h, pad);
  }
  SetDirty(false);
}

IRECT IVButtonControl::GetRectToFitTextIn(IRECT r, float fontSize, float widthInSymbols, float numLines, float padding)
{
  IRECT textR = r;
  // first center empty rect in the middle of mRECT
  textR.L = textR.R = r.L + 0.5f * r.W();
  textR.T = textR.B = r.T + 0.5f * r.H();

  // then expand to fit text
  widthInSymbols *= 0.5f * 0.44f * fontSize; // 0.44 is approx average character w/h ratio
  numLines *= 0.5f * fontSize;
  textR.L -= widthInSymbols;
  textR.R += widthInSymbols;
  textR.T -= numLines;
  textR.B += numLines;

  if (!mEmboss && mDrawShadows)
  {
    textR.R += mShadowOffset;
    textR.B += mShadowOffset;
  }

  if (padding < 0.0) padding *= -1.0;
  textR = textR.GetPadded(padding);

  return textR;
}

void IVButtonControl::SetDrawShadows(bool draw, bool keepButtonRect)
{
  if (draw == mDrawShadows) return;

  if (keepButtonRect && !mEmboss)
  {
    auto d = mShadowOffset;
    if (!draw) d *= -1.0;
    mRECT.R += d;
    mRECT.B += d;
    mTargetRECT = mRECT;
  }

  mDrawShadows = draw;
  SetDirty(false);
}

void IVButtonControl::SetEmboss(bool emboss, bool keepButtonRect)
{
  if (emboss == mEmboss) return;

  if (keepButtonRect && mDrawShadows)
  {
    auto d = mShadowOffset;
    if (emboss) d *= -1.0;
    mRECT.R += d;
    mRECT.B += d;
    mTargetRECT = mRECT;
  }

  mEmboss = emboss;
  SetDirty(false);
}

void IVButtonControl::SetShadowOffset(float offset, bool keepButtonRect)
{
  if (offset == mShadowOffset) return;

  auto oldOff = mShadowOffset;

  if (offset < 0.0)
    mShadowOffset = 0.0;
  else
    mShadowOffset = offset;

  if (keepButtonRect && mDrawShadows && !mEmboss)
  {
    auto d = offset - oldOff;
    mRECT.R += d;
    mRECT.B += d;
    mTargetRECT = mRECT;
  }

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

//IBSliderControl::IBSliderControl(IDelegate& dlg, float x, float y, int len, int paramIdx, IBitmap& bitmap, EDirection dir, bool onlyHandle)
//: ISliderControlBase(dlg, IRECT(x, y, x + bitmap.W(), y + len), paramIdx, dir)
//, mHandleBitmap(bitmap)
//, mOnlyHandle(onlyHandle)
//{
//}
//
//void IBSliderControl::Draw(IGraphics& g)
//{
//  IRECT r = GetHandleRECT();
//  g.DrawBitmap(mHandleBitmap, r, 1, &mBlend);
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
//  SetDirty();
//}

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


