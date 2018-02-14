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
  IVSwitchControl(IDelegate& dlg, IRECT rect, int param = kNoParameter, IActionFunction actionFunc = nullptr,
                  const IVColorSpec& colorSpec = DEFAULT_SPEC,
                  uint32_t numStates = 2, EDirection dir = kVertical);

  void Draw(IGraphics& graphics)  override;
  
private:
  float mStep;
  EDirection mDirection;
};

/** A vector knob control drawn using graphics primitves */
class IVKnobControl : public IKnobControlBase
                    , public IVectorBase
{
public:
  IVKnobControl(IDelegate& dlg, IRECT rect, int param,
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
  IVSVGKnob(IDelegate& dlg, IRECT rect, ISVG& svg, int param = kNoParameter)
    : IKnobControlBase(dlg, rect, param)
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
    SetDirty(false);
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
    //kFR = kFR
  };
  
  IVKeyboardControl(IDelegate& dlg, IRECT rect,
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
                  const char *txtOff = "off", const char *txtOn = "on");
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
  void SetRect(IRECT r)
  {
    mRECT = mTargetRECT = r;
    SetDirty(false);
  }

protected:
  WDL_String mTxtOff, mTxtOn;
  float mTxtH[2]; // [off, on], needed for nice multiline text drawing
  float mTxtW[2];

  bool mDrawBorders = true;
  bool mDrawShadows = true;
  bool mEmboss = false;
  float mShadowOffset = 3.0;

  void DrawInnerShadowForRect(IRECT r, IColor shadowColor, IGraphics& graphics);
  void DrawOuterShadowForRect(IRECT r, IColor shadowColor, IGraphics& graphics)
  {
    auto sr = ShiftRectBy(r, mShadowOffset, mShadowOffset);
    graphics.FillRect(shadowColor, sr);
  }
  IRECT GetRectToAlignTextIn(IRECT r, int state);
  IRECT GetRectToFitTextIn(IRECT r, float fontSize, float widthInSymbols, float numLines, float padding = 0.0);
  IRECT GetButtonRect();
  IRECT ShiftRectBy(IRECT r, float x, float y = 0.0)
  {
    return IRECT(r.L + x, r.T + y, r.R + x, r.B + y);
  }
};

class IVContactControl : public IVButtonControl
{
public:
  IVContactControl(IPlugBaseGraphics& plug, IRECT rect, int param,
                   const char *txtOff = "off", const char *txtOn = "on") :
    IVButtonControl(plug, rect, param, txtOff, txtOn) {};

  ~IVContactControl() {};

  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    mValue = 0.0;
    SetDirty();
  }
};

/** A vector drop down list.
Put this control on top of a draw stack
so that the expanded list is fully visible
and dosn't close when mouse is over another control */
class IVDropDownList : public IControl,
  public IVectorBase
{
  typedef WDL_PtrList<WDL_String> strBuf;

  static const IColor IVDropDownList::DEFAULT_BG_COLOR;
  static const IColor IVDropDownList::DEFAULT_FR_COLOR;
  static const IColor IVDropDownList::DEFAULT_TXT_COLOR;
  static const IColor IVDropDownList::DEFAULT_HL_COLOR;

  // map to IVectorBase colors
  enum EVBColor
  {
    lTxt = kFG,
    lBG = kBG,
    lHL = kHL,
    lFR = kFR
  };

public:

  IVDropDownList(IPlugBaseGraphics& plug, IRECT rect, int param);
  IVDropDownList(IPlugBaseGraphics& plug, IRECT rect, int param,
                 int numStates, const char* names...);

  ~IVDropDownList()
  {
    mValNames.Empty(true);
  };

  void Draw(IGraphics& graphics) override;
  void OnMouseOver(float x, float y, const IMouseMod& mod) override;
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
  {
    OnMouseOver(x, y, mod);
  }
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override;
  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override;
  void OnMouseOut() override;
  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    mBlink = false;
    SetDirty(false);
  }
  void OnResize() override;

  void SetDrawBorders(bool draw)
  {
    mDrawBorders = draw;
    SetDirty(false);
  }
  void SetDrawShadows(bool draw, bool keepButtonRect = true);
  void SetEmboss(bool emboss, bool keepButtonRect = true);
  void SetShadowOffset(float offset, bool keepButtonRect = true);
  void SetRect(IRECT r)
  {
    mInitRect = r;
    UpdateRectsOnInitChange();
    mLastX = mLastY = -1.0;
    SetDirty(false);
  }

  void SetMaxListHeight(int numItems)
  {
    mColHeight = numItems;
  }
  void SetNames(int numStates, const char* names...);
  void FillNamesFromParamDisplayTexts();

