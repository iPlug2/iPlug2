#include "IControls.h"


IVSwitchControl::IVSwitchControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx, std::function<void(IControl*)> actionFunc
  , const IColor& fgColor, const IColor &bgColor, uint32_t numStates, EDirection dir)
  :IButtonControlBase(plug, rect, paramIdx, actionFunc, numStates)
  , mFGColor(fgColor)
  , mBGColor(bgColor)
  , mDirection(dir)
{
  mStep = 1.f / float(mNumStates) - 1.f;
}

void IVSwitchControl::Draw(IGraphics& graphics)
{
  const int state = (int)std::round(mValue / mStep);

  graphics.FillRect(mBGColor, mRECT, &mBlend);

  IRECT handle;

  if (mNumStates > 2)
  {
    if (mDirection == kHorizontal)
      handle = mRECT.SubRectHorizontal(mNumStates, state);
    if (mDirection == kVertical)
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
    mValue += 1.0 / (double)(mBitmap.N - 1);
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
  const float v = mAngleMin + ((float) mValue * (mAngleMax - mAngleMin));
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


//  IVKeyboardControl(IPlugBaseGraphics& pPlug, float x, float y, int minNote, int maxNote)
//  : IVKeyboardControl(pPlug, x, y, -1.0, 70.0, minNote, maxNote) {}
//

IVKeyboardControl::IVKeyboardControl(IPlugBaseGraphics & plug, IRECT rect, int minNote, int maxNote)
  : IControl(plug, rect)
{
  mText.mColor = mFRColor;
  mDblAsSingleClick = true;
  bool keepWidth = !(rect.W() <= 0.0);
  if (rect.W() <= 0.0)
  {
    mRECT.R = mRECT.L + mRECT.H();
    mTargetRECT = mRECT;
  }

  SetMinMaxNote(minNote, maxNote, keepWidth);
}

IVKeyboardControl::~IVKeyboardControl()
{
  mKeyLCoords.Empty(true);
  mNoteIsPlayed.Empty(true);
  mIsBlackKeyList.Empty(true);
}

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
  graphics.FillRect(mWKColor, mRECT);

  auto& top = mRECT.T;
  auto& wBot = mRECT.B;
  auto bBot = top + mRECT.H() * mBKHeightR;
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
        graphics.FillRect(mPKColor, kRect);
        if (mDrawShadows)
        {
          auto sr = kRect;
          sr.R = sr.L + 0.35f * sr.W();
          graphics.FillRect(shadowColor, sr);
        }
      }
      if (mDrawBorders && i != 0)
      { // only draw the left border if it doesn't overlay mRECT l border
        graphics.DrawLine(mFRColor, kL, top, kL, wBot);
        if (i == NumKeys() - 2 && IsBlackKey(NumKeys() - 1))
          graphics.DrawLine(mFRColor, kL + mWKWidth, top, kL + mWKWidth, wBot);
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
      graphics.FillRect(mBKColor, kRect);
      if (i == mKey || NoteIsPlayed(i))
      {
        // draw played black key
        auto cBP = mPKColor;
        cBP.A = (int)mBAlpha;
        graphics.FillRect(cBP, kRect);
      }
      if (mDrawBorders)
      { // draw l, r and bottom if they don't overlay the mRECT borders
        if (mBKHeightR != 1.0)
          graphics.DrawLine(mFRColor, kL, bBot, kL + bKWidth, bBot);
        if (i != 0)
          graphics.DrawLine(mFRColor, kL, top, kL, bBot);
        if (i != NumKeys() - 1)
          graphics.DrawLine(mFRColor, kL + bKWidth, top, kL + bKWidth, bBot);
      }
    }
  }

  if (mDrawBorders)
    graphics.DrawRect(mFRColor, mRECT);

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
      graphics.FillRect(mWKColor, r);
      graphics.DrawRect(mFRColor, r);
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
  IText txt(20, COLOR_RED);
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

  mNoteIsPlayed.Empty(true);

  for (int n = mMinNote; n <= mMaxNote; ++n) // todo here use a call to plug
    mNoteIsPlayed.Add(new bool(false));      // to keep visible state actual

  RecreateRects(keepWidth);
}

