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

void IVButtonControl::Draw(IGraphics& graphics)
{
  auto btnRect = GetButtonRect();
  auto shadowColor = IColor(60, 0, 0, 0);

  if (mValue > 0.5)
  {
    graphics.FillRect(GetColor(bPR), btnRect);

    if (mDrawShadows && mEmboss)
      DrawInnerShadowForRect(btnRect, shadowColor, graphics);

    if (mTxtOn.GetLength())
    {
      auto textR = GetRectToAlignTextIn(btnRect, 1);
      graphics.DrawTextA(mText, mTxtOn.Get(), textR);
    }
  }
  else
  {
    if (mDrawShadows && !mEmboss)
      DrawOuterShadowForRect(btnRect, shadowColor, graphics);

    graphics.FillRect(GetColor(bBG), btnRect);

    if (mTxtOff.GetLength())
    {
      auto textR = GetRectToAlignTextIn(btnRect, 0);
      graphics.DrawTextA(mText, mTxtOff.Get(), textR);
    }
  }

  if(mDrawBorders)
    graphics.DrawRect(GetColor(bFR), btnRect);
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

void IVButtonControl::DrawInnerShadowForRect(IRECT r, IColor shadowColor, IGraphics& graphics)
{
  auto& o = mShadowOffset;
  auto slr = r;
  slr.R = slr.L + o;
  auto str = r;
  str.L += o;
  str.B = str.T + o;
  graphics.FillRect(shadowColor, slr);
  graphics.FillRect(shadowColor, str);
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


IVDropDownList::IVDropDownList(IDelegate& dlg, IRECT rect, int param)
  : IControl(dlg, rect, param),
    IVectorBase(&DEFAULT_BG_COLOR, &DEFAULT_TXT_COLOR, &DEFAULT_FR_COLOR, &DEFAULT_HL_COLOR)
{
  mInitRect = rect;
  mText.mFGColor = DEFAULT_TXT_COLOR;
  FillNamesFromParamDisplayTexts();
}

IVDropDownList::IVDropDownList(IDelegate& dlg, IRECT rect, int param,
                               int numStates, const char* names...)
  : IControl(dlg, rect, param),
    IVectorBase(&DEFAULT_BG_COLOR, &DEFAULT_TXT_COLOR, &DEFAULT_FR_COLOR, &DEFAULT_HL_COLOR)
{
  mInitRect = rect;
  mText.mFGColor = DEFAULT_TXT_COLOR;
  if (numStates)
  {
    va_list args;
    va_start(args, names);
    SetNames(numStates, names, args);
    va_end (args);
  }
  else
    FillNamesFromParamDisplayTexts();
};

const IColor IVDropDownList::DEFAULT_BG_COLOR = IColor(255, 200, 200, 200);
const IColor IVDropDownList::DEFAULT_FR_COLOR = IColor(255, 70, 70, 70);
const IColor IVDropDownList::DEFAULT_TXT_COLOR = DEFAULT_FR_COLOR;
const IColor IVDropDownList::DEFAULT_HL_COLOR = IColor(255, 240, 240, 240);

void IVDropDownList::Draw(IGraphics& graphics)
{
  auto initR = GetInitRect();
  auto shadowColor = IColor(60, 0, 0, 0);

  auto textR = GetRectToAlignTextIn(initR);

  if (!mExpanded)
  {
    if (mDrawShadows && !mEmboss)
      DrawOuterShadowForRect(initR, shadowColor, graphics);

    if (mBlink)
    {
      mBlink = false;
      graphics.FillRect(GetColor(lHL), initR);
      SetDirty(false);
    }
    else
      graphics.FillRect(GetColor(lBG), initR);

    if (mDrawBorders)
      graphics.DrawRect(GetColor(lFR), initR);
    graphics.DrawTextA(mText, NameForVal(StateFromNormalized()), textR);
    ShrinkRects(); // shrink here to clean the expanded area
  }

  else
  {
    auto panelR = GetExpandedRect();
    if (mDrawShadows && !mEmboss)
      DrawOuterShadowForRect(panelR, shadowColor, graphics);
    graphics.FillRect(GetColor(lBG), panelR);
    if (mDrawShadows && mEmboss)
      DrawInnerShadowForRect(panelR, shadowColor, graphics);
    int sx = -1;
    int sy = 0;
    auto rw = initR.W();
    auto rh = initR.H();
    // now just shift the rects and draw them
    for (int v = 0; v < NumStates(); ++v)
    {
      if (v % mColHeight == 0.0)
      {
        ++sx;
        sy = 0;
      }
      IRECT vR = ShiftRectBy(initR, sx * rw, sy * rh);
      IRECT tR = ShiftRectBy(textR, sx * rw, sy * rh);
      if (v == mState)
      {
        if (mDrawShadows) // draw when emboss too, looks good
          DrawOuterShadowForRect(vR, shadowColor, graphics);
        graphics.FillRect(GetColor(lHL), vR);
      }

      if (mDrawBorders)
        graphics.DrawRect(GetColor(lFR), vR);
      graphics.DrawTextA(mText, NameForVal(v), tR);
      ++sy;
    }

    if (mDrawBorders)
    {
      if (!mDrawShadows)   // panelRect == mRECT
      {
        --panelR.R; // fix for strange graphics behavior
        --panelR.B; // mRECT right and bottom are not drawn in expanded state (on Win)
      }
      graphics.DrawRect(GetColor(lFR), panelR);
    }
  }

#ifdef _DEBUG
  //graphics.DrawRect(COLOR_ORANGE, mInitRect);
  //graphics.DrawRect(COLOR_BLUE, mRECT);
  //graphics.DrawRect(COLOR_GREEN, mTargetRECT); // if padded will not be drawn correctly
#endif

}

IRECT IVDropDownList::GetInitRect()
{
  auto ir = mInitRect;
  if (mDrawShadows && !mEmboss)
  {
    ir.R -= mShadowOffset;
    ir.B -= mShadowOffset;
  }
  if (mExpanded)
    ir = ShiftRectBy(ir, mRECT.L - ir.L, mRECT.T - ir.T); // if mRECT didn't fit and was shifted.
  // will be different for some other expand directions
  return ir;
}

IRECT IVDropDownList::GetExpandedRect()
{
  auto er = mRECT;
  if (mDrawShadows && !mEmboss)
  {
    er.R -= mShadowOffset;
    er.B -= mShadowOffset;
  }
  return er;
}

IRECT IVDropDownList::GetRectToAlignTextIn(IRECT r)
{
  // this rect is not precise, it serves as a horizontal level
  auto tr = r;
  // assume all items are 1 line high
  tr.T += 0.5f * (tr.H() - mText.mSize) - 1.0f; // -1 looks better with small text
  tr.B = tr.T + 0.1f;
  return tr;
}

void IVDropDownList::DrawInnerShadowForRect(IRECT r, IColor shadowColor, IGraphics& graphics)
{
  auto& o = mShadowOffset;
  auto slr = r;
  slr.R = slr.L + o;
  auto str = r;
  str.L += o;
  str.B = str.T + o;
  graphics.FillRect(shadowColor, slr);
  graphics.FillRect(shadowColor, str);
}

void IVDropDownList::SetDrawShadows(bool draw, bool keepButtonRect)
{
  if (draw == mDrawShadows) return;

  if (keepButtonRect && !mEmboss)
  {
    auto d = mShadowOffset;
    if (!draw) d *= -1.0;
    mInitRect.R += d;
    mInitRect.B += d;
    UpdateRectsOnInitChange();
  }

  mDrawShadows = draw;
  SetDirty(false);
}

void IVDropDownList::SetEmboss(bool emboss, bool keepButtonRect)
{
  if (emboss == mEmboss) return;

  if (keepButtonRect && mDrawShadows)
  {
    auto d = mShadowOffset;
    if (emboss) d *= -1.0;
    mInitRect.R += d;
    mInitRect.B += d;
    UpdateRectsOnInitChange();
  }

  mEmboss = emboss;
  SetDirty(false);
}

void IVDropDownList::SetShadowOffset(float offset, bool keepButtonRect)
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
    mInitRect.R += d;
    mInitRect.B += d;
    UpdateRectsOnInitChange();
  }

  SetDirty(false);
}

