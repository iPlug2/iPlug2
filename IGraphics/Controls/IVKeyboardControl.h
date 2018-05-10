#include "IControl.h"

/*

 IVKeyboardControl by Eugene Yakshin, 2018

 based on

 IKeyboardControl
 (c) Theo Niessink 2009, 2010
 <http://www.taletn.com/>

 This software is provided 'as-is', without any express or implied
 warranty. In no event will the authors be held liable for any damages
 arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it
 freely, subject to the following restrictions:

 1. The origin of this software must not be misrepresented; you must not
 claim that you wrote the original software. If you use this software in a
 product, an acknowledgment in the product documentation would be
 appreciated but is not required.
 2. Altered source versions must be plainly marked as such, and must not be
 misrepresented as being the original software.
 3. This notice may not be removed or altered from any source distribution.


 This keyboard is runtime customizable. Any key range is supported.
 Key proportions, colors and some other design elements can be changed at any time too.
 See the interface for details.
 */

class IVKeyboardControl : public IControl
                        , public IVectorBase
{
public:
  static const IColor DEFAULT_BK_COLOR;
  static const IColor DEFAULT_WK_COLOR;
  static const IColor DEFAULT_PK_COLOR;
  static const IColor DEFAULT_FR_COLOR;

  // map to IVectorBase colors
  enum EVKColor
  {
    kBK = kFG, // Black Keys
    kWK = kBG, // White Keys
    kPK = kHL, // Pressed Keys
    //kFR = kFR
  };

  IVKeyboardControl(IDelegate& dlg, IRECT bounds,
                    int minNote = 36, int maxNote = 60)
  : IControl(dlg, bounds, kNoParameter)
  , IVectorBase(&DEFAULT_WK_COLOR, &DEFAULT_BK_COLOR, &DEFAULT_FR_COLOR, &DEFAULT_PK_COLOR)
  {
    AttachIControl(this);

    mText.mFGColor = GetColor(kFR);
    mDblAsSingleClick = true;
    bool keepWidth = !(bounds.W() <= 0.0);
    if (bounds.W() <= 0.0)
    {
      mRECT.R = mRECT.L + mRECT.H();
      mTargetRECT = mRECT;
    }

    SetMinMaxNote(minNote, maxNote, keepWidth);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    int oldK = mKey;
    mKey = GetKeyUnderMouse(x, y);

    if (oldK != mKey)
      mVelByWheel = false;

    mMouseOverKey = mKey;

    if (!mVelByWheel)
      UpdateVelocity(y);

    SetDirty(false);
  }

  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    if (mKey > -1)
    {
      mKey = -1;
      mMouseOverKey = -1;
      mVelocity = 0.0;
      mVelByWheel = false;
      SetDirty(false);
    }
  }

  void OnMouseOut() override
  {
    if (mKey > -1 || mShowNoteAndVel)
    {
      mKey = -1;
      mMouseOverKey = -1;
      mVelocity = 0.0;
      mVelByWheel = false;
      SetDirty(false);
    }
  }

  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
  {
    OnMouseDown(x, y, mod);
  }

  void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override
  {
    if (mKey > -1)
    {
      if (mod.C || mod.S) mVelocity += 0.003f * d;
      else mVelocity += 0.03f * d;
      mVelByWheel = true;
      mVelocity = Clip(mVelocity, 1.f / 127.f, 1.f);
#ifdef _DEBUG
      SetDirty(false);
#else
      if (mShowNoteAndVel)
        SetDirty(false);
#endif
    }
  }

  void OnMouseOver(float x, float y, const IMouseMod& mod) override
  {
    if (mShowNoteAndVel)
    {
      mMouseOverKey = GetKeyUnderMouse(x, y);
      SetDirty(false);
    }
  }

  void OnResize() override
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
    SetDirty(false);
  }

  void Draw(IGraphics& g) override
  {
    auto shadowColor = IColor(60, 0, 0, 0);
    g.FillRect(GetColor(kWK), mRECT);

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
          g.FillRect(GetColor(kPK), kRect);
          if (mDrawShadows)
          {
            auto sr = kRect;
            sr.R = sr.L + 0.35f * sr.W();
            g.FillRect(shadowColor, sr);
          }
        }
        if (mDrawFrame && i != 0)
        { // only draw the left border if it doesn't overlay mRECT l border
          g.DrawLine(GetColor(kFR), kL, top, kL, wBot);
          if (i == NumKeys() - 2 && IsBlackKey(NumKeys() - 1))
            g.DrawLine(GetColor(kFR), kL + mWKWidth, top, kL + mWKWidth, wBot);
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
          g.FillRect(shadowColor, sr);
        }
        g.FillRect(GetColor(kBK), kRect);
        if (i == mKey || NoteIsPlayed(i))
        {
          // draw played black key
          auto cBP = GetColor(kPK);
          cBP.A = (int)mBKAlpha;
          g.FillRect(cBP, kRect);
        }
       // draw l, r and bottom if they don't overlay the mRECT borders
        if (mBKHeightRatio != 1.0)
          g.DrawLine(GetColor(kFR), kL, bBot, kL + bKWidth, bBot);
        if (i != 0)
          g.DrawLine(GetColor(kFR), kL, top, kL, bBot);
        if (i != NumKeys() - 1)
          g.DrawLine(GetColor(kFR), kL + bKWidth, top, kL + bKWidth, bBot);
      }
    }

    if (mDrawFrame)
      g.DrawRect(GetColor(kFR), mRECT);

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
        g.FillRect(GetColor(kWK), r);
        g.DrawRect(GetColor(kFR), r);
        g.DrawText(mText, t.Get(), r);
      }
    }

