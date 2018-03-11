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
  IVSwitchControl(IDelegate& dlg, IRECT bounds, int paramIdx = kNoParameter, IActionFunction actionFunc = nullptr, const IVColorSpec& colorSpec = DEFAULT_SPEC, int numStates = 2, EDirection dir = kVertical);

  void Draw(IGraphics& g) override;

  IRECT GetHandleBounds() override;
  void Animate(double progress) override { mFlashCircleRadius = progress * mRECT.W() / 2.; SetDirty(false); }

private:
  float mStep;
  float mFlashCircleRadius = 0.f;
  EDirection mDirection;
};

/** A vector knob control drawn using graphics primitves */
class IVKnobControl : public IKnobControlBase
                    , public IVectorBase
{
public:
  IVKnobControl(IDelegate& dlg, IRECT bounds, int paramIdx,
                const IVColorSpec& colorSpec = DEFAULT_SPEC,
                float rMin = 0.f, float rMax = 1.f, float aMin = -135.f, float aMax = 135.f,
                EDirection direction = kVertical, double gearing = DEFAULT_GEARING);
  ~IVKnobControl() {}

  void Draw(IGraphics& g) override;
  
protected:
  float mAngleMin, mAngleMax, mInnerRadius, mOuterRadius;
};

/** A vector knob control which rotates an SVG image */
class IVSVGKnob : public IKnobControlBase
{
public:
  IVSVGKnob(IDelegate& dlg, IRECT bounds, ISVG& svg, int paramIdx = kNoParameter)
    : IKnobControlBase(dlg, bounds, paramIdx)
    , mSVG(svg)
  {
  }

  void Draw(IGraphics& g) override
  {
#ifdef IGRAPHICS_LICE
    g.DrawText(mText, "UNSUPPORTED", mRECT);
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

class IVSliderControl : public ISliderControlBase
                      , public IVectorBase
{
public:
  IVSliderControl(IDelegate& dlg, IRECT bounds, int paramIdx = kNoParameter,
                  const IVColorSpec& colorSpec = DEFAULT_SPEC,
                  EDirection dir = kVertical, bool onlyHandle = false, int handleSize = 10)
  : ISliderControlBase(dlg, bounds, paramIdx, dir, onlyHandle, handleSize)
  , IVectorBase(colorSpec)
  {
    AttachIControl(this);
  }
  
  IVSliderControl(IDelegate& dlg, IRECT bounds, IActionFunction aF,
                  const IVColorSpec& colorSpec = DEFAULT_SPEC,
                  EDirection dir = kVertical, bool onlyHandle = false, int handleSize = 10)
  : ISliderControlBase(dlg, bounds, aF, dir, onlyHandle, handleSize)
  , IVectorBase(colorSpec)
  {
    AttachIControl(this);
  }
  
  void Draw(IGraphics& g) override;
  void OnResize() override;
};

class IVContactControl : public IVSwitchControl
{
public:
  IVContactControl(IDelegate& dlg, IRECT bounds, int paramIdx)
  : IVSwitchControl(dlg, bounds, paramIdx)
  {};

  ~IVContactControl() {};

  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    mValue = 0.0;
    SetDirty();
  }
};

#pragma mark - Bitmap Controls

/** A bitmap switch control. Click to cycle through states. */
class IBSwitchControl : public IBitmapControl
{
public:
  IBSwitchControl(IDelegate& dlg, float x, float y, int paramIdx, IBitmap& bitmap)
  : IBitmapControl(dlg, x, y, paramIdx, bitmap) {}
  ~IBSwitchControl() {}

  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override {  OnMouseDown(x, y, mod); }
};

/** A slider with a bitmap for the handle. The bitmap snaps to a mouse click or drag. */
//class IBSliderControl : public IControl
//{
//public:
//  IBSliderControl(IDelegate& dlg, float x, float y, int len, int paramIdx,
//                  IBitmap& bitmap, EDirection direction = kVertical, bool onlyHandle = false);
//  ~IBSliderControl() {}
//
//  virtual void OnMouseDown(float x, float y, const IMouseMod& mod) override;
//  virtual void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override { return SnapToMouse(x, y, mDirection, mTrack); }
//  virtual void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override;
//  virtual void Draw(IGraphics& g) override;
//  virtual bool IsHit(float x, float y) const override;
//  virtual void OnRescale() override;
//  virtual void OnResize() override;
//
//  int GetLength() const { return mLen; }
//  int GetHandleHeadroom() const { return mHandleHeadroom; }
//  double GetHandleValueHeadroom() const { return (double) mHandleHeadroom / (double) mLen; }
//  IRECT GetHandleRECT(double value = -1.0) const;
//protected:
////  virtual void SnapToMouse(float x, float y);
//  int mLen, mHandleHeadroom;
//  IBitmap mHandleBitmap;
//  EDirection mDirection;
//  IRECT mTrack;
//  bool mOnlyHandle;
//};

class IBSliderControl : public ISliderControlBase
{
public:
  IBSliderControl(IDelegate& dlg, IRECT bounds, int paramIdx, IBitmap& handleBitmap,
                  EDirection dir = kVertical, bool onlyHandle = false);
  
  ~IBSliderControl() {}

  virtual void Draw(IGraphics& g) override;
  virtual void OnRescale() override;
  virtual void OnResize() override;
  
  void GrayOut(bool gray) override
  {
    mBlend.mWeight = (gray ? GRAYED_ALPHA : 1.0f);
    IControl::GrayOut(gray);
  }
  
private:
  IBitmap mHandleBitmap;
  IBlend mBlend;
};

/** Display monospace bitmap font text */
// TODO: fix Centre/Right aligned behaviour when string exceeds bounds or should wrap onto new line
class IBTextControl : public ITextControl
{
public:
  IBTextControl(IDelegate& dlg, IRECT bounds, IBitmap& bitmap, const IText& text = DEFAULT_TEXT, const char* str = "", int charWidth = 6, int charHeight = 12, int charOffset = 0, bool multiLine = false, bool vCenter = true, EBlendType blend = kBlendNone)
  : ITextControl(dlg, bounds, text, str)
  , mCharWidth(charWidth)
  , mCharHeight(charHeight)
  , mCharOffset(charOffset)
  , mMultiLine(multiLine)
  , mVCentre(vCenter)
  , mTextBitmap(bitmap)
  , mBlend(blend)
  {
    mStr.Set(str);
  }

  void Draw(IGraphics& g) override
  {
    g.DrawBitmapedText(mTextBitmap, mRECT, mText, &mBlend, mStr.Get(), mVCentre, mMultiLine, mCharWidth, mCharHeight, mCharOffset);
  }
  
  void GrayOut(bool gray) override
  {
    mBlend.mWeight = (gray ? GRAYED_ALPHA : 1.0f);
    IControl::GrayOut(gray);
  }

protected:
  WDL_String mStr;
  int mCharWidth, mCharHeight, mCharOffset;
  bool mMultiLine;
  bool mVCentre;
  IBitmap mTextBitmap;
  IBlend mBlend;
};

/**@}*/

