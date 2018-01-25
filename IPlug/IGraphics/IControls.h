#pragma once

/**
 * @file
 * @brief A collection of IControls for common UI widgets, such as knobs, sliders, switches
 */

#include "IControl.h"

/**
 * \defgroup Controls IGraphics::IControls
 * @{
 */

#pragma mark - Base Controls

/** Parent for knobs, to handle mouse action and ballistics. */
class IKnobControlBase : public IControl
{
public:
  IKnobControlBase(IPlugBaseGraphics& plug, IRECT rect, int param = kNoParameter,
                   EDirection direction = kVertical, double gearing = DEFAULT_GEARING)
  : IControl(plug, rect, param)
  , mDirection(direction)
  , mGearing(gearing)
  {}
  
  virtual ~IKnobControlBase() {}
  
  void SetGearing(double gearing) { mGearing = gearing; }
  virtual void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override;
  virtual void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override;
  
protected:
  EDirection mDirection;
  double mGearing;
};

/** Parent for buttons/switch controls */
class IButtonControlBase : public IControl
{
public:
  IButtonControlBase(IPlugBaseGraphics& plug, IRECT rect, int param = kNoParameter,  IActionFunction actionFunc = nullptr,
                     uint32_t numStates = 2);
  
  virtual ~IButtonControlBase() {}
  
  virtual void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  
protected:
  uint32_t mNumStates;
};

#pragma mark - Vector Controls

/** A vector switch control. Click to cycle through states. */
class IVSwitchControl : public IButtonControlBase
{
public:
  IVSwitchControl(IPlugBaseGraphics& plug, IRECT rect, int param = kNoParameter, IActionFunction actionFunc = nullptr,
                  const IColor& fgColor = COLOR_BLACK, const IColor& bgColor = COLOR_WHITE,
                  uint32_t numStates = 2, EDirection dir = kVertical);
  
  ~IVSwitchControl() {}
  
  void Draw(IGraphics& graphics)  override;
  
private:
  float mStep;
  IColor mFGColor;
  IColor mBGColor;
  EDirection mDirection;
};

/** A vector knob control */
class IVKnobControl : public IKnobControlBase
{
public:
  IVKnobControl(IPlugBaseGraphics& plug, IRECT rect, int param, const IColor& fgcolor = DEFAULT_FGCOLOR, const IColor& bgcolor = DEFAULT_BGCOLOR, float rMin = 0.f, float rMax = 1.f, float aMin = -135.f, float aMax = 135.f, EDirection direction = kVertical, double gearing = DEFAULT_GEARING);
  ~IVKnobControl() {}
  
  void Draw(IGraphics& graphics) override;
  
protected:
  IColor mFGColor, mBGColor;
  float mAngleMin, mAngleMax, mInnerRadius, mOuterRadius;
};

#pragma mark - Bitmap Controls

/** A vector switch control. Click to cycle through states. */
class IBSwitchControl : public IBitmapControl
{
public:
  IBSwitchControl(IPlugBaseGraphics& plug, int x, int y, int param, IBitmap& bitmap)
  : IBitmapControl(plug, x, y, param, bitmap) {}
  ~IBSwitchControl() {}
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override {  OnMouseDown(x, y, mod); }
  
private:
};

/** A slider with a bitmap for the handle. The bitmap snaps to a mouse click or drag. */
class IBSliderControl : public IControl
{
public:
  IBSliderControl(IPlugBaseGraphics& plug, float x, float y, int len, int param,
                  IBitmap& bitmap, EDirection direction = kVertical, bool onlyHandle = false);
  ~IBSliderControl() {}
  
  virtual void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  virtual void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override { return SnapToMouse(x, y); }
  virtual void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override;
  virtual void Draw(IGraphics& graphics) override;
  virtual bool IsHit(float x, float y) const override;
  virtual void OnRescale() override;
  
  int GetLength() const { return mLen; }
  int GetHandleHeadroom() const { return mHandleHeadroom; }
  double GetHandleValueHeadroom() const { return (double) mHandleHeadroom / (double) mLen; }
  IRECT GetHandleRECT(double value = -1.0) const;
protected:
  virtual void SnapToMouse(float x, float y);
  int mLen, mHandleHeadroom;
  IBitmap mHandleBitmap;
  EDirection mDirection;
  bool mOnlyHandle;
};


/** Display monospace bitmap font text */
// TODO: fix Centre/Right aligned behaviour when string exceeds bounds or should wrap onto new line
class IBTextControl : public ITextControl
{
public:
  IBTextControl(IPlugBaseGraphics& plug, IRECT rect, IBitmap& bitmap, const IText& text = DEFAULT_TEXT, const char* str = "", int charWidth = 6, int charHeight = 12, int charOffset = 0, bool multiLine = false, bool vCenter = true, EBlendType bl = kBlendNone)
  : ITextControl(plug, rect, text, str)
  , mCharWidth(charWidth)
  , mCharHeight(charHeight)
  , mCharOffset(charOffset)
  , mMultiLine(multiLine)
  , mVCentre(vCenter)
  , mTextBitmap(bitmap)
  {
    mStr.Set(str);
  }