#ifdef _DEBUG
    //g.DrawRect(COLOR_GREEN, mTargetRECT);
    //g.DrawRect(COLOR_BLUE, mRECT);
    WDL_String ti;
    ti.SetFormatted(32, "key: %d, vel: %3.2f", mKey, GetVelocity());
    //ti.SetFormatted(32, "key: %d, vel: %d", mKey, GetVelocityInt());
    //ti.SetFormatted(16, "mBAlpha: %d", mBAlpha);
    IText txt(COLOR_RED, 20);
    auto& mr = mRECT;
    IRECT tr(mr.L + 20, mr.B - 20, mr.L + 160, mr.B);
    g.DrawText(txt, ti.Get(), tr);
#endif
  }

  void SetMinMaxNote(int min, int max, bool keepWidth = true)
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
  
  void SetNoteIsPlayed(int noteNum, bool played)
  {
    if (noteNum < mMinNote || noteNum > mMaxNote) return;
    mNoteIsPlayed.Get()[noteNum - mMinNote] = played;
    SetDirty(false);
  }

  void SetBlackToWhiteWidthAndHeightRatios(float widthR, float heightR = 0.6)
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
    SetDirty(false);
  }
  void SetHeight(float h, bool keepProportions = false)
  {
    if (h <= 0.0) return;
    auto& mR = mRECT;
    auto r = h / mR.H();
    mR.B = mR.T + mR.H() * r;

    mTargetRECT = mRECT;

    if (keepProportions)
      SetWidth(mR.W() * r);
    SetDirty(false);
  }
  
  void SetWidth(float w, bool keepProportions = false)
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

    SetDirty(false);
  }
  
  void SetShowNotesAndVelocity(bool show)
  {
    mShowNoteAndVel = show;
  }

  void SetColors(const IColor BKColor, const IColor& WKColor, const IColor& PKColor = DEFAULT_PK_COLOR, const IColor& FRColor = DEFAULT_FR_COLOR)
  {
    SetColor(kBK, BKColor);
    SetColor(kWK, WKColor);
    SetColor(kPK, PKColor);
    SetColor(kFR, FRColor);

    mBKAlpha = (float)PKColor.A;

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

      mBKAlpha = Clip(mBKAlpha, 15.f, 255.f);
    }

    SetDirty(false);
  }

  // returns pressed key number inside the keyboard
  int GetKey() const
  {
    return mKey;
  }
  // returns pressed MIDI note number
  int GetNote() const
  {
    if (mKey > -1) return mMinNote + mKey;
    else return -1;
  }

  double GetVelocity() const { return mVelocity * 127.f; }
  double GetVelocityNormalized() const { return mVelocity; }
  int GetVelocityInt() const { return (int)(mVelocity * 127. + 0.5); }

private:
  void RecreateKeyBounds(bool keepWidth)
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
    SetDirty(false);
  }

  int GetKeyUnderMouse(float x, float y)
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

  void UpdateVelocity(float y)
  {
    if (mKey > -1)
    {
      auto h = mRECT.H();

      if (IsBlackKey(mKey))
        h *= mBKHeightRatio;

      mVelocity = (float)(y - mRECT.T) / (0.95f * h);
      // 0.95 is to get max velocity around the bottom
      mVelocity = Clip(mVelocity, 1.f / 127.f, 1.f);
    }
    else mVelocity = 0.f;
  }
  void GetNoteNameStr(int midiNoteNum, bool addOctave, WDL_String& str)
  {
    int oct = midiNoteNum / 12;
    midiNoteNum -= 12 * oct;
    const char* notes[12] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    const char* n = notes[midiNoteNum];
    str.Set(n);
    if (addOctave)
      str.AppendFormatted(2, "%d", --oct);
  }

  bool IsBlackKey(int i) const { return *(mIsBlackKeyList.Get() + i); }

  float KeyLCoord(int i) { return *(mKeyLCoords.Get() + i); }

  float* KeyLCoordPtr(int i) { return mKeyLCoords.Get() + i; }

  bool NoteIsPlayed(int i) const { return *(mNoteIsPlayed.Get() + i); }

  int NumKeys() const { return mMaxNote - mMinNote + 1; }

  float CalcBKWidth() const
  {
    auto w = mWKWidth;
    if (NumKeys() > 1)
      w *= mBKWidthR;
    return w;
  }

protected:
  bool mShowNoteAndVel = false;
  float mWKWidth = 0.f;
  float mBKWidthR = 0.6f;
  float mBKHeightRatio = 0.6f;
  float mBKAlpha = 100.f;
  int mKey = -1;
  int mMouseOverKey = -1;
  float mVelocity = 0.f;
  bool mVelByWheel = false;
  int mMinNote, mMaxNote;
  WDL_TypedBuf<bool> mIsBlackKeyList;
  WDL_TypedBuf<bool> mNoteIsPlayed;
  WDL_TypedBuf<float> mKeyLCoords;
};

const IColor IVKeyboardControl::DEFAULT_BK_COLOR = IColor(255, 70, 70, 70);
const IColor IVKeyboardControl::DEFAULT_WK_COLOR = IColor(255, 240, 240, 240);
const IColor IVKeyboardControl::DEFAULT_PK_COLOR = IColor(60, 0, 0, 0);
const IColor IVKeyboardControl::DEFAULT_FR_COLOR = DEFAULT_BK_COLOR;
