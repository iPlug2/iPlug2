#include "IControls.h"

#pragma mark - VECTOR CONTROLS

IVSwitchControl::IVSwitchControl(IDelegate& dlg, IRECT rect, int paramIdx, std::function<void(IControl*)> actionFunc
  , const IVColorSpec& colorSpec, uint32_t numStates, EDirection dir)
  : ISwitchControlBase(dlg, rect, paramIdx, actionFunc, numStates)
  , IVectorBase(colorSpec)
  , mDirection(dir)
{
  mStep = 1.f / float(mNumStates) - 1.f;
}

void IVSwitchControl::Draw(IGraphics& graphics)
{
  const int state = (int)std::round(mValue / mStep);

  graphics.FillRect(GetColor(EVColor::kBG), mRECT, &mBlend);

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
 // graphics.FillRect(GetColor(EVColor::kFG), handle.GetPadded(-10), &mBlend);
  graphics.FillCircle(GetColor(EVColor::kFG), handle.MW(), handle.MH(), handle.W()/2., &mBlend);

  //graphics.DrawRect(GetColor(EVColor::kFR), mRECT.GetPadded(-5), &mBlend);
  graphics.FillCircle(GetColor(EVColor::kFR), handle.MW(), handle.MH(), (handle.W()/2.)-2, &mBlend);
}

IVKnobControl::IVKnobControl(IDelegate& dlg, IRECT rect, int param,
                             const IVColorSpec& colorSpec,
                             float rMin, float rMax, float aMin, float aMax,
                             EDirection direction, double gearing)
: IKnobControlBase(dlg, rect, param, direction, gearing)
, IVectorBase(colorSpec)
, mAngleMin(aMin)
, mAngleMax(aMax)
, mInnerRadius(rMin)
, mOuterRadius(rMax)
{
  if (mOuterRadius == 0.0f)
    mOuterRadius = 0.5f * (float) rect.W();
}

void IVKnobControl::Draw(IGraphics& graphics)
{
  const float v = mAngleMin + ((float) mValue * (mAngleMax - mAngleMin));
  const float cx = mRECT.MW(), cy = mRECT.MH();
  const float radius = (mRECT.W()/2.f) - 2.f;
  graphics.FillCircle(GetColor(EVColor::kFR), cx, cy, radius+2, GetMouseIsOver() ? &BLEND_25 : &BLEND_10);
  graphics.DrawCircle(GetColor(EVColor::kBG), cx, cy, radius, &BLEND_50);
  graphics.FillArc(GetColor(EVColor::kBG), cx, cy, radius, mAngleMin, v, &BLEND_50);
  graphics.DrawRadialLine(GetColor(EVColor::kFG), cx, cy, v, mInnerRadius * radius, mOuterRadius * radius);
}

void IVSliderControl::Draw(IGraphics& graphics)
{
  graphics.FillRect(GetColor(kBG), mRECT);
  
  const float top = mTrack.B - (mValue * mTrack.H());
  IRECT innerRect = IRECT(mTrack.L, top, mTrack.R, mRECT.B);
  graphics.FillRect(GetColor(kFG), innerRect);
}

void IVSliderControl::OnResize()
{
  mTrack = mRECT.GetPadded(-10);
  SetDirty();
}

IVKeyboardControl::IVKeyboardControl(IDelegate& dlg, IRECT rect,
                                     int minNote, int maxNote)
: IControl(dlg, rect)
, IVectorBase(&DEFAULT_WK_COLOR, &DEFAULT_BK_COLOR, &DEFAULT_FR_COLOR, &DEFAULT_PK_COLOR)
{
  mText.mFGColor = GetColor(kFR);
  mDblAsSingleClick = true;
  bool keepWidth = !(rect.W() <= 0.0);
  if (rect.W() <= 0.0)
  {
    mRECT.R = mRECT.L + mRECT.H();
    mTargetRECT = mRECT;
  }
  
  SetMinMaxNote(minNote, maxNote, keepWidth);
}

const IColor IVKeyboardControl::DEFAULT_BK_COLOR = IColor(255, 70, 70, 70);
const IColor IVKeyboardControl::DEFAULT_WK_COLOR = IColor(255, 240, 240, 240);
const IColor IVKeyboardControl::DEFAULT_PK_COLOR = IColor(60, 0, 0, 0);
const IColor IVKeyboardControl::DEFAULT_FR_COLOR = DEFAULT_BK_COLOR;