  void Draw(IGraphics& graphics) override
  {
    graphics.DrawBitmapedText(mTextBitmap, mRECT, mText, &mBlend, mStr.Get(), mVCentre, mMultiLine, mCharWidth, mCharHeight, mCharOffset);
  }
  
protected:
  WDL_String mStr;
  int mCharWidth, mCharHeight, mCharOffset;
  bool mMultiLine;
  bool mVCentre;
  IBitmap mTextBitmap;
};

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
{
public:
  IVKeyboardControl(IPlugBaseGraphics& plug, IRECT rect, int minNote, int maxNote)
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
  
//  IVKeyboardControl(IPlugBaseGraphics& pPlug, float x, float y, int minNote, int maxNote)
//  : IVKeyboardControl(pPlug, x, y, -1.0, 70.0, minNote, maxNote) {}
//
  ~IVKeyboardControl()
  {
    mKeyRects.Empty(true);
    mNoteIsPlayed.Empty(true);
    mKeyIsBlack.Empty(true);
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
    
    SetDirty();
  }
  
  void OnMouseUp(float x, float y, const IMouseMod& mod) override
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
  
  void OnMouseOut() override
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
      mVelocity = BOUNDED(mVelocity, 1.f / 127.f, 1.f);
#ifdef _DEBUG
      SetDirty();
#else
      if (mShowNoteAndVel)
        SetDirty();
#endif
    }
  }
  
  void OnMouseOver(float x, float y, const IMouseMod& pMod) override
  {
    if (mShowNoteAndVel)
    {
      mMouseOverKey = GetKeyUnderMouse(x, y);
      SetDirty();
    }
  }
  
  void OnResize() override
  {
    mTargetRECT = mRECT;
    SetWidth(mRECT.W());
    SetHeight(mRECT.H());
    RecreateRects(true);
  }
  
  void Draw(IGraphics& graphics) override
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
          cBP.A = (int) mBAlpha;
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
    
    mNoteIsPlayed.Empty(true);
    
    for (int n = mMinNote; n <= mMaxNote; ++n) // todo here use a call to host
      mNoteIsPlayed.Add(new bool(false));    // to keep visible state actual
      
    RecreateRects(keepWidth);
  }
  
  void SetNoteIsPlayed(int noteNum, bool played)
  {
    if (noteNum < mMinNote || noteNum > mMaxNote) return;
    mNoteIsPlayed.Set(noteNum - mMinNote, &played);
    SetDirty();
  }
  
  void SetBlackToWhiteWidthAndHeightRatios(float widthR, float heightR = 0.6)
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
  
  void SetHeight(float h, bool keepProportions = false)
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
  
  void SetWidth(float w, bool keepProportions = false)
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
  
  void SetShowNotesAndVelocity(bool show)
  {
    mShowNoteAndVel = show;
  }
  
  void SetColors(IColor bkColor, IColor wkColor, IColor pkColor = IColor(60, 0, 0, 0), IColor frColor = IColor(255, 80, 80, 80))
  {
    mBKColor = bkColor;
    mWKColor = wkColor;
    mPKColor = pkColor;
    mFRColor = frColor;
    
    auto Luminocity = [] (IColor c)
    {
      auto min = c.R < c.G ? (c.R < c.B ? c.R : c.B) : (c.G < c.B ? c.G : c.B);
      auto max = c.R > c.G ? (c.R > c.B ? c.R : c.B) : (c.G > c.B ? c.G : c.B);
      return (min + max) / 2;
    };
    
    mBAlpha = (float)pkColor.A;
    
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
  
  void SetDrawShadows(bool draw)
  {
    mDrawShadows = draw;
  }
  
  void SetDrawBorder(bool draw)
  {
    mDrawBorder = draw;
    SetDirty();
  }
  
private:  
  void RecreateRects(bool keepWidth)
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
    
    auto ShiftForKey = [this] (int note)
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
      if (n % 12 == 1 || n % 12 == 3 || n % 12 == 6 || n % 12 == 8|| n % 12 == 10)
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
  
  int GetKeyUnderMouse(float x, float y)
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
  
  void UpdateVelocity(float y)
  {
    if (mKey > -1)
    {
      auto kr = mKeyRects.Get(mKey);
      mVelocity = (float) (y - kr->T) / (0.95f * kr->H());
      // 0.95 is to get max velocity around the bottom
      mVelocity = BOUNDED(mVelocity, 1.f / 127.f, 1.f);
    }
    else mVelocity = 0.f;
  }
  
  void GetNoteNameStr(int midiNoteNum, bool addOctave, WDL_String& str)
  {
    int oct = midiNoteNum / 12;
    midiNoteNum -= 12 * oct;
    const char* notes[12] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    const char* n = notes[midiNoteNum];
    str.Set(n);
    if (addOctave)
      str.AppendFormatted(2, "%d", --oct);
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
  
  double GetVelocity() const
  {
    return mVelocity * 127.f;
  }
  
  double GetVelocityNormalized() const
  {
    return mVelocity;
  }
  
  int GetVelocityInt() const
  {
    return (int)(mVelocity * 127. + 0.5);
  }
  
protected:
  bool mShowNoteAndVel = false;
  bool mDrawShadows = true;
  bool mDrawBorder = true;
  IColor mBKColor = IColor(255, 70, 70, 70);
  IColor mWKColor = IColor(255, 240, 240, 240);
  IColor mPKColor = IColor(60, 0, 0, 0); // pressed key color
  IColor mFRColor = mBKColor; // frame color

  float mBAlpha = 100.f; // needed cause not any mPKColor will have nice contrast on black keys ?
  float mBKWidthR = 0.6f;
  float mBKHeightR = 0.6f;
  int mKey = -1;
  int mMouseOverKey = -1;
  float mVelocity = 0.f;
  bool mVelByWheel = false;

  int mMinNote, mMaxNote;
  WDL_PtrList<bool> mKeyIsBlack;
  WDL_PtrList<bool> mNoteIsPlayed;
  WDL_PtrList<IRECT> mKeyRects;
};

/**@}*/

