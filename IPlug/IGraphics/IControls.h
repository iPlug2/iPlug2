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

/**@}*/