protected:
  IRECT mInitRect;
  strBuf mValNames;

  bool mExpanded = false;
  bool mBlink = false;
  bool mDrawBorders = true;
  bool mDrawShadows = true;
  bool mEmboss = false;
  float mShadowOffset = 3.0;

  float mLastX = -1.0; // to avoid lots of useless extra computations
  float mLastY = -1.0;
  int mState = -1;

  int mColHeight = 5; // how long the list can get before adding a new column

  void SetNames(int numStates, const char* names, va_list args);
  auto NameForVal(int val)
  {
    return (mValNames.Get(val))->Get();
  }

  int NumStates()
  {
    return mValNames.GetSize();
  }
  double NormalizedFromState()
  {
    if (NumStates() < 2)
      return 0.0;
    else
      return (double) mState / (NumStates() - 1);
  }
  int StateFromNormalized()
  {
    return (int) (mValue * (NumStates() - 1));
  }

  IRECT GetInitRect();
  IRECT GetExpandedRect();
  void ExpandRects();
  void ShrinkRects()
  {
    mTargetRECT = mRECT = mInitRect;
  }
  void UpdateRectsOnInitChange();

  void DrawInnerShadowForRect(IRECT r, IColor shadowColor, IGraphics& graphics);
  void DrawOuterShadowForRect(IRECT r, IColor shadowColor, IGraphics& graphics)
  {
    auto sr = ShiftRectBy(r, mShadowOffset, mShadowOffset);
    graphics.FillRect(shadowColor, sr);
  }
  IRECT GetRectToAlignTextIn(IRECT r);
  IRECT ShiftRectBy(IRECT r, float x, float y = 0.0)
  {
    return IRECT(r.L + x, r.T + y, r.R + x, r.B + y);
  }


  void DbgMsg(const char* msg, float val)
  {
#ifdef _DEBUG
    char str[32];
    int p = 0;
    while (*msg != '\0')
    {
      str[p] = *msg;
      ++msg;
      ++p;
    }
    sprintf(str + p, "%f", val);
    DBGMSG(str);
#endif
  }
};

#pragma mark - Bitmap Controls

/** A vector switch control. Click to cycle through states. */
class IBSwitchControl : public IBitmapControl
{
public:
  IBSwitchControl(IDelegate& dlg, float x, float y, int param, IBitmap& bitmap)
  : IBitmapControl(dlg, x, y, param, bitmap) {}
  ~IBSwitchControl() {}

  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override {  OnMouseDown(x, y, mod); }
};

/** A slider with a bitmap for the handle. The bitmap snaps to a mouse click or drag. */
class IBSliderControl : public IControl
{
public:
  IBSliderControl(IDelegate& dlg, float x, float y, int len, int param,
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
  IBTextControl(IDelegate& dlg, IRECT rect, IBitmap& bitmap, const IText& text = DEFAULT_TEXT, const char* str = "", int charWidth = 6, int charHeight = 12, int charOffset = 0, bool multiLine = false, bool vCenter = true, EBlendType bl = kBlendNone)
  : ITextControl(dlg, rect, text, str)
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