void IVKeyboardControl::OnMouseDown(float x, float y, const IMouseMod & mod)
{
  int oldK = mKey;
  mKey = GetKeyUnderMouse(x, y);
  
  if (oldK != mKey)
    mVelByWheel = false;
  
  mMouseOverKey = mKey;
  
  if (!mVelByWheel)
    UpdateVelocity(y);
  
  SetDirty();
}

void IVKeyboardControl::OnMouseUp(float x, float y, const IMouseMod & mod)
{
  if (mKey > -1)
  {
    mKey = -1;
    mMouseOverKey = -1;
    mVelocity = 0.0;
    mVelByWheel = false;
    SetDirty();
  }
}

void IVKeyboardControl::OnMouseOut()
{
  if (mKey > -1 || mShowNoteAndVel)
  {
    mKey = -1;
    mMouseOverKey = -1;
    mVelocity = 0.0;
    mVelByWheel = false;
    SetDirty();
  }
}

void IVKeyboardControl::OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod & mod)
{
  OnMouseDown(x, y, mod);
}

void IVKeyboardControl::OnMouseWheel(float x, float y, const IMouseMod & mod, float d)
{
  if (mKey > -1)
  {
    if (mod.C || mod.S) mVelocity += 0.003f * d;
    else mVelocity += 0.03f * d;
    mVelByWheel = true;
    mVelocity = BOUNDED(mVelocity, 1.f / 127.f, 1.f);
#ifdef _DEBUG
    SetDirty();
#else
    if (mShowNoteAndVel)
      SetDirty();
#endif
  }
}

void IVKeyboardControl::OnMouseOver(float x, float y, const IMouseMod & pMod)
{
  if (mShowNoteAndVel)
  {
    mMouseOverKey = GetKeyUnderMouse(x, y);
    SetDirty();
  }
}

void IVKeyboardControl::OnResize()
{
  auto r = mRECT.W() / mTargetRECT.W();
  auto dx = mRECT.L - mTargetRECT.L;
  mWKWidth *= r;
  for (int i = 0; i < NumKeys(); ++i)
  {
    auto kl = KeyLCoordPtr(i);
    auto d = *kl - mRECT.L;
    *kl = mRECT.L + d * r + dx;
  }
  
  mTargetRECT = mRECT;
  SetDirty();
}