void IVKeyboardControl::SetNoteIsPlayed(int noteNum, bool played)
{
  if (noteNum < mMinNote || noteNum > mMaxNote) return;
  mNoteIsPlayed.Set(noteNum - mMinNote, &played);
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
  mBKHeightR = heightR;
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

void IVKeyboardControl::SetColors(const IColor bkColor, const IColor& wkColor, const IColor& pkColor, const IColor& frColor)
{
  mBKColor = bkColor;
  mWKColor = wkColor;
  mPKColor = pkColor;
  mFRColor = frColor;

  auto Luminocity = [](IColor c)
  {
    auto min = c.R < c.G ? (c.R < c.B ? c.R : c.B) : (c.G < c.B ? c.G : c.B);
    auto max = c.R > c.G ? (c.R > c.B ? c.R : c.B) : (c.G > c.B ? c.G : c.B);
    return (min + max) / 2;
  };

  mBAlpha = (float) pkColor.A;

  if (mBAlpha < 240)
  {
    float lumW = Luminocity(wkColor) * wkColor.A / 255.f;
    float transp = pkColor.A / 255.f;
    float lumP = Luminocity(pkColor) * transp;
    float lumRes = (1.f - transp) * lumW + lumP;
    float dWLum = lumRes - lumW;

    float lumB = Luminocity(bkColor) * bkColor.A / 255.f;

    if ((dWLum < 0 && lumB < lumW) || (dWLum > 0 && lumB > lumW))
    {
      float dbWB = lumW - lumB; // not used in the conditions ^^ for readability
      mBAlpha += (255.f - mBAlpha) * (1.f - dbWB * dbWB / 255.f / 255.f) + 0.5f;
    }
    else
      mBAlpha += dWLum + 0.5f;

    mBAlpha = BOUNDED(mBAlpha, 15.f, 255.f);
  }

  SetDirty();
}

void IVKeyboardControl::RecreateRects(bool keepWidth)
{
  if (keepWidth) mWKWidth = 0.f;

  mKeyLCoords.Empty(true);

  // create size-independent data.
  mIsBlackKeyList.Empty(true);
  float numWhites = 0.f;
  for (int n = mMinNote; n <= mMaxNote; ++n)
  {
    if (n % 12 == 1 || n % 12 == 3 || n % 12 == 6 || n % 12 == 8 || n % 12 == 10)
    {
      mIsBlackKeyList.Add(new bool(true));
    }
    else
    {
      mIsBlackKeyList.Add(new bool(false));
      numWhites += 1.0;
    }
  }

  // black key middle isn't aligned exactly between whites
  float wPadStart = 0.0; // 1st note may be black
  float wPadEnd = 0.0;   // last note may be black

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

  wPadStart = ShiftForKey(mMinNote);
  if (mMinNote != mMaxNote && IsBlackKey(mIsBlackKeyList.GetSize() - 1))
    wPadEnd = 1.f - ShiftForKey(mMaxNote);

  // build rects
  auto& top = mRECT.T;
//  auto& whiteB = mRECT.B;
//  float blackB = top + mRECT.H() * mBKHeightR;

  if (mWKWidth == 0) mWKWidth = 0.2f * mRECT.H(); // first call from the constructor
  if (keepWidth)
  {
    mWKWidth = mRECT.W();
    if (numWhites) mWKWidth /= (numWhites + mBKWidthR * (wPadStart + wPadEnd));
  }
  float blackW = mWKWidth;
  if (numWhites) blackW *= mBKWidthR;

  float prevWLCoord = mRECT.L;

  for (int k = 0; k < mIsBlackKeyList.GetSize(); ++k)
  {
    if (IsBlackKey(k))
    {
      float l = prevWLCoord;
      if (k != 0)
      {
        auto s = ShiftForKey(mMinNote + k);
        l -= s * blackW;
      }
      else prevWLCoord += wPadStart * blackW;
      mKeyLCoords.Add(new float(l));
    }
    else
    {
      mKeyLCoords.Add(new float(prevWLCoord));
      prevWLCoord += mWKWidth;
    }
  }

  mTargetRECT = mRECT;
  SetDirty();
}

int IVKeyboardControl::GetKeyUnderMouse(float x, float y)
{
  auto& top = mRECT.T;
  auto& wBot = mRECT.B;
  auto bBot = top + mRECT.H() * mBKHeightR;
  auto bKWidth = CalcBKWidth();

  // black keys are on top
  int k = -1;
  for (int i = 0; i < NumKeys(); ++i)
  {
    if (IsBlackKey(i))
    {
      auto kL = KeyLCoord(i);
      auto kRect = IRECT(kL, top, kL + bKWidth, bBot);
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
      auto kRect = IRECT(kL, top, kL + mWKWidth, wBot);
      if (kRect.Contains(x, y))
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
      h *= mBKHeightR;
    
    mVelocity = (float)(y - mRECT.T) / (0.95f * h);
    // 0.95 is to get max velocity around the bottom
    mVelocity = BOUNDED(mVelocity, 1.f / 127.f, 1.f);
  }
  else mVelocity = 0.f;
}

void IVKeyboardControl::GetNoteNameStr(int midiNoteNum, bool addOctave, WDL_String & str)
{
  int oct = midiNoteNum / 12;
  midiNoteNum -= 12 * oct;
  const char* notes[12] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
  const char* n = notes[midiNoteNum];
  str.Set(n);
  if (addOctave)
    str.AppendFormatted(2, "%d", --oct);
}