void IVDropDownList::UpdateRectsOnInitChange()
{
  if (!mExpanded)
    ShrinkRects();
  else
    ExpandRects();
}

void IVDropDownList::OnResize()
{
  mInitRect = mRECT;
  mExpanded = false;
  mLastX = mLastY = -1.0;
  mBlink = false;
  SetDirty(false);
}

void IVDropDownList::OnMouseOver(float x, float y, const IMouseMod& mod)
{
  if (mLastX != x || mLastY != y)
  {
    mLastX = x;
    mLastY = y;
    auto panelR = GetExpandedRect();
    if (mExpanded && panelR.Contains(x, y))
    {
      auto rx = x - panelR.L;
      auto ry = y - panelR.T;

      auto initR = GetInitRect();
      int ix = (int)(rx / initR.W());
      int iy = (int)(ry / initR.H());

      int i = ix * mColHeight + iy;

      if (i >= NumStates())
        i = NumStates() - 1;
      if (i != mState)
      {
        mState = i;
        //DbgMsg("mState ", mState);
        SetDirty(false);
      }
    }
  }
}

void IVDropDownList::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  if (!mExpanded)
    ExpandRects();
  else
  {
    mExpanded = false;
    mValue = NormalizedFromState();
    SetDirty();
  }
  //DbgMsg("mValue ", mValue);
}

void IVDropDownList::OnMouseWheel(float x, float y, const IMouseMod& mod, float d)
{
  int ns = mState;
  ns += (int)d;
  ns = BOUNDED(ns, 0, NumStates() - 1);
  if (ns != mState)
  {
    mState = ns;
    mValue = NormalizedFromState();
    //DbgMsg("mState ", mState);
    //DbgMsg("mValue ", mValue);
    if (!mExpanded)
      mBlink = true;
    SetDirty();
  }
}