void IVKeyboardControl::Draw(IGraphics & graphics)
{
  auto shadowColor = IColor(60, 0, 0, 0);
  graphics.FillRect(GetColor(kWK), mRECT);
  
  auto& top = mRECT.T;
  auto& wBot = mRECT.B;
  auto bBot = top + mRECT.H() * mBKHeightRatio;
  auto bKWidth = CalcBKWidth();
  
  // first draw whites
  for (int i = 0; i < NumKeys(); ++i)
  {
    if (!IsBlackKey(i))
    {
      auto kL = KeyLCoord(i);
      auto kRect = IRECT(kL, top, kL + mWKWidth, wBot);
      if (i == mKey || NoteIsPlayed(i))
      {
        // draw played white key
        graphics.FillRect(GetColor(kPK), kRect);
        if (mDrawShadows)
        {
          auto sr = kRect;
          sr.R = sr.L + 0.35f * sr.W();
          graphics.FillRect(shadowColor, sr);
        }
      }
      if (mDrawBorders && i != 0)
      { // only draw the left border if it doesn't overlay mRECT l border
        graphics.DrawLine(GetColor(kFR), kL, top, kL, wBot);
        if (i == NumKeys() - 2 && IsBlackKey(NumKeys() - 1))
          graphics.DrawLine(GetColor(kFR), kL + mWKWidth, top, kL + mWKWidth, wBot);
      }
    }
  }
  
  // then blacks
  for (int i = 0; i < NumKeys(); ++i)
  {
    if (IsBlackKey(i))
    {
      auto kL = KeyLCoord(i);
      auto kRect = IRECT(kL, top, kL + bKWidth, bBot);
      // first draw underlying shadows
      if (mDrawShadows && i != mKey && !NoteIsPlayed(i) && i < NumKeys() - 1)
      {
        auto sr = kRect;
        float w = sr.W();
        sr.L += 0.6f * w;
        if (i + 1 == mKey || NoteIsPlayed(i + 1))
        {
          // if white to the right is pressed, shadow is longer
          w *= 1.3f;
          sr.B = sr.T + 1.05f * sr.H();
        }
        sr.R = sr.L + w;
        graphics.FillRect(shadowColor, sr);
      }
      graphics.FillRect(GetColor(kBK), kRect);
      if (i == mKey || NoteIsPlayed(i))
      {
        // draw played black key
        auto cBP = GetColor(kPK);
        cBP.A = (int)mBKAlpha;
        graphics.FillRect(cBP, kRect);
      }
      if (mDrawBorders)
      { // draw l, r and bottom if they don't overlay the mRECT borders
        if (mBKHeightRatio != 1.0)
          graphics.DrawLine(GetColor(kFR), kL, bBot, kL + bKWidth, bBot);
        if (i != 0)
          graphics.DrawLine(GetColor(kFR), kL, top, kL, bBot);
        if (i != NumKeys() - 1)
          graphics.DrawLine(GetColor(kFR), kL + bKWidth, top, kL + bKWidth, bBot);
      }
    }
  }
  
  if (mDrawBorders)
    graphics.DrawRect(GetColor(kFR), mRECT);
  
  if (mShowNoteAndVel)
  {
    if (mMouseOverKey > -1)
    {
      auto r = IRECT(KeyLCoord(mMouseOverKey), top, 0, 0);
      r.B = r.T + 1.2f * mText.mSize;
      r.R = r.L + 35.0f;
      WDL_String t;
      GetNoteNameStr(mMinNote + mMouseOverKey, false, t);
      if (mKey > -1)
      {
        t.AppendFormatted(16, ", vel: %3.2f", GetVelocity());
        r.R += 60.0;
      }
      auto e = r.R - mRECT.R;
      if (e > 0.0)
      {
        r.L -= e;
        r.R -= e;
      }
      graphics.FillRect(GetColor(kWK), r);
      graphics.DrawRect(GetColor(kFR), r);
      graphics.DrawText(mText, t.Get(), r);
    }
  }
  
#ifdef _DEBUG
  //graphics.DrawRect(COLOR_GREEN, mTargetRECT);
  //graphics.DrawRect(COLOR_BLUE, mRECT);
  WDL_String ti;
  ti.SetFormatted(32, "key: %d, vel: %3.2f", mKey, GetVelocity());
  //ti.SetFormatted(32, "key: %d, vel: %d", mKey, GetVelocityInt());
  //ti.SetFormatted(16, "mBAlpha: %d", mBAlpha);
  IText txt(COLOR_RED, 20);
  auto& mr = mRECT;
  IRECT tr(mr.L + 20, mr.B - 20, mr.L + 160, mr.B);
  graphics.DrawText(txt, ti.Get(), tr);
#endif
}

void IVKeyboardControl::SetMinMaxNote(int min, int max, bool keepWidth)
{
  if (min < 0 || max < 0) return;
  if (min < max)
  {
    mMinNote = min;
    mMaxNote = max;
  }
  else
  {
    mMinNote = max;
    mMaxNote = min;
  }
  
  mNoteIsPlayed.Resize(NumKeys());
  memset(mNoteIsPlayed.Get(), 0, mNoteIsPlayed.GetSize() * sizeof(bool));
  
  //TODO: call to plugin to retain pressed keys
  
  RecreateKeyBounds(keepWidth);
}

void IVKeyboardControl::SetNoteIsPlayed(int noteNum, bool played)
{
  if (noteNum < mMinNote || noteNum > mMaxNote) return;
  mNoteIsPlayed.Get()[noteNum - mMinNote] = played;
  SetDirty();
}

void IVKeyboardControl::SetBlackToWhiteWidthAndHeightRatios(float widthR, float heightR)
{
  if (widthR <= 0.0 || heightR <= 0.0) return;
  if (widthR > 1.0) widthR = 1.0;
  if (heightR > 1.0) heightR = 1.0;
  auto halfW = 0.5f * mWKWidth * mBKWidthR;
  float r = widthR / mBKWidthR;
  mBKWidthR = widthR;
  mBKHeightRatio = heightR;
  for (int i = 0; i < NumKeys(); ++i)
  {
    if (IsBlackKey(i))
    {
      auto kl = KeyLCoordPtr(i);
      float mid = *kl + halfW;
      *kl = mid - halfW * r;
      if (*kl < mRECT.L) *kl = mRECT.L;
    }
  }
  SetDirty();
}

