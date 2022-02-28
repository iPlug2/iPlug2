/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @ingroup Controls
 * @copydoc IVKeyboardControl
 */

#include "IControl.h"
#include "IPlugMidi.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

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

/** Vectorial keyboard control
 * @ingroup IControls */
class IVKeyboardControl : public IControl
{
public:
  static const IColor DEFAULT_BK_COLOR;
  static const IColor DEFAULT_WK_COLOR;
  static const IColor DEFAULT_PK_COLOR;
  static const IColor DEFAULT_FR_COLOR;
  static const IColor DEFAULT_HK_COLOR;

  IVKeyboardControl(const IRECT& bounds, int minNote = 48, int maxNote = 72, bool roundedKeys = false,
                    const IColor& WK_COLOR = DEFAULT_WK_COLOR,
                    const IColor& BK_COLOR = DEFAULT_BK_COLOR,
                    const IColor& PK_COLOR = DEFAULT_PK_COLOR,
                    const IColor& FR_COLOR = DEFAULT_FR_COLOR,
                    const IColor& HK_COLOR = DEFAULT_HK_COLOR)
  : IControl(bounds, kNoParameter)
  , mWK_COLOR(WK_COLOR)
  , mBK_COLOR(BK_COLOR)
  , mPK_COLOR(PK_COLOR)
  , mFR_COLOR(FR_COLOR)
  , mHK_COLOR(HK_COLOR)
  , mRoundedKeys(roundedKeys)
  {
    mText.mFGColor = FR_COLOR;
    mDblAsSingleClick = true;
    bool keepWidth = !(bounds.W() <= 0.0);
    if (bounds.W() <= 0.0)
    {
      mRECT.R = mRECT.L + mRECT.H();
      mTargetRECT = mRECT;
    }

    SetNoteRange(minNote, maxNote, keepWidth);
    SetWantsMidi(true);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    int prevKey = mLastTouchedKey;
    mLastTouchedKey = GetKeyAtPoint(x, y);

    SetKeyIsPressed(mLastTouchedKey, true);

    mMouseOverKey = mLastTouchedKey;

    if(mLastTouchedKey != prevKey)
    {
      mLastVelocity = GetVelocity(y);

      TriggerMidiMsgFromKeyPress(mLastTouchedKey, (int) (mLastVelocity * 127.f));
    }

    SetDirty(true);
  }

  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    if (mLastTouchedKey > -1)
    {
      SetKeyIsPressed(mLastTouchedKey, false);
      TriggerMidiMsgFromKeyPress(mLastTouchedKey, 0);

      mLastTouchedKey = -1;
      mMouseOverKey = -1;
      mLastVelocity = 0.;

      SetDirty(false);
    }
  }

  void OnMouseOut() override
  {
    if (mLastTouchedKey > -1 || mShowNoteAndVel)
    {
      mLastTouchedKey = -1;
      mMouseOverKey = -1;
      mLastVelocity = 0.;
      SetDirty(false);
    }
  }

  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
  {
    int prevKey = mLastTouchedKey;
    mLastTouchedKey = GetKeyAtPoint(x, y);

    SetKeyIsPressed(mLastTouchedKey, true);

    mMouseOverKey = mLastTouchedKey;

    if(mLastTouchedKey != prevKey)
    {
      mLastVelocity = GetVelocity(y);

      TriggerMidiMsgFromKeyPress(mLastTouchedKey, (int) (mLastVelocity * 127.f));

      TriggerMidiMsgFromKeyPress(prevKey, 0);
      SetKeyIsPressed(prevKey, false);
    }

    SetDirty(true);

  }

  void OnMouseOver(float x, float y, const IMouseMod& mod) override
  {
    if (mShowNoteAndVel)
    {
      mMouseOverKey = GetKeyAtPoint(x, y);
      SetDirty(false);
    }
  }
    
  void OnTouchCancelled(float x, float y, const IMouseMod& mod) override
  {
    if (mLastTouchedKey > -1)
    {
      SetKeyIsPressed(mLastTouchedKey, false);
      TriggerMidiMsgFromKeyPress(mLastTouchedKey, 0);

      mLastTouchedKey = -1;
      mMouseOverKey = -1;
      mLastVelocity = 0.;

      SetDirty(false);
    }
  }

  void OnResize() override
  {
    float r = mRECT.W() / mTargetRECT.W();
    float dx = mRECT.L - mTargetRECT.L;
    mWKWidth *= r;
    for (int i = 0; i < NKeys(); ++i)
    {
      float* pKeyL = GetKeyXPos(i);
      float d = *pKeyL - mRECT.L;
      *pKeyL = mRECT.L + d * r + dx;
    }

    mTargetRECT = mRECT;
    RecreateKeyBounds(true);
    SetDirty(false);
  }

  void OnMidi(const IMidiMsg& msg) override
  {
    switch (msg.StatusMsg())
    {
      case IMidiMsg::kNoteOn:
        SetNoteFromMidi(msg.NoteNumber(), (msg.Velocity() != 0));
        break;
      case IMidiMsg::kNoteOff:
        SetNoteFromMidi(msg.NoteNumber(), false);
        break;
      case IMidiMsg::kControlChange:
        if(msg.ControlChangeIdx() == IMidiMsg::kAllNotesOff)
          ClearNotesFromMidi();
        break;
      default: break;
    }

    SetDirty(false);
  }

  void DrawKey(IGraphics& g, const IRECT& bounds, const IColor& color)
  {
    if(mRoundedKeys)
    {
      g.FillRoundRect(color, bounds, 0., 0., mRoundness, mRoundness/*, &blend*/);
    }
    else
      g.FillRect(color, bounds/*, &blend*/);
  }

  void Draw(IGraphics& g) override
  {
    IColor shadowColor = IColor(60, 0, 0, 0);

    float BKBottom = mRECT.T + mRECT.H() * mBKHeightRatio;
    float BKWidth = GetBKWidth();

    // first draw white keys
    for (int i = 0; i < NKeys(); ++i)
    {
      if (!IsBlackKey(i))
      {
        float kL = *GetKeyXPos(i);
        IRECT keyBounds = IRECT(kL, mRECT.T, kL + mWKWidth, mRECT.B);

        DrawKey(g, keyBounds, i == mHighlight ? mHK_COLOR : mWK_COLOR);

        if (GetKeyIsPressed(i))
        {
          // draw played white key
          DrawKey(g, keyBounds, mPK_COLOR);

          if (mDrawShadows)
          {
            IRECT shadowBounds = keyBounds;
            shadowBounds.R = shadowBounds.L + 0.35f * shadowBounds.W();
            
            if(!mRoundedKeys)
              g.FillRect(shadowColor, shadowBounds, &mBlend);
            else {
              g.FillRoundRect(shadowColor, shadowBounds, 0., 0., mRoundness, mRoundness, &mBlend); // this one looks strange with rounded corners
            }
          }
        }
        if (mDrawFrame && i != 0)
        { // only draw the left border if it doesn't overlay mRECT left border
          g.DrawLine(mFR_COLOR, kL, mRECT.T, kL, mRECT.B, &mBlend, mFrameThickness);
          if (i == NKeys() - 2 && IsBlackKey(NKeys() - 1))
            g.DrawLine(mFR_COLOR, kL + mWKWidth, mRECT.T, kL + mWKWidth, mRECT.B, &mBlend, mFrameThickness);
        }
      }
    }

    // then blacks
    for (int i = 0; i < NKeys(); ++i)
    {
      if (IsBlackKey(i))
      {
        float kL = *GetKeyXPos(i);
        IRECT keyBounds = IRECT(kL, mRECT.T, kL + BKWidth, BKBottom);
        // first draw underlying shadows
        if (mDrawShadows && !GetKeyIsPressed(i) && i < NKeys() - 1)
        {
          IRECT shadowBounds = keyBounds;
          float w = shadowBounds.W();
          shadowBounds.L += 0.6f * w;
          if (GetKeyIsPressed(i + 1))
          {
            // if white to the right is pressed, shadow is longer
            w *= 1.3f;
            shadowBounds.B = shadowBounds.T + 1.05f * shadowBounds.H();
          }
          shadowBounds.R = shadowBounds.L + w;
          DrawKey(g, shadowBounds, shadowColor);
        }
        DrawKey(g, keyBounds, (i == mHighlight ? mHK_COLOR : mBK_COLOR.WithContrast(IsDisabled() ? GRAYED_ALPHA : 0.f)));

        if (GetKeyIsPressed(i))
        {
          // draw pressed black key
          IColor cBP = mPK_COLOR;
          cBP.A = (int) mBKAlpha;
          g.FillRect(cBP, keyBounds, &mBlend);
        }

        if(!mRoundedKeys)
        {
          // draw l, r and bottom if they don't overlay the mRECT borders
          if (mBKHeightRatio != 1.0)
            g.DrawLine(mFR_COLOR, kL, BKBottom, kL + BKWidth, BKBottom, &mBlend);
          if (i > 0)
            g.DrawLine(mFR_COLOR, kL, mRECT.T, kL, BKBottom, &mBlend);
          if (i != NKeys() - 1)
            g.DrawLine(mFR_COLOR, kL + BKWidth, mRECT.T, kL + BKWidth, BKBottom, &mBlend);
        }
      }
    }

    if (mDrawFrame)
      g.DrawRect(mFR_COLOR, mRECT, &mBlend, mFrameThickness);

    if (mShowNoteAndVel)
    {
      if (mMouseOverKey > -1)
      {
        IRECT r = IRECT(*GetKeyXPos(mMouseOverKey), mRECT.T, 0, 0);
        r.B = r.T + 1.2f * mText.mSize;
        r.R = r.L + 35.0f;
        WDL_String t;
        GetNoteNameStr(mMinNote + mMouseOverKey, false, t);
        if (mLastTouchedKey > -1)
        {
          t.AppendFormatted(16, ", vel: %3.2f", mLastVelocity * 127.f);
          r.R += 60.0;
        }
        float e = r.R - mRECT.R;
        if (e > 0.0)
        {
          r.L -= e;
          r.R -= e;
        }
        g.FillRect(mWK_COLOR, r, &mBlend);
        g.DrawRect(mFR_COLOR, r, &mBlend);
        g.DrawText(mText, t.Get(), r, &mBlend);
      }
    }

#ifdef _DEBUG
    //g.DrawRect(COLOR_GREEN, mTargetRECT);
    //g.DrawRect(COLOR_BLUE, mRECT);
    WDL_String ti;
    ti.SetFormatted(32, "key: %d, vel: %3.2f", mLastTouchedKey, mLastVelocity * 127.f);
    //ti.SetFormatted(16, "mBAlpha: %d", mBAlpha);
    IText txt(20, COLOR_RED);
    IRECT tr(mRECT.L + 20, mRECT.B - 20, mRECT.L + 160, mRECT.B);
    g.DrawText(txt, ti.Get(), tr, &mBlend);
#endif
  }