void IVDropDownList::OnMouseDblClick(float x, float y, const IMouseMod& mod)
{
  mValue = mDefaultValue;
  int ns = StateFromNormalized();
  if (mState != ns)
  {
    mState = ns;
    mValue = NormalizedFromState();
    //DbgMsg("mState ", mState);
    //DbgMsg("mValue ", mValue);
    if (!mExpanded)
      mBlink = true;
    SetDirty();
  }
  mExpanded = false;
}

void IVDropDownList::OnMouseOut()
{
  mState = StateFromNormalized();
  mExpanded = false;
  mLastX = mLastY = -1.0;
  SetDirty(false);
  //DbgMsg("mState ", mState);
  //DbgMsg("mValue ", mValue);
}

void IVDropDownList::ExpandRects()
{
  // expand from top left of init Rect
  auto ir = GetInitRect();
  auto& l = ir.L;
  auto& t = ir.T;
  // if num states > max list height, we need more columns
  float w = (float) NumStates() / mColHeight;
  if (w < 1.0) w = 1.0;
  else w += 0.5;
  w = std::round(w);
  w *= ir.W();
  float h = (float) NumStates();
  if (mColHeight < h)
    h = (float) mColHeight;
  h *= ir.H();

  // todo add expand directions. for now only down right
  auto& mR = mRECT;
  auto& mT = mTargetRECT;
  mR = IRECT(l, t, l + w, t + h);
  if (mDrawShadows && !mEmboss)
  {
    mR.R += mShadowOffset;
    mR.B += mShadowOffset;
  }
  // we don't want expansion to collapse right around the borders, that'd be very UI unfriendly
  mT = mR.GetPadded(20.0); // todo perhaps padding should depend on display dpi
  // expansion may get over the bounds. if so, shift it
  auto br = mDelegate.GetGUI()->GetBounds();
  auto ex = mR.R - br.R;
  if (ex > 0.0)
  {
    mR = ShiftRectBy(mR, -ex);
    mT = ShiftRectBy(mT, -ex);
  }
  auto ey = mR.B - br.B;
  if (ey > 0.0)
  {
    mR = ShiftRectBy(mR, 0.0, -ey);
    mT = ShiftRectBy(mT, 0.0, -ey);
  }

  mExpanded = true;
  SetDirty(false);
}

void IVDropDownList::SetNames(int numStates, const char* names, va_list args)
{
  if (numStates < 1) return;
  mValNames.Add(new WDL_String(names));
  for (int i = 1; i < numStates; ++i)
    mValNames.Add(new WDL_String(va_arg(args, const char*)));
}

void IVDropDownList::SetNames(int numStates, const char* names...)
{
  mValNames.Empty(true);

  va_list args;
  va_start(args, names);
  SetNames(numStates, names, args);
  va_end(args);

  SetDirty(false);
}

void IVDropDownList::FillNamesFromParamDisplayTexts()
{
  mValNames.Empty(true);
  auto param = GetParam();
  if (param)
  {
    int n = param->NDisplayTexts();
    if (n > 0)
      for (int i = 0; i < n; ++i)
        mValNames.Add(new WDL_String(param->GetDisplayTextAtIdx(i)));
    else
      mValNames.Add(new WDL_String("no display texts"));
  }
  else
    mValNames.Add(new WDL_String("no param"));

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

IBSliderControl::IBSliderControl(IDelegate& dlg, float x, float y, int len, int paramIdx, IBitmap& bitmap, EDirection direction, bool onlyHandle)
: IControl(dlg, IRECT(), paramIdx)
, mLen(len), mHandleBitmap(bitmap), mDirection(direction), mOnlyHandle(onlyHandle)
{
  if (direction == kVertical)
  {
    mHandleHeadroom = mHandleBitmap.H();
    mRECT = mTargetRECT = IRECT(x, y, x + mHandleBitmap.W(), y + len);
  }
  else
  {
    mHandleHeadroom = mHandleBitmap.W();
    mRECT = mTargetRECT = IRECT(x, y, x + len, y + mHandleBitmap.H());
  }
}

IRECT IBSliderControl::GetHandleRECT(double value) const
{
  if (value < 0.0)
  {
    value = mValue;
  }
  IRECT r(mRECT.L, mRECT.T, mRECT.L + mHandleBitmap.W(), mRECT.T + mHandleBitmap.H());
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
    mValue += 0.001 * d;
#else
  if (mod.C || mod.S)
    mValue += 0.001 * d;
#endif
  else
    mValue += 0.01 * d;

  SetDirty();
}

void IBSliderControl::SnapToMouse(float x, float y)
{
  if (mDirection == kVertical)
    mValue = 1.0 - (double) (y - mRECT.T - mHandleHeadroom / 2) / (double) (mLen - mHandleHeadroom);
  else
    mValue = (double) (x - mRECT.L - mHandleHeadroom / 2) / (double) (mLen - mHandleHeadroom);

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
  mHandleBitmap = GetUI()->GetScaledBitmap(mHandleBitmap);
}