void IVKeyboardControl::SetHeight(float h, bool keepProportions)
{
  if (h <= 0.0) return;
  auto& mR = mRECT;
  auto r = h / mR.H();
  mR.B = mR.T + mR.H() * r;
  
  mTargetRECT = mRECT;
  
  if (keepProportions)
    SetWidth(mR.W() * r);
  SetDirty();
}

void IVKeyboardControl::SetWidth(float w, bool keepProportions)
{
  if (w <= 0.0) return;
  auto& mR = mRECT;
  auto r = w / mR.W();
  mR.R = mR.L + mR.W() * r;
  mWKWidth *= r;
  for (int i = 0; i < NumKeys(); ++i)
  {
    auto kl = KeyLCoordPtr(i);
    auto d = *kl - mR.L;
    *kl = mR.L + d * r;
  }
  
  mTargetRECT = mRECT;
  
  if (keepProportions)
    SetHeight(mR.H() * r);
  
  SetDirty();
}

void IVKeyboardControl::SetShowNotesAndVelocity(bool show)
{
  mShowNoteAndVel = show;
}

void IVKeyboardControl::SetColors(const IColor BKColor, const IColor& WKColor, const IColor& PKColor, const IColor& FRColor)
{
  SetColor(kBK, BKColor);
  SetColor(kWK, WKColor);
  SetColor(kPK, PKColor);
  SetColor(kFR, FRColor);

  mBKAlpha = (float) PKColor.A;
  
  if (mBKAlpha < 240.f)
  {
    const float lumWK = WKColor.GetLuminocity() * WKColor.A / 255.f;
    const float adjustment = PKColor.A / 255.f;
    const float lumPK = PKColor.GetLuminocity() * adjustment;
    const float lumRes = (1.f - adjustment) * lumWK + lumPK;
    const float lumDW = lumRes - lumWK;
    const float lumBK = BKColor.GetLuminocity() * BKColor.A / 255.f;
    
    if ((lumDW < 0 && lumBK < lumWK) || (lumDW > 0 && lumBK > lumWK))
    {
      float dbWB = lumWK - lumBK; // not used in the conditions ^^ for readability
      mBKAlpha += (255.f - mBKAlpha) * (1.f - dbWB * dbWB / 255.f / 255.f) + 0.5f;
    }
    else
      mBKAlpha += lumDW + 0.5f;
    
    mBKAlpha = BOUNDED(mBKAlpha, 15.f, 255.f);
  }
  
  SetDirty();
}

void IVKeyboardControl::RecreateKeyBounds(bool keepWidth)
{
  if (keepWidth)
    mWKWidth = 0.f;
  
  // create size-independent data.
  mIsBlackKeyList.Resize(NumKeys());
  mKeyLCoords.Resize(NumKeys());
  
  float numWhites = 0.f;
  for (int n = mMinNote, i = 0; n <= mMaxNote; ++n, i++)
  {
    if (n % 12 == 1 || n % 12 == 3 || n % 12 == 6 || n % 12 == 8 || n % 12 == 10)
    {
      mIsBlackKeyList.Get()[i] = true;
    }
    else
    {
      mIsBlackKeyList.Get()[i] = false;
      numWhites += 1.f;
    }
  }
  
  // black key middle isn't aligned exactly between whites
  float WKPadStart = 0.f; // 1st note may be black
  float WKPadEnd = 0.f;   // last note may be black
  
  auto ShiftForKey = [this](int note)
  {
    // usually black key width + distance to the closest black key = white key width,
    // and often b width is ~0.6 * w width
    if (note == 0) return 0.f;
    else if (note % 12 == 1)  return 7.f / 12.f;
    else if (note % 12 == 3)  return 5.f / 12.f;
    else if (note % 12 == 6)  return 2.f / 3.f;
    else if (note % 12 == 8)  return 0.5f;
    else if (note % 12 == 10) return 1.f / 3.f;
    else return 0.f;
  };
  
  WKPadStart = ShiftForKey(mMinNote);
  
  if (mMinNote != mMaxNote && IsBlackKey(mIsBlackKeyList.GetSize() - 1))
    WKPadEnd = 1.f - ShiftForKey(mMaxNote);
  
  // build rects
  if (mWKWidth == 0.f)
    mWKWidth = 0.2f * mRECT.H(); // first call from the constructor
  
  if (keepWidth)
  {
    mWKWidth = mRECT.W();
    if (numWhites) mWKWidth /= (numWhites + mBKWidthR * (WKPadStart + WKPadEnd));
  }
  float blackW = mWKWidth;
  if (numWhites) blackW *= mBKWidthR;
  
  float prevWKLeft = mRECT.L;
  
  for (int k = 0; k < mIsBlackKeyList.GetSize(); ++k)
  {
    if (IsBlackKey(k))
    {
      float l = prevWKLeft;
      if (k != 0)
      {
        auto s = ShiftForKey(mMinNote + k);
        l -= s * blackW;
      }
      else prevWKLeft += WKPadStart * blackW;
      mKeyLCoords.Get()[k] = l;
    }
    else
    {
      mKeyLCoords.Get()[k] = prevWKLeft;
      prevWKLeft += mWKWidth;
    }
  }
  
  mTargetRECT = mRECT;
  SetDirty();
}