#pragma mark -

  void SetNoteRange(int min, int max, bool keepWidth = true)
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

    mPressedKeys.Resize(NKeys());
    memset(mPressedKeys.Get(), 0, mPressedKeys.GetSize() * sizeof(bool));

    RecreateKeyBounds(keepWidth);
  }

  void SetNoteFromMidi(int noteNum, bool played)
  {
    if (noteNum < mMinNote || noteNum > mMaxNote) return;
    SetKeyIsPressed(noteNum - mMinNote, played);
  }

  void SetKeyIsPressed(int key, bool pressed)
  {
    mPressedKeys.Get()[key] = pressed;
    SetDirty(false);
  }
  
  void SetKeyHighlight(int key)
  {
    mHighlight = key;
    SetDirty(false);
  }

  void ClearNotesFromMidi()
  {
    memset(mPressedKeys.Get(), 0, mPressedKeys.GetSize() * sizeof(bool));
    SetDirty(false);
  }

  void SetBlackToWhiteRatios(float widthRatio, float heightRatio = 0.6)
  {
    widthRatio = Clip(widthRatio, 0.1f, 1.f);
    heightRatio = Clip(heightRatio, 0.1f, 1.f);

    float halfW = 0.5f * mWKWidth * mBKWidthRatio;
    float r = widthRatio / mBKWidthRatio;
    mBKWidthRatio = widthRatio;
    mBKHeightRatio = heightRatio;

    for (int i = 0; i < NKeys(); ++i)
    {
      if (IsBlackKey(i))
      {
        float* pKeyL = GetKeyXPos(i);
        float mid = *pKeyL + halfW;
        *pKeyL = mid - halfW * r;
        if (*pKeyL < mRECT.L)
          *pKeyL = mRECT.L;
      }
    }

    SetDirty(false);
  }

  void SetHeight(float h, bool keepAspectRatio = false)
  {
    if (h <= 0.0) return;
    float r = h / mRECT.H();
    mRECT.B = mRECT.T + mRECT.H() * r;

    mTargetRECT = mRECT;

    if (keepAspectRatio)
      SetWidth(mRECT.W() * r);
    SetDirty(false);
  }

  void SetWidth(float w, bool keepAspectRatio = false)
  {
    if (w <= 0.0) return;
    float r = w / mRECT.W();
    mRECT.R = mRECT.L + mRECT.W() * r;
    mWKWidth *= r;
    for (int i = 0; i < NKeys(); ++i)
    {
      float* pKeyL = GetKeyXPos(i);
      float d = *pKeyL - mRECT.L;
      *pKeyL = mRECT.L + d * r;
    }

    mTargetRECT = mRECT;

    if (keepAspectRatio)
      SetHeight(mRECT.H() * r);

    SetDirty(false);
  }

  void SetShowNotesAndVelocity(bool show)
  {
    mShowNoteAndVel = show;
  }

  void SetColors(const IColor BKColor, const IColor& WKColor, const IColor& PKColor = DEFAULT_PK_COLOR, const IColor& FRColor = DEFAULT_FR_COLOR)
  {
    mBK_COLOR = BKColor;
    mWK_COLOR = WKColor;
    mPK_COLOR = PKColor;
    mFR_COLOR = FRColor;

    mBKAlpha = (float) PKColor.A;

    if (mBKAlpha < 240.f)
    {
      const float lumWK = WKColor.GetLuminosity() * WKColor.A / 255.f;
      const float adjustment = PKColor.A / 255.f;
      const float lumPK = PKColor.GetLuminosity() * adjustment;
      const float lumRes = (1.f - adjustment) * lumWK + lumPK;
      const float lumDW = lumRes - lumWK;
      const float lumBK = BKColor.GetLuminosity() * BKColor.A / 255.f;

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

  // returns pressed Midi note number
  int GetMidiNoteNumberForKey(int key) const
  {
    if (key > -1) return mMinNote + key;
    else return -1;
  }

//  double GetVelocity() const { return mVelocity * 127.f; }

private:
  void RecreateKeyBounds(bool keepWidth)
  {
    if (keepWidth)
      mWKWidth = 0.f;

    // create size-independent data.
    mIsBlackKeyList.Resize(NKeys());
    mKeyXPos.Resize(NKeys());

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

    auto GetShiftForPitchClass = [this](int pitch) {
      // usually black key width + distance to the closest black key = white key width,
      // and often b width is ~0.6 * w width
      if (pitch == 0) return 0.f;
      else if (pitch % 12 == 1)  return 7.f / 12.f;
      else if (pitch % 12 == 3)  return 5.f / 12.f;
      else if (pitch % 12 == 6)  return 2.f / 3.f;
      else if (pitch % 12 == 8)  return 0.5f;
      else if (pitch % 12 == 10) return 1.f / 3.f;
      else return 0.f;
    };

    WKPadStart = GetShiftForPitchClass(mMinNote);

    if (mMinNote != mMaxNote && IsBlackKey(mIsBlackKeyList.GetSize() - 1))
      WKPadEnd = 1.f - GetShiftForPitchClass(mMaxNote);

    // build rects
    if (mWKWidth == 0.f)
      mWKWidth = 0.2f * mRECT.H(); // first call from the constructor

    if (keepWidth)
    {
      mWKWidth = mRECT.W();
      if (numWhites)
        mWKWidth /= (numWhites + mBKWidthRatio * (WKPadStart + WKPadEnd));
    }

    float BKWidth = mWKWidth;

    if (numWhites)
      BKWidth *= mBKWidthRatio;

    float prevWKLeft = mRECT.L;

    for (int k = 0; k < mIsBlackKeyList.GetSize(); ++k)
    {
      if (IsBlackKey(k))
      {
        float l = prevWKLeft;
        if (k != 0)
        {
          l -= GetShiftForPitchClass(mMinNote + k) * BKWidth;
        }
        else prevWKLeft += WKPadStart * BKWidth;
        mKeyXPos.Get()[k] = l;
      }
      else
      {
        mKeyXPos.Get()[k] = prevWKLeft;
        prevWKLeft += mWKWidth;
      }
    }

    mTargetRECT = mRECT;
    SetDirty(false);
  }

  int GetKeyAtPoint(float x, float y)
  {
    IRECT clipRect = mRECT.GetPadded(-2);
    clipRect.Constrain(x, y);

    float BKBottom = mRECT.T + mRECT.H() * mBKHeightRatio;
    float BKWidth = GetBKWidth();

    // black keys are on top
    int k = -1;
    for (int i = 0; i < NKeys(); ++i)
    {
      if (IsBlackKey(i))
      {
        float kL = *GetKeyXPos(i);
        IRECT keyBounds = IRECT(kL, mRECT.T, kL + BKWidth, BKBottom);
        if (keyBounds.Contains(x, y))
        {
          k = i;
          break;
        }
      }
    }

    if (k == -1)
    {
      for (int i = 0; i < NKeys(); ++i)
      {
        if (!IsBlackKey(i))
        {
          float kL = *GetKeyXPos(i);
          IRECT keyBounds = IRECT(kL, mRECT.T, kL + mWKWidth, mRECT.B);
          if (keyBounds.Contains(x, y))
          {
            k = i;
            break;
          }
        }
      }
    }

    return k;
  }

  float GetVelocity(float yPos)
  {
    float velocity = 0.;

    if (mLastTouchedKey > -1)
    {
      float h = mRECT.H();

      if (IsBlackKey(mLastTouchedKey))
        h *= mBKHeightRatio;

      float fracPos = (yPos - mRECT.T) / (0.95f * h); // 0.95 is to get max velocity around the bottom

      velocity = Clip(fracPos, 1.f / 127.f, 1.f);
    }

    return velocity;
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

  float* GetKeyXPos(int i) { return mKeyXPos.Get() + i; }

  bool GetKeyIsPressed(int i) const { return *(mPressedKeys.Get() + i); }

  int NKeys() const { return mMaxNote - mMinNote + 1; }

  float GetBKWidth() const
  {
    float w = mWKWidth;
    if (NKeys() > 1)
      w *= mBKWidthRatio;
    return w;
  }

  void TriggerMidiMsgFromKeyPress(int key, int velocity)
  {
    IMidiMsg msg;

    const int nn = GetMidiNoteNumberForKey(key);

    if(velocity > 0)
      msg.MakeNoteOnMsg(nn, velocity, 0);
    else
      msg.MakeNoteOffMsg(nn, 0);

    GetDelegate()->SendMidiMsgFromUI(msg);
  }

protected:
  IColor mWK_COLOR;
  IColor mBK_COLOR;
  IColor mPK_COLOR;
  IColor mFR_COLOR;
  IColor mHK_COLOR;

  bool mRoundedKeys = false;
  float mRoundness = 5.f;
  bool mDrawShadows = false;
  bool mDrawFrame = true;
  float mFrameThickness = 1.f;
  bool mShowNoteAndVel = false;
  float mWKWidth = 0.f;
  float mBKWidthRatio = 0.6f;
  float mBKHeightRatio = 0.6f;
  float mBKAlpha = 100.f;
  int mLastTouchedKey = -1;
  float mLastVelocity = 0.f;
  int mMouseOverKey = -1;
  int mMinNote, mMaxNote;
  WDL_TypedBuf<bool> mIsBlackKeyList;
  WDL_TypedBuf<bool> mPressedKeys;
  WDL_TypedBuf<float> mKeyXPos;
  int mHighlight = -1;
};

/** Vectorial "wheel" control for pitchbender/modwheel
* @ingroup IControls */
class IWheelControl : public ISliderControlBase
{
  static constexpr int kSpringAnimationTime = 50;
  static constexpr int kNumRungs = 10;
public:
  static constexpr int kMessageTagSetPitchBendRange = 0;

  /** Create a WheelControl
   * @param bounds The control's bounds
   * @param cc A Midi CC to link, defaults to kNoCC which is interpreted as pitch bend */
  IWheelControl(const IRECT& bounds, IMidiMsg::EControlChangeMsg cc = IMidiMsg::EControlChangeMsg::kNoCC, int initBendRange = 12)
  : ISliderControlBase(bounds, kNoParameter, EDirection::Vertical, DEFAULT_GEARING, 40.f)
  , mPitchBendRange(initBendRange)
  , mCC(cc)
  {
    mMenu.AddItem("1 semitone");
    mMenu.AddItem("2 semitones");
    mMenu.AddItem("Fifth");
    mMenu.AddItem("Octave");

    SetValue(cc == IMidiMsg::EControlChangeMsg::kNoCC ? 0.5 : 0.);
    SetWantsMidi(true);
    SetActionFunction([cc](IControl* pControl){
      IMidiMsg msg;
      if(cc == IMidiMsg::EControlChangeMsg::kNoCC) // pitchbend
        msg.MakePitchWheelMsg((pControl->GetValue() * 2.) - 1.);
      else
        msg.MakeControlChangeMsg(cc, pControl->GetValue());
      
      pControl->GetDelegate()->SendMidiMsgFromUI(msg);
    });
  }
  
  void Draw(IGraphics& g) override
  {
    IRECT handleBounds = mRECT.GetPadded(-10.f);
    const float stepSize = handleBounds.H() / (float) kNumRungs;
    g.FillRoundRect(DEFAULT_SHCOLOR, mRECT.GetPadded(-5.f));
    
    if(!g.CheckLayer(mLayer))
    {
      const IRECT layerRect = handleBounds.GetMidVPadded(handleBounds.H() + stepSize);
      
      if(layerRect.W() > 0 && layerRect.H() > 0)
      {
        g.StartLayer(this, layerRect);
        g.DrawGrid(COLOR_BLACK.WithOpacity(0.5f), layerRect, 0.f, stepSize, nullptr, 2.f);
        mLayer = g.EndLayer();
      }
    }
    
    // NanoVG only has 2 stop gradients
    IRECT r = handleBounds.FracRectVertical(0.5, true);
    g.PathRect(r);
    g.PathFill(IPattern::CreateLinearGradient(r, EDirection::Vertical, {{COLOR_BLACK, 0.f},{COLOR_MID_GRAY, 1.f}}));
    r = handleBounds.FracRectVertical(0.51f, false); // slight overlap
    g.PathRect(r);
    g.PathFill(IPattern::CreateLinearGradient(r, EDirection::Vertical, {{COLOR_MID_GRAY, 0.f},{COLOR_BLACK, 1.f}}));

    const float value = static_cast<float>(GetValue());
    const float y = (handleBounds.H() - (stepSize)) * value;
    const float triangleRamp = std::fabs(value-0.5f) * 2.f;
    
    g.DrawBitmap(mLayer->GetBitmap(), handleBounds, 0, (int) y);
  
    const IRECT cutoutBounds = handleBounds.GetFromBottom(stepSize).GetTranslated(0, -y);
    g.PathRect(cutoutBounds);
    g.PathFill(IPattern::CreateLinearGradient(cutoutBounds, EDirection::Vertical,
    {
      //TODO: this can be improved
      {COLOR_BLACK.WithContrast(iplug::Lerp(0.f, 0.5f, triangleRamp)), 0.f},
      {COLOR_BLACK.WithContrast(iplug::Lerp(0.5f, 0.f, triangleRamp)), 1.f}
    }));
    
    g.DrawVerticalLine(COLOR_BLACK, cutoutBounds, 0.f);
    g.DrawVerticalLine(COLOR_BLACK, cutoutBounds, 1.f);
    g.DrawRect(COLOR_BLACK, handleBounds);
  }
  
  void OnMidi(const IMidiMsg& msg) override
  {
    if(mCC == IMidiMsg::EControlChangeMsg::kNoCC)
    {
      if(msg.StatusMsg() == IMidiMsg::kPitchWheel)
      {
        SetValue((msg.PitchWheel() + 1.) * 0.5);
        SetDirty(false);
      }
    }
    else
    {
      if(msg.ControlChangeIdx() == mCC)
      {
        SetValue(msg.ControlChange(mCC));
        SetDirty(false);
      }
    }
  }
  
  void OnMouseWheel(float x, float y, const IMouseMod &mod, float d) override
  {
    /* NO-OP */
  }

  void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int) override
  {
    if(pSelectedMenu) 
    {
      switch (pSelectedMenu->GetChosenItemIdx()) 
      {
        case 0: mPitchBendRange = 1; break;
        case 1: mPitchBendRange = 2; break;
        case 2: mPitchBendRange = 7; break;
        case 3:
        default:
          mPitchBendRange = 12; break;
      }
      
      GetDelegate()->SendArbitraryMsgFromUI(kMessageTagSetPitchBendRange, GetTag(), sizeof(int), &mPitchBendRange);
    }
  }
  
  void OnMouseDown(float x, float y, const IMouseMod &mod) override
  {
    if(mod.R && mCC == IMidiMsg::EControlChangeMsg::kNoCC)
    {
      switch (mPitchBendRange) 
      {
        case 1: mMenu.CheckItemAlone(0); break;
        case 2: mMenu.CheckItemAlone(1); break;
        case 7: mMenu.CheckItemAlone(2); break;
        case 12: mMenu.CheckItemAlone(3); break;
        default:
          break;
      }
      
      GetUI()->CreatePopupMenu(*this, mMenu, x, y);
    }
    else
      ISliderControlBase::OnMouseDown(x, y, mod);
  }
  
  void OnMouseUp(float x, float y, const IMouseMod &mod) override
  {
    if(mCC == IMidiMsg::EControlChangeMsg::kNoCC) // pitchbend
    {
      double startValue = GetValue();
      SetAnimation([startValue](IControl* pCaller) {
        pCaller->SetValue(iplug::Lerp(startValue, 0.5, Clip(pCaller->GetAnimationProgress(), 0., 1.)));
        if(pCaller->GetAnimationProgress() > 1.) {
          pCaller->SetDirty(true);
          pCaller->OnEndAnimation();
          return;
        }
      }, kSpringAnimationTime);
    }
    
    ISliderControlBase::OnMouseUp(x, y, mod);
  }
  
private:
  IPopupMenu mMenu;
  int mPitchBendRange;
  IMidiMsg::EControlChangeMsg mCC;
  ILayerPtr mLayer;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
