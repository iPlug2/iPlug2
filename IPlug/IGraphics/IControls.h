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
  IBSwitchControl(IPlugBaseGraphics& plug, float x, float y, int param, IBitmap& bitmap)
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

const IColor DEFAULT_BK_COLOR = IColor(255, 70, 70, 70);
const IColor DEFAULT_WK_COLOR = IColor(255, 240, 240, 240);
const IColor DEFAULT_PK_COLOR = IColor(60, 0, 0, 0);
const IColor DEFAULT_FR_COLOR = DEFAULT_BK_COLOR;

class IVKeyboardControl : public IControl
{
public:
  IVKeyboardControl(IPlugBaseGraphics& plug, IRECT rect, int minNote, int maxNote);
  
//  IVKeyboardControl(IPlugBaseGraphics& pPlug, float x, float y, int minNote, int maxNote)
//  : IVKeyboardControl(pPlug, x, y, -1.0, 70.0, minNote, maxNote) {}
//
  ~IVKeyboardControl();
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;  
  void OnMouseUp(float x, float y, const IMouseMod& mod) override;
  void OnMouseOut() override;
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override;
  void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override;
  void OnMouseOver(float x, float y, const IMouseMod& pMod) override;
  void OnResize() override;

  void Draw(IGraphics& graphics) override;
  
  void SetMinMaxNote(int min, int max, bool keepWidth = true);  
  void SetNoteIsPlayed(int noteNum, bool played);
  void SetBlackToWhiteWidthAndHeightRatios(float widthR, float heightR = 0.6);
  void SetHeight(float h, bool keepProportions = false);
  void SetWidth(float w, bool keepProportions = false);
  void SetShowNotesAndVelocity(bool show);
  void SetColors(const IColor bkColor, const IColor& wkColor, const IColor& pkColor = DEFAULT_PK_COLOR, const IColor& frColor = DEFAULT_FR_COLOR);
 
  void SetDrawShadows(bool draw)
  {
    mDrawShadows = draw;
    SetDirty();
  }

  void SetDrawBorders(bool draw)
  {
    mDrawBorders = draw;
    SetDirty();
  }
  
private:  
  void RecreateRects(bool keepWidth);
  int GetKeyUnderMouse(float x, float y);
  void UpdateVelocity(float y);
  void GetNoteNameStr(int midiNoteNum, bool addOctave, WDL_String& str);
  
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

  bool KeyIsBlack(int i) const
  {
    return *(mKeyIsBlack.Get(i));
  }

  IRECT KeyRect(int i)
  {
    return *(mKeyRects.Get(i));
  }

  IRECT* KeyRectPtr(int i)
  {
    return mKeyRects.Get(i);
  }

protected:
  bool mShowNoteAndVel = false;
  bool mDrawShadows = true;
  bool mDrawBorders = true;
  IColor mBKColor = DEFAULT_BK_COLOR;
  IColor mWKColor = DEFAULT_WK_COLOR;
  IColor mPKColor = DEFAULT_PK_COLOR; // pressed key color
  IColor mFRColor = DEFAULT_FR_COLOR; // frame color

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

