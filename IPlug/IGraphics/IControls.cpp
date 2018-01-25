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
  bool keepWidth = !(rect.W() < 0.0);
  if (rect.W() < 0.0)
  {
    mRECT.R = mRECT.L + mRECT.H();
    mTargetRECT = mRECT;
  }

  SetMinMaxNote(minNote, maxNote, keepWidth);
}

IVKeyboardControl::~IVKeyboardControl()
{
  mKeyRects.Empty(true);
  mNoteIsPlayed.Empty(true);
  mKeyIsBlack.Empty(true);
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
  mTargetRECT = mRECT;
  SetWidth(mRECT.W());
  SetHeight(mRECT.H());
  RecreateRects(true);
}

void IVKeyboardControl::Draw(IGraphics & graphics)
{
  auto shadowColor = IColor(60, 0, 0, 0);
  graphics.FillRect(mWKColor, mTargetRECT);

  // first draw whites
  for (int i = 0; i < mKeyRects.GetSize(); ++i)
  {
    if (!*(mKeyIsBlack.Get(i)))
    {
      graphics.DrawRect(mFRColor, *(mKeyRects.Get(i)));
      if (i == mKey || *(mNoteIsPlayed.Get(i)))
      {
        // draw played white key
        auto r = *mKeyRects.Get(i);
        graphics.FillRect(mPKColor, r);
        if (mDrawShadows)
        {
          r.R = r.L + 0.35f * r.W();
          graphics.FillRect(shadowColor, r);
        }
      }
    }
  }

  // then blacks
  for (int i = 0; i < mKeyRects.GetSize(); ++i)
  {
    if (*(mKeyIsBlack.Get(i)))
    {
      // first draw underlying shadows
      if (mDrawShadows && i != mKey && !*(mNoteIsPlayed.Get(i))
        && i < mKeyRects.GetSize() - 1)
      {
        auto r = *mKeyRects.Get(i);
        float w = r.W();
        r.L += 0.6f * w;
        if (i + 1 == mKey || *(mNoteIsPlayed.Get(i + 1)))
        {
          // if white to the right is pressed, shadow is longer
          w *= 1.3f;
          r.B = r.T + 1.05f * r.H();
        }
        r.R = r.L + w;
        graphics.FillRect(shadowColor, r);
      }
      graphics.FillRect(mBKColor, *(mKeyRects.Get(i)));
      if (i == mKey || *(mNoteIsPlayed.Get(i)))
      {
        // draw played black key
        auto cBP = mPKColor;
        cBP.A = (int)mBAlpha;
        graphics.FillRect(cBP, *(mKeyRects.Get(i)));
      }
    }
  }

  if (mDrawBorder)
    graphics.DrawRect(mFRColor, mTargetRECT);

  if (mShowNoteAndVel)
  {
    if (mMouseOverKey > -1)
    {
      auto r = *mKeyRects.Get(mMouseOverKey);
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
  //graphics->DrawRect(&COLOR_GREEN, &mTargetRECT);
  //graphics->DrawRect(&COLOR_BLUE, &mRECT);
  //for (int i = 0; i < mKeyRects.GetSize(); ++i) graphics->DrawRect(&COLOR_ORANGE, mKeyRects.Get(i));
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

  for (int n = mMinNote; n <= mMaxNote; ++n) // todo here use a call to host
    mNoteIsPlayed.Add(new bool(false));    // to keep visible state actual

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
  mBKHeightR = heightR;
  float r = widthR / mBKWidthR;
  mBKWidthR = widthR;
  auto& tR = mTargetRECT;
  float bkBot = tR.T + tR.H() * heightR;
  for (int i = 0; i < mKeyRects.GetSize(); ++i)
  {
    if (*(mKeyIsBlack.Get(i)))
    {
      auto& kr = *(mKeyRects.Get(i));
      kr.B = bkBot;
      auto d = 0.5f * kr.W();
      float mid = 0.5f * (kr.L + kr.R);
      kr.L = mid - d * r;
      kr.R = mid + d * r;
      if (kr.L < tR.L)kr.L = tR.L;
      if (kr.R > tR.R)kr.R = tR.R;
    }
  }
  SetDirty();
}

void IVKeyboardControl::SetHeight(float h, bool keepProportions)
{
  if (h < 0) return;
  auto& tR = mTargetRECT;
  auto r = h / tR.H();
  tR.B = tR.T + tR.H() * r;
  for (int i = 0; i < mKeyRects.GetSize(); ++i)
  {
    auto& kr = *(mKeyRects.Get(i));
    kr.B = kr.T + kr.H() * r;
  }

  if (keepProportions)
    SetWidth(tR.W() * r);
  SetDirty();
}

void IVKeyboardControl::SetWidth(float w, bool keepProportions)
{
  if (w < 0) return;
  auto& tR = mTargetRECT;
  auto r = w / tR.W();
  tR.R = tR.L + tR.W() * r;
  for (int i = 0; i < mKeyRects.GetSize(); ++i)
  {
    auto& kr = *(mKeyRects.Get(i));
    auto kw = kr.W();
    auto d = kr.L - tR.L;
    kr.L = tR.L + d * r;
    kr.R = kr.L + kw * r;
  }

  if (keepProportions)
    SetHeight(tR.H() * r);
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
  // save key width if needed
  float whiteW = 0.0;
  if (!keepWidth)
  {
    if (mKeyRects.GetSize())
    {
      whiteW = mKeyRects.Get(0)->W();
      if (*(mKeyIsBlack.Get(0))) whiteW /= mBKWidthR;
    }
    else whiteW = 0.2f * mTargetRECT.H();
  }
  mKeyRects.Empty(true);

  // create size-independent data.
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
  if (mMinNote != mMaxNote) wPadEnd = 1.f - ShiftForKey(mMaxNote);

  mKeyIsBlack.Empty(true);
  float numWhites = 0.f;
  for (int n = mMinNote; n <= mMaxNote; ++n)
  {
    if (n % 12 == 1 || n % 12 == 3 || n % 12 == 6 || n % 12 == 8 || n % 12 == 10)
    {
      mKeyIsBlack.Add(new bool(true));
    }
    else
    {
      mKeyIsBlack.Add(new bool(false));
      numWhites += 1.0;
    }
  }


  // build rects
  auto& top = mTargetRECT.T;
  auto& whiteB = mTargetRECT.B;
  float blackB = top + mTargetRECT.H() * mBKHeightR;

  if (whiteW == 0) whiteW = 0.2f * mTargetRECT.H(); // first call from the constructor
  if (keepWidth)
  {
    whiteW = mTargetRECT.W();
    if (numWhites) whiteW /= (numWhites + wPadStart + wPadEnd);
  }
  float blackW = whiteW;
  if (numWhites) blackW *= mBKWidthR;

  float prevWhiteRectR = mTargetRECT.L;

  for (int k = 0; k < mKeyIsBlack.GetSize(); ++k)
  {
    if (*(mKeyIsBlack.Get(k)))
    {
      float l = prevWhiteRectR;
      if (k != 0)
      {
        auto s = ShiftForKey(mMinNote + k);
        l -= s * blackW;
      }
      else prevWhiteRectR += wPadStart * blackW;
      mKeyRects.Add(new IRECT(l, top, l + blackW, blackB));
    }
    else
    {
      mKeyRects.Add(new IRECT(prevWhiteRectR, top, prevWhiteRectR + whiteW, whiteB));
      prevWhiteRectR += whiteW;
    }
  }

  mTargetRECT.R = (mKeyRects.Get(mKeyRects.GetSize() - 1))->R;

  SetDirty();
}

int IVKeyboardControl::GetKeyUnderMouse(float x, float y)
{
  // black keys are on top
  int k = -1;
  for (int i = 0; i < mKeyRects.GetSize(); ++i)
  {
    if (*(mKeyIsBlack.Get(i)) && mKeyRects.Get(i)->Contains(x, y))
    {
      k = i;
      break;
    }
  }
  if (k < 0) for (int i = 0; i < mKeyRects.GetSize(); ++i)
  {
    if (!*(mKeyIsBlack.Get(i)) && mKeyRects.Get(i)->Contains(x, y))
    {
      k = i;
      break;
    }
  }

  return k;
}

void IVKeyboardControl::UpdateVelocity(float y)
{
  if (mKey > -1)
  {
    auto kr = mKeyRects.Get(mKey);
    mVelocity = (float)(y - kr->T) / (0.95f * kr->H());
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