int IVKeyboardControl::GetKeyUnderMouse(float x, float y)
{
  auto& top = mRECT.T;
  auto& WKBottom = mRECT.B;
  auto BKBottom = top + mRECT.H() * mBKHeightRatio;
  auto BKWidth = CalcBKWidth();
  
  // black keys are on top
  int k = -1;
  for (int i = 0; i < NumKeys(); ++i)
  {
    if (IsBlackKey(i))
    {
      auto kL = KeyLCoord(i);
      auto kRect = IRECT(kL, top, kL + BKWidth, BKBottom);
      if (kRect.Contains(x, y))
      {
        k = i;
        break;
      }
    }
  }
  
  if (k < 0) for (int i = 0; i < NumKeys(); ++i)
  {
    if (!IsBlackKey(i))
    {
      auto kL = KeyLCoord(i);
      auto keyBounds = IRECT(kL, top, kL + mWKWidth, WKBottom);
      if (keyBounds.Contains(x, y))
      {
        k = i;
        break;
      }
    }
  }
  
  return k;
}

void IVKeyboardControl::UpdateVelocity(float y)
{
  if (mKey > -1)
  {
    auto h = mRECT.H();
    
    if (IsBlackKey(mKey))
      h *= mBKHeightRatio;
    
    mVelocity = (float)(y - mRECT.T) / (0.95f * h);
    // 0.95 is to get max velocity around the bottom
    mVelocity = BOUNDED(mVelocity, 1.f / 127.f, 1.f);
  }
  else mVelocity = 0.f;
}

void IVKeyboardControl::GetNoteNameStr(int midiNoteNum, bool addOctave, WDL_String& str)
{
  int oct = midiNoteNum / 12;
  midiNoteNum -= 12 * oct;
  const char* notes[12] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
  const char* n = notes[midiNoteNum];
  str.Set(n);
  if (addOctave)
    str.AppendFormatted(2, "%d", --oct);
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

//IBSliderControl::IBSliderControl(IDelegate& dlg, float x, float y, int len, int paramIdx, IBitmap& bitmap, EDirection direction, bool onlyHandle)
//: IControl(dlg, IRECT(x, y, x + bitmap.W(), y + len), paramIdx)
//, mHandleBitmap(bitmap), mDirection(direction), mOnlyHandle(onlyHandle)
//{
//}
//
//IRECT IBSliderControl::GetHandleRECT(double value) const
//{
//  if (value < 0.0)
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
//  return SnapToMouse(x, y, mDirection, mTrack);
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
////void IBSliderControl::SnapToMouse(float x, float y)
////{
////  if (mDirection == kVertical)
////    mValue = 1.0 - (double) (y - mRECT.T - mHandleHeadroom / 2) / (double) (mLen - mHandleHeadroom);
////  else
////    mValue = (double) (x - mRECT.L - mHandleHeadroom / 2) / (double) (mLen - mHandleHeadroom);
////
////  SetDirty();
////}
//
//void IBSliderControl::Draw(IGraphics& graphics)
//{
//  IRECT r = GetHandleRECT();
//  graphics.DrawBitmap(mHandleBitmap, r, 1, &mBlend);
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
//  SetDirty();
//}
//
