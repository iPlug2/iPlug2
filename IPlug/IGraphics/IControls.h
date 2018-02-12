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

#pragma mark - Vector Controls

/** A vector switch control. Click to cycle through states. */
class IVSwitchControl : public ISwitchControlBase
                      , public IVectorBase
{
public:
  IVSwitchControl(IPlugBaseGraphics& plug, IRECT rect, int param = kNoParameter, IActionFunction actionFunc = nullptr,
                  const IVColorSpec& colorSpec = DEFAULT_SPEC,
                  uint32_t numStates = 2, EDirection dir = kVertical);

  ~IVSwitchControl() {}

  void Draw(IGraphics& graphics)  override;

  void OnMouseOver(float x, float y, const IMouseMod& mod) override;
  void OnMouseOut() override;

private:
  bool mMouseOver = false;
  float mStep;
  EDirection mDirection;
};

/** A vector knob control drawn using graphics primitves */
class IVKnobControl : public IKnobControlBase
                    , public IVectorBase
{
public:
  IVKnobControl(IPlugBaseGraphics& plug, IRECT rect, int param,
                const IVColorSpec& colorSpec = DEFAULT_SPEC,
                float rMin = 0.f, float rMax = 1.f, float aMin = -135.f, float aMax = 135.f,
                EDirection direction = kVertical, double gearing = DEFAULT_GEARING);
  ~IVKnobControl() {}

  void Draw(IGraphics& graphics) override;

protected:
  float mAngleMin, mAngleMax, mInnerRadius, mOuterRadius;
};

/** A vector knob control which rotates an SVG image */
class IVSVGKnob : public IKnobControlBase
{
public:
  IVSVGKnob(IPlugBaseGraphics& plug, IRECT rect, ISVG& svg, int param = kNoParameter)
    : IKnobControlBase(plug, rect, param)
    , mSVG(svg)
  {
  }

  void Draw(IGraphics& g) override
  {
#ifdef IGRAPHICS_LICE
    g.DrawText(mText, "NO LICE SVG", mRECT);
#else
    g.DrawRotatedSVG(mSVG, mRECT.MW(), mRECT.MH(), mRECT.W(), mRECT.H(), mStartAngle + mValue * (mEndAngle - mStartAngle));
#endif
  }

  void SetSVG(ISVG& svg)
  {
    mSVG = svg;
    GetGUI()->SetAllControlsDirty();
  }

private:
  ISVG mSVG;
  float mStartAngle = -135.f;
  float mEndAngle = 135.f;
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
    kBK = kFG,
    kWK = kBG,
    kPK = kHL,
    kFR = kFR
  };

  IVKeyboardControl(IPlugBaseGraphics& plug, IRECT rect,
                    int minNote = 36, int maxNote = 60);

  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseUp(float x, float y, const IMouseMod& mod) override;
  void OnMouseOut() override;
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override;
  void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override;
  void OnMouseOver(float x, float y, const IMouseMod& mod) override;
  void OnResize() override;

  void Draw(IGraphics& graphics) override;

  void SetMinMaxNote(int min, int max, bool keepWidth = true);
  void SetNoteIsPlayed(int noteNum, bool played);
  void SetBlackToWhiteWidthAndHeightRatios(float widthR, float heightR = 0.6);
  void SetHeight(float h, bool keepProportions = false);
  void SetWidth(float w, bool keepProportions = false);
  void SetShowNotesAndVelocity(bool show);
  void SetColors(const IColor bkColor, const IColor& wkColor, const IColor& pkColor = DEFAULT_PK_COLOR, const IColor& frColor = DEFAULT_FR_COLOR);

  void SetDrawShadows(bool draw) // todo make shadow offset a settable member
  {
    mDrawShadows = draw;
    SetDirty();
  }

  void SetDrawBorders(bool draw)
  {
    mDrawBorders = draw;
    SetDirty();
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
  void RecreateKeyBounds(bool keepWidth);
  int GetKeyUnderMouse(float x, float y);
  void UpdateVelocity(float y);
  void GetNoteNameStr(int midiNoteNum, bool addOctave, WDL_String& str);
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
  bool mDrawShadows = true;
  bool mDrawBorders = true;

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

class IVButtonControl : public IControl,
                        public IVectorBase
  {
  public:

  static const IColor DEFAULT_BG_COLOR;
  static const IColor DEFAULT_PR_COLOR;
  static const IColor DEFAULT_TXT_COLOR;
  static const IColor DEFAULT_FR_COLOR;

  // map to IVectorBase colors
  enum EVBColor
    {
    bTXT = kFG,
    bBG = kBG,
    bPR = kHL,
    bFR = kFR
    };

  IVButtonControl(IPlugBaseGraphics& plug, IRECT rect, int param,
                  const char *txtOn = "on", const char *txtOff = "off");
  ~IVButtonControl() {};

  void Draw(IGraphics& graphics) override;
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;

  void SetTexts(const char *txtOff, const char *txtOn, bool fitToText = false, float pad = 10.0);

  void SetDrawBorders(bool draw)
    {
    mDrawBorders = draw;
    SetDirty(false);
    }
  void SetDrawShadows(bool draw, bool keepButtonRect = true);
  void SetEmboss(bool emboss, bool keepButtonRect = true);
  void SetShadowOffset(float offset, bool keepButtonRect = true);
  void SetRect(IRECT r) {
    mRECT = mTargetRECT = r;
    SetDirty(false);
    }

  protected:
    WDL_String mTxtOff, mTxtOn;
    float mTxtH[2]; // [off, on], needed for nice multiline text drawing
    float mTxtW[2];
    // perhaps next two should be IVectorBase members too:
    bool mDrawBorders = true;
    bool mDrawShadows = true;
    bool mEmboss = false;
    float mShadowOffset = 5.0;

    IRECT GetButtonRect() {
      auto br = mRECT;
      if (mDrawShadows && !mEmboss) {
        br.R -= mShadowOffset;
        br.B -= mShadowOffset;
        }
      return br;
      }
  };

  class IVContactControl : public IVButtonControl
    {
    public:
    IVContactControl(IPlugBaseGraphics& plug, IRECT rect, int param,
                     const char *txtOn = "on", const char *txtOff = "off") :
      IVButtonControl(plug, rect, param, txtOn, txtOff) {};

    ~IVContactControl() {};

    void OnMouseUp(float x, float y, const IMouseMod& mod) override {
      mValue = 0.0;
      SetDirty();
      }
  };

#pragma mark - Bitmap Controls

/** A vector switch control. Click to cycle through states. */
class IBSwitchControl : public IBitmapControl
{
public:
  IBSwitchControl(IPlugBaseGraphics& plug, float x, float y, int param, IBitmap& bitmap)
  : IBitmapControl(plug, x, y, param, bitmap) {}
  ~IBSwitchControl() {}

  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override {  OnMouseDown(x, y, mod); }
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

/**@}*/

